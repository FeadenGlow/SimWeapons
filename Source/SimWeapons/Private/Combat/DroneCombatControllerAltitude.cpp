// Fill out your copyright notice in the Description page of Project Settings.

#include "Combat/DroneCombatControllerComponent.h"

#include "Carriers/Drone/SimDroneCarrier.h"
#include "Carriers/SimWeaponMountComponent.h"
#include "CollisionQueryParams.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

bool UDroneCombatControllerComponent::GetGroundLocationBelowDrone(FVector& OutGroundLocation) const
{
	OutGroundLocation = FVector::ZeroVector;

	const UWorld* World = GetWorld();
	const AActor* OwnerActor = GetOwner();

	if (!World || !OwnerActor || GroundTraceDistanceCm <= 0.0f)
	{
		return false;
	}

	const FVector StartLocation = OwnerActor->GetActorLocation();
	const FVector EndLocation = StartLocation - FVector::UpVector * GroundTraceDistanceCm;

	FCollisionQueryParams QueryParams(TEXT("DroneCombatGroundTrace"), false);
	AddDroneIgnoredActors(QueryParams, true);

	FHitResult GroundHit;
	const bool bHit = World->LineTraceSingleByChannel(
		GroundHit,
		StartLocation,
		EndLocation,
		GroundTraceChannel,
		QueryParams
	);

	if (bDrawDebugGroundTrace)
	{
		DrawDebugLine(
			GetWorld(),
			StartLocation,
			EndLocation,
			bHit ? FColor::Green : FColor::Red,
			false,
			DebugDrawDuration,
			0,
			1.5f
		);

		if (bHit)
		{
			DrawDebugSphere(
				GetWorld(),
				GroundHit.ImpactPoint,
				35.0f,
				12,
				FColor::Green,
				false,
				DebugDrawDuration
			);
		}
	}

	if (!bHit || !GroundHit.bBlockingHit)
	{
		return false;
	}

	OutGroundLocation = GroundHit.ImpactPoint;
	return true;
}

bool UDroneCombatControllerComponent::GetMinimumGroundSafeAltitudeZ(float& OutMinimumOwnerZ) const
{
	OutMinimumOwnerZ = 0.0f;

	if (!bUseGroundHeightControl)
	{
		return false;
	}

	FVector GroundLocation = FVector::ZeroVector;

	if (!GetGroundLocationBelowDrone(GroundLocation))
	{
		return false;
	}

	OutMinimumOwnerZ = GroundLocation.Z + DesiredAltitudeAboveGroundCm;
	return true;
}

float UDroneCombatControllerComponent::GetRocketMountZOffsetFromOwner() const
{
	const AActor* OwnerActor = GetOwner();

	if (!OwnerActor)
	{
		return 0.0f;
	}

	if (!IsValid(RocketWeaponMount))
	{
		return 0.0f;
	}

	return RocketWeaponMount->GetComponentLocation().Z - OwnerActor->GetActorLocation().Z;
}

bool UDroneCombatControllerComponent::GetDesiredOwnerAltitudeZ(float& OutDesiredOwnerZ) const
{
	OutDesiredOwnerZ = 0.0f;

	const AActor* OwnerActor = GetOwner();

	if (!OwnerActor)
	{
		return false;
	}

	bool bHasDesiredAltitude = false;
	float DesiredOwnerZ = OwnerActor->GetActorLocation().Z;

	const float RocketMountZOffset = GetRocketMountZOffsetFromOwner();

	if (IsValid(CurrentTarget))
	{
		const FVector AimLocation = GetPredictedTargetAimLocation();

		DesiredOwnerZ = AimLocation.Z - RocketMountZOffset;
		bHasDesiredAltitude = true;
	}
	else if (HasValidTargetMemory())
	{
		DesiredOwnerZ = LastKnownTargetLocation.Z - RocketMountZOffset;
		bHasDesiredAltitude = true;
	}

	float MinimumGroundSafeZ = 0.0f;

	if (GetMinimumGroundSafeAltitudeZ(MinimumGroundSafeZ))
	{
		if (bHasDesiredAltitude)
		{
			DesiredOwnerZ = FMath::Max(DesiredOwnerZ, MinimumGroundSafeZ);
		}
		else
		{
			DesiredOwnerZ = MinimumGroundSafeZ;
		}

		bHasDesiredAltitude = true;
	}

	if (!bHasDesiredAltitude)
	{
		return false;
	}

	OutDesiredOwnerZ = DesiredOwnerZ;
	return true;
}

FVector UDroneCombatControllerComponent::ApplyAltitudeCorrectionToMove(
	const FVector& CurrentMoveDelta,
	float DeltaTime
) const
{
	if (!bUseGroundHeightControl || DeltaTime <= 0.0f)
	{
		return CurrentMoveDelta;
	}

	const AActor* OwnerActor = GetOwner();

	if (!OwnerActor)
	{
		return CurrentMoveDelta;
	}

	float DesiredOwnerZ = 0.0f;

	if (!GetDesiredOwnerAltitudeZ(DesiredOwnerZ))
	{
		return CurrentMoveDelta;
	}

	const float CurrentOwnerZ = OwnerActor->GetActorLocation().Z;
	const float AltitudeError = DesiredOwnerZ - CurrentOwnerZ;

	if (FMath::IsNearlyZero(AltitudeError, 1.0f))
	{
		return CurrentMoveDelta;
	}

	const bool bUsingCombatTargetAltitude = IsValid(CurrentTarget);

	const float InterpSpeed = bUsingCombatTargetAltitude
		? AttackAltitudeInterpSpeed
		: AltitudeInterpSpeed;

	const float MaxCorrectionSpeed = bUsingCombatTargetAltitude
		? MaxAttackAltitudeCorrectionSpeedCmPerSecond
		: MaxAltitudeCorrectionSpeedCmPerSecond;

	if (InterpSpeed <= 0.0f || MaxCorrectionSpeed <= 0.0f)
	{
		return CurrentMoveDelta;
	}

	const float DesiredCorrectionZ = AltitudeError * InterpSpeed * DeltaTime;
	const float MaxCorrectionZ = MaxCorrectionSpeed * DeltaTime;

	const float ClampedCorrectionZ = FMath::Clamp(
		DesiredCorrectionZ,
		-MaxCorrectionZ,
		MaxCorrectionZ
	);

	FVector CorrectedMoveDelta = CurrentMoveDelta;
	CorrectedMoveDelta.Z += ClampedCorrectionZ;

	return CorrectedMoveDelta;
}

void UDroneCombatControllerComponent::ApplyStationaryAltitudeControl(float DeltaTime)
{
	if (!bUseGroundHeightControl || DeltaTime <= 0.0f)
	{
		return;
	}

	if (!IsValid(DroneCarrier) || !DroneCarrier->IsAlive())
	{
		return;
	}

	AActor* OwnerActor = GetOwner();

	if (!OwnerActor)
	{
		return;
	}

	FVector MoveDelta = ApplyAltitudeCorrectionToMove(FVector::ZeroVector, DeltaTime);

	if (MoveDelta.IsNearlyZero())
	{
		return;
	}

	FHitResult BodyMoveHit;
	if (IsDroneBodyMoveBlocked(MoveDelta, BodyMoveHit))
	{
		PrintDebugMessage(
			FString::Printf(
				TEXT("Drone body altitude movement blocked by: %s"),
				BodyMoveHit.GetActor() ? *BodyMoveHit.GetActor()->GetName() : TEXT("Unknown")
			)
		);

		if (bDrawDebugDroneBodySafetyCollision && BodyMoveHit.bBlockingHit)
		{
			DrawDebugSphere(
				GetWorld(),
				BodyMoveHit.ImpactPoint,
				35.0f,
				12,
				FColor::Red,
				false,
				DebugDrawDuration
			);
		}

		return;
	}

	const float ActualSpeedCmPerSecond = MoveDelta.Size() / DeltaTime;
	const float MovementEnergyCost = CalculateMovementEnergyCost(ActualSpeedCmPerSecond, DeltaTime);

	if (MovementEnergyCost > 0.0f && !DroneCarrier->ConsumeEnergy(MovementEnergyCost))
	{
		SetCurrentTarget(nullptr);
		ClearTargetMemory();
		SetCombatState(EDroneCombatState::Disabled);
		PrintDebugMessage(TEXT("Drone has no energy for altitude control."));
		return;
	}

	const FVector OwnerLocation = OwnerActor->GetActorLocation();
	const FVector NewLocation = OwnerLocation + MoveDelta;

	FHitResult SweepHit;
	const bool bMoved = OwnerActor->SetActorLocation(
		NewLocation,
		bUseSweepMovement,
		&SweepHit
	);

	if (bUseSweepMovement && SweepHit.bBlockingHit)
	{
		PrintDebugMessage(
			FString::Printf(
				TEXT("Drone altitude movement blocked by root sweep: %s"),
				SweepHit.GetActor() ? *SweepHit.GetActor()->GetName() : TEXT("Unknown")
			)
		);

		DrawDebugSphere(
			GetWorld(),
			SweepHit.ImpactPoint,
			30.0f,
			12,
			FColor::Red,
			false,
			DebugDrawDuration
		);

		return;
	}

	if (!bMoved)
	{
		PrintDebugMessage(TEXT("Drone altitude movement failed."));
		return;
	}

	UpdateLastSafeDroneTransform();

	if (bDrawDebugMovementLine)
	{
		DrawDebugLine(
			GetWorld(),
			OwnerLocation,
			NewLocation,
			FColor::Cyan,
			false,
			DebugDrawDuration,
			0,
			2.0f
		);
	}
}