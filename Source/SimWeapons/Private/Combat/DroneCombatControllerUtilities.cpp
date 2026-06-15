// Fill out your copyright notice in the Description page of Project Settings.

#include "Combat/DroneCombatControllerComponent.h"

#include "Carriers/Drone/SimDroneCarrier.h"
#include "Carriers/SimWeaponMountComponent.h"
#include "CollisionQueryParams.h"
#include "Components/MK1EyeballComponent.h"
#include "Engine/Engine.h"
#include "GameFramework/Actor.h"

bool UDroneCombatControllerComponent::CacheRequiredComponents()
{
	AActor* OwnerActor = GetOwner();

	if (!OwnerActor)
	{
		return false;
	}

	if (!IsValid(DroneCarrier))
	{
		DroneCarrier = OwnerActor->FindComponentByClass<USimDroneCarrier>();
	}

	if (!IsValid(VisionSensor))
	{
		VisionSensor = OwnerActor->FindComponentByClass<UMK1EyeballComponent>();
	}

	if (!IsValid(RocketWeaponMount))
	{
		RocketWeaponMount = FindWeaponMountByName(RocketWeaponMountComponentName);
	}

	if (!IsValid(DroneCarrier))
	{
		PrintDebugMessage(TEXT("DroneCombatControllerComponent: SimDroneCarrier component is missing."));
		return false;
	}

	if (!IsValid(VisionSensor))
	{
		PrintDebugMessage(TEXT("DroneCombatControllerComponent: MK1EyeballComponent is missing."));
		return false;
	}

	return true;
}

void UDroneCombatControllerComponent::SetCurrentTarget(AActor* NewTarget)
{
	if (CurrentTarget == NewTarget)
	{
		return;
	}

	CurrentTarget = NewTarget;

	OnTargetChanged.Broadcast(CurrentTarget);
}

void UDroneCombatControllerComponent::SetCombatState(EDroneCombatState NewState)
{
	if (CombatState == NewState)
	{
		return;
	}

	CombatState = NewState;

	OnCombatStateChanged.Broadcast(CombatState);
}

AActor* UDroneCombatControllerComponent::SelectTarget(const TArray<AActor*>& Targets) const
{
	if (Targets.IsEmpty())
	{
		return nullptr;
	}

	if (!bSelectClosestTarget)
	{
		for (AActor* Target : Targets)
		{
			if (IsValid(Target) && Target != GetOwner())
			{
				return Target;
			}
		}

		return nullptr;
	}

	const AActor* OwnerActor = GetOwner();

	if (!OwnerActor)
	{
		return nullptr;
	}

	const FVector OwnerLocation = OwnerActor->GetActorLocation();

	AActor* ClosestTarget = nullptr;
	float ClosestDistanceSquared = TNumericLimits<float>::Max();

	for (AActor* Target : Targets)
	{
		if (!IsValid(Target) || Target == OwnerActor)
		{
			continue;
		}

		const float DistanceSquared = FVector::DistSquared(
			OwnerLocation,
			Target->GetActorLocation()
		);

		if (DistanceSquared < ClosestDistanceSquared)
		{
			ClosestDistanceSquared = DistanceSquared;
			ClosestTarget = Target;
		}
	}

	return ClosestTarget;
}

void UDroneCombatControllerComponent::AddDroneIgnoredActors(
	FCollisionQueryParams& QueryParams,
	bool bIgnoreCurrentTarget
) const
{
	const AActor* OwnerActor = GetOwner();

	if (!OwnerActor)
	{
		return;
	}

	QueryParams.AddIgnoredActor(OwnerActor);

	TArray<AActor*> AttachedActors;
	OwnerActor->GetAttachedActors(AttachedActors, true, true);

	for (AActor* AttachedActor : AttachedActors)
	{
		if (IsValid(AttachedActor))
		{
			QueryParams.AddIgnoredActor(AttachedActor);
		}
	}

	TArray<USimWeaponMountComponent*> WeaponMounts;
	OwnerActor->GetComponents<USimWeaponMountComponent>(WeaponMounts);

	for (USimWeaponMountComponent* WeaponMount : WeaponMounts)
	{
		if (!IsValid(WeaponMount))
		{
			continue;
		}

		if (AActor* SpawnedWeapon = WeaponMount->GetSpawnedWeapon())
		{
			QueryParams.AddIgnoredActor(SpawnedWeapon);
		}
	}

	if (bIgnoreCurrentTarget && IsValid(CurrentTarget))
	{
		QueryParams.AddIgnoredActor(CurrentTarget);
	}
}

void UDroneCombatControllerComponent::PrintDebugMessage(const FString& Message) const
{
	if (!bPrintDebugMessages)
	{
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("%s"), *Message);

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			-1,
			2.0f,
			FColor::Cyan,
			Message
		);
	}
}