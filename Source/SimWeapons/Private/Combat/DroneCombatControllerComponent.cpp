// Fill out your copyright notice in the Description page of Project Settings.

#include "Combat/DroneCombatControllerComponent.h"

#include "Carriers/Drone/SimDroneCarrier.h"
#include "Components/MK1EyeballComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

UDroneCombatControllerComponent::UDroneCombatControllerComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UDroneCombatControllerComponent::BeginPlay()
{
	Super::BeginPlay();

	CacheRequiredComponents();
	UpdateLastSafeDroneTransform();

	if (bAutoStart)
	{
		StartCombatController();
	}
}

void UDroneCombatControllerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ScanTimerHandle);
		World->GetTimerManager().ClearTimer(ReceiverEnergyTimerHandle);
	}

	Super::EndPlay(EndPlayReason);
}

void UDroneCombatControllerComponent::TickComponent(
	float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction
)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (DeltaTime <= 0.0f)
	{
		return;
	}

	if (TryRecoverFromBodyOverlap(DeltaTime))
	{
		return;
	}

	if (ShouldRunAttackPreparation())
	{
		UpdateAttackPreparation(DeltaTime);
		return;
	}

	if (!bMoveToCurrentTarget)
	{
		UpdateLastSafeDroneTransform();
		return;
	}

	MoveTowardCurrentTarget(DeltaTime);
}

void UDroneCombatControllerComponent::StartCombatController()
{
	if (!CacheRequiredComponents())
	{
		SetCombatState(EDroneCombatState::Disabled);
		SetComponentTickEnabled(false);
		return;
	}

	UWorld* World = GetWorld();

	if (!World)
	{
		SetCombatState(EDroneCombatState::Disabled);
		SetComponentTickEnabled(false);
		return;
	}

	World->GetTimerManager().ClearTimer(ScanTimerHandle);
	World->GetTimerManager().ClearTimer(ReceiverEnergyTimerHandle);

	UpdateLastSafeDroneTransform();

	SetComponentTickEnabled(true);
	SetCombatState(EDroneCombatState::Idle);

	World->GetTimerManager().SetTimer(
		ScanTimerHandle,
		this,
		&UDroneCombatControllerComponent::ScanForTarget,
		ScanInterval,
		true,
		0.2f
	);

	if (bConsumeReceiverEnergy)
	{
		World->GetTimerManager().SetTimer(
			ReceiverEnergyTimerHandle,
			this,
			&UDroneCombatControllerComponent::ConsumeReceiverEnergy,
			ReceiverEnergyInterval,
			true,
			ReceiverEnergyInterval
		);
	}

	PrintDebugMessage(TEXT("Drone combat controller started."));
}

void UDroneCombatControllerComponent::StopCombatController()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ScanTimerHandle);
		World->GetTimerManager().ClearTimer(ReceiverEnergyTimerHandle);
	}

	SetComponentTickEnabled(false);
	SetCurrentTarget(nullptr);
	ClearTargetMemory();
	SetCombatState(EDroneCombatState::Idle);

	PrintDebugMessage(TEXT("Drone combat controller stopped."));
}

void UDroneCombatControllerComponent::ScanForTarget()
{
	if (!CacheRequiredComponents())
	{
		SetCombatState(EDroneCombatState::Disabled);
		return;
	}

	if (!DroneCarrier->IsAlive())
	{
		SetCurrentTarget(nullptr);
		ClearTargetMemory();
		SetCombatState(EDroneCombatState::Disabled);
		PrintDebugMessage(TEXT("Drone is destroyed. Scan skipped."));
		return;
	}

	const float ScanCost = DroneCarrier->GetVisionRadarSensorCost();

	if (!DroneCarrier->HasEnoughEnergy(ScanCost))
	{
		SetCurrentTarget(nullptr);

		if (HasValidTargetMemory())
		{
			SetCombatState(EDroneCombatState::LostSight);
			PrintDebugMessage(TEXT("Not enough energy for scan. Keeping last known target location."));
		}
		else
		{
			ClearTargetMemory();
			SetCombatState(EDroneCombatState::Idle);
			PrintDebugMessage(TEXT("Not enough energy for vision/radar scan."));
		}

		return;
	}

	if (!DroneCarrier->ConsumeEnergy(ScanCost))
	{
		SetCurrentTarget(nullptr);

		if (HasValidTargetMemory())
		{
			SetCombatState(EDroneCombatState::LostSight);
			PrintDebugMessage(TEXT("Failed to consume scan energy. Keeping last known target location."));
		}
		else
		{
			ClearTargetMemory();
			SetCombatState(EDroneCombatState::Idle);
			PrintDebugMessage(TEXT("Failed to consume scan energy."));
		}

		return;
	}

	SetCombatState(EDroneCombatState::Scanning);

	const TArray<AActor*> VisibleTargets = VisionSensor->GetTargetsInFrustum();

	AActor* SelectedTarget = SelectTarget(VisibleTargets);

	SetCurrentTarget(SelectedTarget);

	if (SelectedTarget)
	{
		RememberTarget(SelectedTarget);
		UpdateTargetVelocityEstimate(SelectedTarget);

		SetCombatState(EDroneCombatState::TargetFound);

		PrintDebugMessage(
			FString::Printf(
				TEXT("Target found: %s | Energy: %.2f / %.2f"),
				*SelectedTarget->GetName(),
				DroneCarrier->GetCurrentEnergy(),
				DroneCarrier->GetMaxEnergy()
			)
		);

		if (bDrawDebugTargetLine)
		{
			if (const AActor* OwnerActor = GetOwner())
			{
				DrawDebugLine(
					GetWorld(),
					OwnerActor->GetActorLocation(),
					SelectedTarget->GetActorLocation(),
					FColor::Green,
					false,
					DebugDrawDuration,
					0,
					3.0f
				);
			}
		}

		return;
	}

	if (HasValidTargetMemory())
	{
		SetCombatState(EDroneCombatState::LostSight);

		PrintDebugMessage(
			FString::Printf(
				TEXT("No visible target. Moving to last known position: %s | Energy: %.2f / %.2f"),
				*LastKnownTargetLocation.ToString(),
				DroneCarrier->GetCurrentEnergy(),
				DroneCarrier->GetMaxEnergy()
			)
		);

		if (bDrawDebugLastKnownTargetLocation)
		{
			DrawDebugSphere(
				GetWorld(),
				LastKnownTargetLocation,
				100.0f,
				16,
				FColor::Magenta,
				false,
				DebugDrawDuration
			);
		}

		return;
	}

	ClearTargetMemory();
	SetCombatState(EDroneCombatState::Idle);

	PrintDebugMessage(
		FString::Printf(
			TEXT("No target found. Energy: %.2f / %.2f"),
			DroneCarrier->GetCurrentEnergy(),
			DroneCarrier->GetMaxEnergy()
		)
	);
}

void UDroneCombatControllerComponent::ConsumeReceiverEnergy()
{
	if (!CacheRequiredComponents())
	{
		SetCombatState(EDroneCombatState::Disabled);
		return;
	}

	if (!DroneCarrier->IsAlive())
	{
		SetCurrentTarget(nullptr);
		ClearTargetMemory();
		SetCombatState(EDroneCombatState::Disabled);
		return;
	}

	const float ReceiverCost = DroneCarrier->GetReceiverCostPerSecond() * ReceiverEnergyInterval;

	if (!DroneCarrier->ConsumeEnergy(ReceiverCost))
	{
		SetCurrentTarget(nullptr);
		ClearTargetMemory();
		SetCombatState(EDroneCombatState::Disabled);
		PrintDebugMessage(TEXT("Drone has no energy for receiver."));
		return;
	}

	PrintDebugMessage(
		FString::Printf(
			TEXT("Receiver energy consumed: %.2f | Energy: %.2f / %.2f"),
			ReceiverCost,
			DroneCarrier->GetCurrentEnergy(),
			DroneCarrier->GetMaxEnergy()
		)
	);
}

AActor* UDroneCombatControllerComponent::GetCurrentTarget() const
{
	return CurrentTarget;
}

bool UDroneCombatControllerComponent::HasCurrentTarget() const
{
	return IsValid(CurrentTarget);
}

EDroneCombatState UDroneCombatControllerComponent::GetCombatState() const
{
	return CombatState;
}

float UDroneCombatControllerComponent::GetCurrentTargetHorizontalDistance() const
{
	const AActor* OwnerActor = GetOwner();

	if (!OwnerActor || !IsValid(CurrentTarget))
	{
		return -1.0f;
	}

	return FVector::Dist2D(
		OwnerActor->GetActorLocation(),
		CurrentTarget->GetActorLocation()
	);
}