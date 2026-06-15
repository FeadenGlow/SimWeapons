// Fill out your copyright notice in the Description page of Project Settings.

#include "Combat/DroneCombatControllerComponent.h"

#include "Carriers/Drone/SimDroneCarrier.h"
#include "Carriers/SimWeaponMountComponent.h"
#include "CollisionQueryParams.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

bool UDroneCombatControllerComponent::HasClearAttackLineToCurrentTarget()
{
	FHitResult HitResult;
	return TraceAttackLineToCurrentTarget(HitResult);
}

bool UDroneCombatControllerComponent::ShouldRunAttackPreparation() const
{
	if (!bUseAttackPreparation)
	{
		return false;
	}

	if (bHasFiredPrimaryWeapon)
	{
		return false;
	}

	if (!IsValid(CurrentTarget))
	{
		return false;
	}

	const AActor* OwnerActor = GetOwner();

	if (!OwnerActor)
	{
		return false;
	}

	if (
		CombatState == EDroneCombatState::PositioningForAttack ||
		CombatState == EDroneCombatState::Aiming ||
		CombatState == EDroneCombatState::ReadyToFire ||
		CombatState == EDroneCombatState::AttackLineBlocked ||
		CombatState == EDroneCombatState::RecoveringFromCollision
	)
	{
		return true;
	}

	const float HorizontalDistance = FVector::Dist2D(
		OwnerActor->GetActorLocation(),
		CurrentTarget->GetActorLocation()
	);

	return HorizontalDistance <= AttackRangeCm + AttackPositionDistanceToleranceCm;
}

void UDroneCombatControllerComponent::UpdateAttackPreparation(float DeltaTime)
{
	if (DeltaTime <= 0.0f)
	{
		return;
	}

	if (!CacheRequiredComponents())
	{
		SetCombatState(EDroneCombatState::Disabled);
		return;
	}

	if (!IsValid(DroneCarrier) || !DroneCarrier->IsAlive())
	{
		SetCurrentTarget(nullptr);
		ClearTargetMemory();
		SetCombatState(EDroneCombatState::Disabled);
		return;
	}

	if (TryRecoverFromBodyOverlap(DeltaTime))
	{
		return;
	}

	if (!IsValid(CurrentTarget))
	{
		SetCombatState(EDroneCombatState::Idle);
		return;
	}

	if (!IsValid(RocketWeaponMount))
	{
		RocketWeaponMount = FindWeaponMountByName(RocketWeaponMountComponentName);
	}

	UpdateTargetVelocityEstimate(CurrentTarget);

	const FVector AimLocation = GetPredictedTargetAimLocation();

	AActor* OwnerActor = GetOwner();

	if (!OwnerActor)
	{
		SetCombatState(EDroneCombatState::Disabled);
		return;
	}

	const FVector OwnerLocation = OwnerActor->GetActorLocation();
	const FVector HorizontalToAim = FVector(
		AimLocation.X - OwnerLocation.X,
		AimLocation.Y - OwnerLocation.Y,
		0.0f
	);

	if (!HorizontalToAim.IsNearlyZero())
	{
		const float DesiredYaw = HorizontalToAim.Rotation().Yaw;
		const FRotator DesiredRotation = FRotator(0.0f, DesiredYaw, 0.0f);

		const FRotator NewRotation = FMath::RInterpTo(
			OwnerActor->GetActorRotation(),
			DesiredRotation,
			DeltaTime,
			RotationInterpSpeed
		);

		TrySetDroneRotationSafely(NewRotation);
	}

	const bool bPositionReady = ApplyAttackPositioningMove(AimLocation, DeltaTime);

	if (!bPositionReady)
	{
		SetCombatState(EDroneCombatState::PositioningForAttack);
		return;
	}

	const bool bAimed = IsRocketAimedAtLocation(AimLocation);

	FHitResult AttackLineHit;
	const bool bClearAttackLine = TraceAttackLineToCurrentTarget(AttackLineHit);

	if (bDrawDebugAimLine)
	{
		DrawDebugLine(
			GetWorld(),
			GetAttackTraceStartLocation(),
			AimLocation,
			bAimed ? FColor::Cyan : FColor::Yellow,
			false,
			DebugDrawDuration,
			0,
			3.0f
		);

		DrawDebugSphere(
			GetWorld(),
			AimLocation,
			45.0f,
			16,
			FColor::Cyan,
			false,
			DebugDrawDuration
		);
	}

	if (!bClearAttackLine)
	{
		SetCombatState(EDroneCombatState::AttackLineBlocked);

		PrintDebugMessage(
			FString::Printf(
				TEXT("Attack line blocked by: %s"),
				AttackLineHit.GetActor() ? *AttackLineHit.GetActor()->GetName() : TEXT("Unknown")
			)
		);

		return;
	}

	if (!bAimed)
	{
		SetCombatState(EDroneCombatState::Aiming);

		PrintDebugMessage(
			FString::Printf(
				TEXT("Aiming... angle error: %.2f deg | height error: %.2f cm"),
				GetRocketAimAngleToLocation(AimLocation),
				GetRocketHeightErrorToLocation(AimLocation)
			)
		);

		return;
	}

	SetCombatState(EDroneCombatState::ReadyToFire);

	PrintDebugMessage(
		FString::Printf(
			TEXT("Ready to fire. Aim angle: %.2f deg | height error: %.2f cm"),
			GetRocketAimAngleToLocation(AimLocation),
			GetRocketHeightErrorToLocation(AimLocation)
		)
	);
}

void UDroneCombatControllerComponent::UpdateTargetVelocityEstimate(AActor* Target)
{
	if (!IsValid(Target))
	{
		EstimatedTargetVelocity = FVector::ZeroVector;
		return;
	}

	const UWorld* World = GetWorld();

	if (!World)
	{
		EstimatedTargetVelocity = FVector::ZeroVector;
		return;
	}

	const float CurrentTimeSeconds = World->GetTimeSeconds();
	const FVector CurrentLocation = Target->GetActorLocation();

	if (LastObservedTargetTimeSeconds < 0.0f)
	{
		LastObservedTargetLocation = CurrentLocation;
		LastObservedTargetTimeSeconds = CurrentTimeSeconds;
		EstimatedTargetVelocity = FVector::ZeroVector;
		return;
	}

	const float DeltaSeconds = CurrentTimeSeconds - LastObservedTargetTimeSeconds;

	if (DeltaSeconds <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	EstimatedTargetVelocity = (CurrentLocation - LastObservedTargetLocation) / DeltaSeconds;

	LastObservedTargetLocation = CurrentLocation;
	LastObservedTargetTimeSeconds = CurrentTimeSeconds;
}

FVector UDroneCombatControllerComponent::GetPredictedTargetAimLocation() const
{
	const FVector CurrentAimLocation = GetCurrentTargetAimLocation();

	if (!bUseTargetPrediction)
	{
		return CurrentAimLocation;
	}

	if (RocketSpeedEstimateCmPerSecond <= KINDA_SMALL_NUMBER)
	{
		return CurrentAimLocation;
	}

	const FVector RocketStartLocation = GetAttackTraceStartLocation();
	const float DistanceToAim = FVector::Distance(RocketStartLocation, CurrentAimLocation);

	const float TimeToImpact = FMath::Clamp(
		DistanceToAim / RocketSpeedEstimateCmPerSecond,
		0.0f,
		MaxTargetPredictionTimeSeconds
	);

	return CurrentAimLocation + EstimatedTargetVelocity * TimeToImpact;
}

FVector UDroneCombatControllerComponent::GetCurrentTargetAimLocation() const
{
	if (!IsValid(CurrentTarget))
	{
		return FVector::ZeroVector;
	}

	return GetTargetAimLocation();
}

bool UDroneCombatControllerComponent::IsRocketAimedAtLocation(const FVector& AimLocation) const
{
	const float AimAngle = GetRocketAimAngleToLocation(AimLocation);
	const float HeightError = FMath::Abs(GetRocketHeightErrorToLocation(AimLocation));

	return AimAngle <= AimToleranceDegrees && HeightError <= AimHeightToleranceCm;
}

float UDroneCombatControllerComponent::GetRocketAimAngleToLocation(const FVector& AimLocation) const
{
	const FVector RocketStartLocation = GetAttackTraceStartLocation();
	const FVector ToAim = AimLocation - RocketStartLocation;

	if (ToAim.IsNearlyZero())
	{
		return 0.0f;
	}

	FVector RocketForward = FVector::ForwardVector;

	if (IsValid(RocketWeaponMount))
	{
		RocketForward = RocketWeaponMount->GetForwardVector();
	}
	else if (const AActor* OwnerActor = GetOwner())
	{
		RocketForward = OwnerActor->GetActorForwardVector();
	}

	const float Dot = FVector::DotProduct(
		RocketForward.GetSafeNormal(),
		ToAim.GetSafeNormal()
	);

	const float ClampedDot = FMath::Clamp(Dot, -1.0f, 1.0f);

	return FMath::RadiansToDegrees(FMath::Acos(ClampedDot));
}

float UDroneCombatControllerComponent::GetRocketHeightErrorToLocation(const FVector& AimLocation) const
{
	const FVector RocketStartLocation = GetAttackTraceStartLocation();

	return AimLocation.Z - RocketStartLocation.Z;
}

bool UDroneCombatControllerComponent::TraceAttackLineToCurrentTarget(FHitResult& OutHit)
{
	const UWorld* World = GetWorld();
	const AActor* OwnerActor = GetOwner();

	if (!World || !OwnerActor || !IsValid(CurrentTarget))
	{
		return false;
	}

	if (!IsValid(RocketWeaponMount))
	{
		RocketWeaponMount = FindWeaponMountByName(RocketWeaponMountComponentName);
	}

	const FVector StartLocation = GetAttackTraceStartLocation();
	const FVector EndLocation = GetPredictedTargetAimLocation();

	FCollisionQueryParams QueryParams(TEXT("DroneCombatAttackLineTrace"), false);
	AddDroneIgnoredActors(QueryParams, false);

	const bool bHit = World->LineTraceSingleByChannel(
		OutHit,
		StartLocation,
		EndLocation,
		AttackLineTraceChannel,
		QueryParams
	);

	bool bClearLine = false;

	if (!bHit)
	{
		bClearLine = true;
	}
	else
	{
		bClearLine = OutHit.GetActor() == CurrentTarget;
	}

	if (bDrawDebugAttackLine)
	{
		DrawDebugLine(
			GetWorld(),
			StartLocation,
			EndLocation,
			bClearLine ? FColor::Green : FColor::Red,
			false,
			DebugDrawDuration,
			0,
			3.0f
		);

		if (!bClearLine && OutHit.bBlockingHit)
		{
			DrawDebugSphere(
				GetWorld(),
				OutHit.ImpactPoint,
				35.0f,
				12,
				FColor::Red,
				false,
				DebugDrawDuration
			);
		}
	}

	return bClearLine;
}

USimWeaponMountComponent* UDroneCombatControllerComponent::FindWeaponMountByName(FName MountComponentName) const
{
	const AActor* OwnerActor = GetOwner();

	if (!OwnerActor || MountComponentName.IsNone())
	{
		return nullptr;
	}

	TArray<USimWeaponMountComponent*> WeaponMounts;
	OwnerActor->GetComponents<USimWeaponMountComponent>(WeaponMounts);

	const FString ExpectedName = MountComponentName.ToString();

	for (USimWeaponMountComponent* WeaponMount : WeaponMounts)
	{
		if (!IsValid(WeaponMount))
		{
			continue;
		}

		if (WeaponMount->GetFName() == MountComponentName)
		{
			return WeaponMount;
		}

		if (WeaponMount->GetName().Equals(ExpectedName) || WeaponMount->GetName().Contains(ExpectedName))
		{
			return WeaponMount;
		}
	}

	return nullptr;
}

FVector UDroneCombatControllerComponent::GetAttackTraceStartLocation() const
{
	if (IsValid(RocketWeaponMount))
	{
		return RocketWeaponMount->GetComponentLocation();
	}

	if (const AActor* OwnerActor = GetOwner())
	{
		return OwnerActor->GetActorLocation();
	}

	return FVector::ZeroVector;
}

FVector UDroneCombatControllerComponent::GetTargetAimLocation() const
{
	if (!IsValid(CurrentTarget))
	{
		return FVector::ZeroVector;
	}

	const FVector AimOffset = FVector(0.0f, 0.0f, TargetAimVerticalOffsetCm);

	if (!bUseTargetBoundsForAim)
	{
		return CurrentTarget->GetActorLocation() + AimOffset;
	}

	FVector TargetOrigin = FVector::ZeroVector;
	FVector TargetExtent = FVector::ZeroVector;

	CurrentTarget->GetActorBounds(false, TargetOrigin, TargetExtent);

	if (TargetExtent.IsNearlyZero())
	{
		return CurrentTarget->GetActorLocation() + AimOffset;
	}

	return TargetOrigin + AimOffset;
}