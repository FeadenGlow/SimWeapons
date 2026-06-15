// Fill out your copyright notice in the Description page of Project Settings.

#include "Combat/DroneCombatControllerComponent.h"

#include "CollisionQueryParams.h"
#include "CollisionShape.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

FVector UDroneCombatControllerComponent::GetDroneBodySafetyBoxHalfExtentWithMargin() const
{
	const FVector PositiveHalfExtent = FVector(
		FMath::Max(1.0f, DroneBodySafetyBoxHalfExtentCm.X),
		FMath::Max(1.0f, DroneBodySafetyBoxHalfExtentCm.Y),
		FMath::Max(1.0f, DroneBodySafetyBoxHalfExtentCm.Z)
	);

	const float PositiveMargin = FMath::Max(0.0f, DroneBodySafetyMarginCm);
	const float PositiveExtraTopHeight = FMath::Max(0.0f, DroneBodySafetyExtraTopHeightCm);

	return FVector(
		PositiveHalfExtent.X + PositiveMargin,
		PositiveHalfExtent.Y + PositiveMargin,
		PositiveHalfExtent.Z + PositiveMargin + PositiveExtraTopHeight * 0.5f
	);
}

FVector UDroneCombatControllerComponent::GetDroneBodySafetyBoxCenterLocation(const FVector& OwnerLocation) const
{
	const float PositiveExtraTopHeight = FMath::Max(0.0f, DroneBodySafetyExtraTopHeightCm);

	return OwnerLocation + FVector::UpVector * (PositiveExtraTopHeight * 0.5f);
}

bool UDroneCombatControllerComponent::IsDroneBodyMoveBlocked(
	const FVector& MoveDelta,
	FHitResult& OutHit
) const
{
	OutHit = FHitResult();

	const UWorld* World = GetWorld();
	const AActor* OwnerActor = GetOwner();

	if (!bUseDroneBodySafetyCollision)
	{
		return false;
	}

	if (!World || !OwnerActor || MoveDelta.IsNearlyZero())
	{
		return false;
	}

	const FVector StartOwnerLocation = OwnerActor->GetActorLocation();
	const FVector EndOwnerLocation = StartOwnerLocation + MoveDelta;

	const FVector StartBoxCenter = GetDroneBodySafetyBoxCenterLocation(StartOwnerLocation);
	const FVector EndBoxCenter = GetDroneBodySafetyBoxCenterLocation(EndOwnerLocation);

	const FQuat BoxRotation = OwnerActor->GetActorQuat();

	FCollisionQueryParams QueryParams(TEXT("DroneBodySafetyMoveTrace"), false);
	AddDroneIgnoredActors(QueryParams, true);

	const FVector BoxHalfExtent = GetDroneBodySafetyBoxHalfExtentWithMargin();

	const bool bBlocked = World->SweepSingleByChannel(
		OutHit,
		StartBoxCenter,
		EndBoxCenter,
		BoxRotation,
		DroneBodySafetyTraceChannel,
		FCollisionShape::MakeBox(BoxHalfExtent),
		QueryParams
	);

	if (bDrawDebugDroneBodySafetyCollision)
	{
		DrawDebugBox(
			GetWorld(),
			StartBoxCenter,
			BoxHalfExtent,
			BoxRotation,
			FColor::Cyan,
			false,
			DebugDrawDuration,
			0,
			1.5f
		);

		DrawDebugBox(
			GetWorld(),
			EndBoxCenter,
			BoxHalfExtent,
			BoxRotation,
			bBlocked ? FColor::Red : FColor::Green,
			false,
			DebugDrawDuration,
			0,
			1.5f
		);

		DrawDebugLine(
			GetWorld(),
			StartBoxCenter,
			EndBoxCenter,
			bBlocked ? FColor::Red : FColor::Green,
			false,
			DebugDrawDuration,
			0,
			2.0f
		);
	}

	return bBlocked && OutHit.bBlockingHit;
}

bool UDroneCombatControllerComponent::IsDroneBodyLocationBlocked(
	const FVector& Location,
	FHitResult& OutHit
) const
{
	OutHit = FHitResult();

	const UWorld* World = GetWorld();
	const AActor* OwnerActor = GetOwner();

	if (!bUseDroneBodySafetyCollision)
	{
		return false;
	}

	if (!World || !OwnerActor)
	{
		return false;
	}

	const FVector BoxCenter = GetDroneBodySafetyBoxCenterLocation(Location);
	const FQuat BoxRotation = OwnerActor->GetActorQuat();

	FCollisionQueryParams QueryParams(TEXT("DroneBodySafetyLocationTrace"), false);
	AddDroneIgnoredActors(QueryParams, true);

	const FVector BoxHalfExtent = GetDroneBodySafetyBoxHalfExtentWithMargin();

	const bool bBlocked = World->SweepSingleByChannel(
		OutHit,
		BoxCenter,
		BoxCenter + FVector::UpVector * 1.0f,
		BoxRotation,
		DroneBodySafetyTraceChannel,
		FCollisionShape::MakeBox(BoxHalfExtent),
		QueryParams
	);

	if (bDrawDebugDroneBodySafetyCollision)
	{
		DrawDebugBox(
			GetWorld(),
			BoxCenter,
			BoxHalfExtent,
			BoxRotation,
			bBlocked ? FColor::Red : FColor::Green,
			false,
			DebugDrawDuration,
			0,
			1.5f
		);
	}

	return bBlocked && OutHit.bBlockingHit;
}

bool UDroneCombatControllerComponent::IsDroneBodyTransformBlocked(
	const FTransform& Transform,
	FHitResult& OutHit
) const
{
	OutHit = FHitResult();

	const UWorld* World = GetWorld();
	const AActor* OwnerActor = GetOwner();

	if (!bUseDroneBodySafetyCollision)
	{
		return false;
	}

	if (!World || !OwnerActor)
	{
		return false;
	}

	FCollisionQueryParams QueryParams(TEXT("DroneBodySafetyTransformTrace"), false);
	AddDroneIgnoredActors(QueryParams, true);

	const FVector BoxHalfExtent = GetDroneBodySafetyBoxHalfExtentWithMargin();

	const float PositiveExtraTopHeight = FMath::Max(0.0f, DroneBodySafetyExtraTopHeightCm);
	const FVector BoxCenter = Transform.GetLocation() + Transform.TransformVector(
		FVector::UpVector * (PositiveExtraTopHeight * 0.5f)
	);

	const FQuat BoxRotation = Transform.GetRotation();

	const bool bBlocked = World->SweepSingleByChannel(
		OutHit,
		BoxCenter,
		BoxCenter + FVector::UpVector * 1.0f,
		BoxRotation,
		DroneBodySafetyTraceChannel,
		FCollisionShape::MakeBox(BoxHalfExtent),
		QueryParams
	);

	if (bDrawDebugDroneBodySafetyCollision)
	{
		DrawDebugBox(
			GetWorld(),
			BoxCenter,
			BoxHalfExtent,
			BoxRotation,
			bBlocked ? FColor::Red : FColor::Green,
			false,
			DebugDrawDuration,
			0,
			1.5f
		);
	}

	return bBlocked && OutHit.bBlockingHit;
}

void UDroneCombatControllerComponent::UpdateLastSafeDroneTransform()
{
	AActor* OwnerActor = GetOwner();

	if (!OwnerActor)
	{
		return;
	}

	if (!bUseDroneBodySafetyCollision)
	{
		LastSafeDroneTransform = OwnerActor->GetActorTransform();
		bHasLastSafeDroneTransform = true;
		return;
	}

	FHitResult LocationHit;
	if (IsDroneBodyLocationBlocked(OwnerActor->GetActorLocation(), LocationHit))
	{
		return;
	}

	LastSafeDroneTransform = OwnerActor->GetActorTransform();
	bHasLastSafeDroneTransform = true;
}

bool UDroneCombatControllerComponent::TryRecoverFromBodyOverlap(float DeltaTime)
{
	if (!bUseCollisionRecovery)
	{
		ClearCollisionRecoveryTarget();
		return false;
	}

	AActor* OwnerActor = GetOwner();

	if (!OwnerActor)
	{
		ClearCollisionRecoveryTarget();
		return false;
	}

	if (bHasCollisionRecoveryTarget)
	{
		SetCombatState(EDroneCombatState::RecoveringFromCollision);
		ContinueSmoothCollisionRecovery(DeltaTime);
		return true;
	}

	FHitResult CurrentLocationHit;
	const bool bCurrentlyBlocked = IsDroneBodyLocationBlocked(
		OwnerActor->GetActorLocation(),
		CurrentLocationHit
	);

	if (!bCurrentlyBlocked)
	{
		ClearCollisionRecoveryTarget();
		UpdateLastSafeDroneTransform();
		return false;
	}

	SetCombatState(EDroneCombatState::RecoveringFromCollision);

	if (bDrawDebugCollisionRecovery && CurrentLocationHit.bBlockingHit)
	{
		DrawDebugSphere(
			GetWorld(),
			CurrentLocationHit.ImpactPoint,
			55.0f,
			16,
			FColor::Red,
			false,
			DebugDrawDuration
		);
	}

	PrintDebugMessage(
		FString::Printf(
			TEXT("Drone body is already inside collision. Starting smooth recovery. Blocked by: %s"),
			CurrentLocationHit.GetActor() ? *CurrentLocationHit.GetActor()->GetName() : TEXT("Unknown")
		)
	);

	if (!StartSmoothCollisionRecovery())
	{
		PrintDebugMessage(TEXT("Smooth collision recovery failed: no safe location found. Drone stays in RecoveringFromCollision."));
		return true;
	}

	ContinueSmoothCollisionRecovery(DeltaTime);
	return true;
}

bool UDroneCombatControllerComponent::FindNearestSafeDroneLocation(FVector& OutSafeLocation) const
{
	OutSafeLocation = FVector::ZeroVector;

	const AActor* OwnerActor = GetOwner();

	if (!OwnerActor)
	{
		return false;
	}

	const FVector OwnerLocation = OwnerActor->GetActorLocation();

	FVector AwayFromTarget = -OwnerActor->GetActorForwardVector();

	if (IsValid(CurrentTarget))
	{
		const FVector FromTargetToOwner = FVector(
			OwnerLocation.X - CurrentTarget->GetActorLocation().X,
			OwnerLocation.Y - CurrentTarget->GetActorLocation().Y,
			0.0f
		);

		if (!FromTargetToOwner.IsNearlyZero())
		{
			AwayFromTarget = FromTargetToOwner.GetSafeNormal();
		}
	}
	else if (HasValidTargetMemory())
	{
		const FVector FromMemoryToOwner = FVector(
			OwnerLocation.X - LastKnownTargetLocation.X,
			OwnerLocation.Y - LastKnownTargetLocation.Y,
			0.0f
		);

		if (!FromMemoryToOwner.IsNearlyZero())
		{
			AwayFromTarget = FromMemoryToOwner.GetSafeNormal();
		}
	}

	const FVector RightDirection = OwnerActor->GetActorRightVector().GetSafeNormal();
	const FVector LeftDirection = -RightDirection;
	const FVector BackDirection = -OwnerActor->GetActorForwardVector().GetSafeNormal();
	const FVector UpDirection = FVector::UpVector;

	const TArray<FVector> BaseDirections =
	{
		UpDirection,
		(UpDirection * UnstuckUpPriorityMultiplier + AwayFromTarget).GetSafeNormal(),
		AwayFromTarget,
		(UpDirection * UnstuckUpPriorityMultiplier + RightDirection).GetSafeNormal(),
		(UpDirection * UnstuckUpPriorityMultiplier + LeftDirection).GetSafeNormal(),
		RightDirection,
		LeftDirection,
		(BackDirection + RightDirection).GetSafeNormal(),
		(BackDirection + LeftDirection).GetSafeNormal(),
		(UpDirection + BackDirection).GetSafeNormal(),
		(UpDirection + RightDirection + AwayFromTarget).GetSafeNormal(),
		(UpDirection + LeftDirection + AwayFromTarget).GetSafeNormal()
	};

	for (int32 StepIndex = 1; StepIndex <= UnstuckSearchSteps; ++StepIndex)
	{
		const float SearchDistance = UnstuckSearchStepCm * static_cast<float>(StepIndex);

		for (const FVector& Direction : BaseDirections)
		{
			if (Direction.IsNearlyZero())
			{
				continue;
			}

			const FVector CandidateLocation = OwnerLocation + Direction.GetSafeNormal() * SearchDistance;

			FHitResult CandidateHit;
			const bool bCandidateBlocked = IsDroneBodyLocationBlocked(CandidateLocation, CandidateHit);

			if (!bCandidateBlocked)
			{
				OutSafeLocation = CandidateLocation;

				if (bDrawDebugCollisionRecovery)
				{
					DrawDebugSphere(
						GetWorld(),
						CandidateLocation,
						45.0f,
						12,
						FColor::Green,
						false,
						DebugDrawDuration
					);
				}

				return true;
			}

			if (bDrawDebugCollisionRecovery)
			{
				DrawDebugSphere(
					GetWorld(),
					CandidateLocation,
					25.0f,
					8,
					FColor::Red,
					false,
					DebugDrawDuration
				);
			}
		}
	}

	if (bHasLastSafeDroneTransform)
	{
		const FVector LastSafeLocation = LastSafeDroneTransform.GetLocation();

		FHitResult LastSafeHit;
		if (!IsDroneBodyLocationBlocked(LastSafeLocation, LastSafeHit))
		{
			OutSafeLocation = LastSafeLocation;

			if (bDrawDebugCollisionRecovery)
			{
				DrawDebugSphere(
					GetWorld(),
					LastSafeLocation,
					65.0f,
					16,
					FColor::Green,
					false,
					DebugDrawDuration
				);
			}

			return true;
		}
	}

	return false;
}

bool UDroneCombatControllerComponent::TrySetDroneRotationSafely(const FRotator& NewRotation)
{
	AActor* OwnerActor = GetOwner();

	if (!OwnerActor)
	{
		return false;
	}

	if (!bUseRotationSafetyCheck || !bUseDroneBodySafetyCollision)
	{
		OwnerActor->SetActorRotation(NewRotation);
		UpdateLastSafeDroneTransform();
		return true;
	}

	const FTransform TestTransform(NewRotation, OwnerActor->GetActorLocation(), OwnerActor->GetActorScale3D());

	FHitResult RotationHit;
	const bool bBlockedAfterRotation = IsDroneBodyTransformBlocked(TestTransform, RotationHit);

	if (bBlockedAfterRotation)
	{
		if (bDrawDebugCollisionRecovery && RotationHit.bBlockingHit)
		{
			DrawDebugSphere(
				GetWorld(),
				RotationHit.ImpactPoint,
				40.0f,
				12,
				FColor::Red,
				false,
				DebugDrawDuration
			);
		}

		PrintDebugMessage(
			FString::Printf(
				TEXT("Drone rotation blocked by body safety collision: %s"),
				RotationHit.GetActor() ? *RotationHit.GetActor()->GetName() : TEXT("Unknown")
			)
		);

		return false;
	}

	OwnerActor->SetActorRotation(NewRotation);
	UpdateLastSafeDroneTransform();
	return true;
}

void UDroneCombatControllerComponent::ClearCollisionRecoveryTarget()
{
	CollisionRecoveryTargetLocation = FVector::ZeroVector;
	bHasCollisionRecoveryTarget = false;
}

bool UDroneCombatControllerComponent::StartSmoothCollisionRecovery()
{
	if (!bUseSmoothCollisionRecovery)
	{
		ClearCollisionRecoveryTarget();
		return false;
	}

	FVector SafeLocation = FVector::ZeroVector;

	if (!FindNearestSafeDroneLocation(SafeLocation))
	{
		ClearCollisionRecoveryTarget();
		return false;
	}

	CollisionRecoveryTargetLocation = SafeLocation;
	bHasCollisionRecoveryTarget = true;

	if (bDrawDebugCollisionRecovery)
	{
		if (const AActor* OwnerActor = GetOwner())
		{
			DrawDebugLine(
				GetWorld(),
				OwnerActor->GetActorLocation(),
				CollisionRecoveryTargetLocation,
				FColor::Green,
				false,
				DebugDrawDuration,
				0,
				4.0f
			);
		}

		DrawDebugSphere(
			GetWorld(),
			CollisionRecoveryTargetLocation,
			85.0f,
			20,
			FColor::Green,
			false,
			DebugDrawDuration
		);
	}

	PrintDebugMessage(
		FString::Printf(
			TEXT("Smooth collision recovery target selected: %s"),
			*CollisionRecoveryTargetLocation.ToString()
		)
	);

	return true;
}

bool UDroneCombatControllerComponent::ContinueSmoothCollisionRecovery(float DeltaTime)
{
	if (!bHasCollisionRecoveryTarget)
	{
		return false;
	}

	AActor* OwnerActor = GetOwner();

	if (!OwnerActor || DeltaTime <= 0.0f)
	{
		return false;
	}

	FHitResult TargetHit;
	if (IsDroneBodyLocationBlocked(CollisionRecoveryTargetLocation, TargetHit))
	{
		PrintDebugMessage(TEXT("Smooth collision recovery target became blocked. Searching for a new target."));

		ClearCollisionRecoveryTarget();

		if (!StartSmoothCollisionRecovery())
		{
			return false;
		}
	}

	const FVector OwnerLocation = OwnerActor->GetActorLocation();
	const FVector ToRecoveryTarget = CollisionRecoveryTargetLocation - OwnerLocation;
	const float DistanceToTarget = ToRecoveryTarget.Size();

	if (DistanceToTarget <= CollisionRecoveryAcceptanceRadiusCm)
	{
		FHitResult CurrentHit;
		const bool bStillBlocked = IsDroneBodyLocationBlocked(OwnerLocation, CurrentHit);

		if (!bStillBlocked)
		{
			ClearCollisionRecoveryTarget();
			UpdateLastSafeDroneTransform();

			PrintDebugMessage(TEXT("Smooth collision recovery finished. Drone is in a safe location."));
			return true;
		}

		ClearCollisionRecoveryTarget();

		if (!StartSmoothCollisionRecovery())
		{
			PrintDebugMessage(TEXT("Smooth collision recovery reached target, but drone is still blocked and no new target was found."));
			return false;
		}

		return true;
	}

	const FVector RecoveryDirection = ToRecoveryTarget.GetSafeNormal();

	const float PositiveRecoverySpeed = FMath::Max(1.0f, CollisionRecoveryMoveSpeedCmPerSecond);
	const float StepDistance = FMath::Min(
		PositiveRecoverySpeed * DeltaTime,
		DistanceToTarget
	);

	const FVector NewLocation = OwnerLocation + RecoveryDirection * StepDistance;

	const bool bMoved = OwnerActor->SetActorLocation(
		NewLocation,
		false,
		nullptr,
		ETeleportType::None
	);

	if (!bMoved)
	{
		PrintDebugMessage(TEXT("Smooth collision recovery movement failed."));
		return false;
	}

	if (bDrawDebugCollisionRecovery)
	{
		DrawDebugLine(
			GetWorld(),
			OwnerLocation,
			NewLocation,
			FColor::Green,
			false,
			DebugDrawDuration,
			0,
			3.0f
		);

		DrawDebugSphere(
			GetWorld(),
			NewLocation,
			35.0f,
			12,
			FColor::Green,
			false,
			DebugDrawDuration
		);
	}

	FHitResult NewLocationHit;
	const bool bStillBlockedAfterStep = IsDroneBodyLocationBlocked(NewLocation, NewLocationHit);

	if (!bStillBlockedAfterStep)
	{
		ClearCollisionRecoveryTarget();
		UpdateLastSafeDroneTransform();

		PrintDebugMessage(TEXT("Smooth collision recovery finished before reaching target. Drone is safe now."));
	}

	return true;
}