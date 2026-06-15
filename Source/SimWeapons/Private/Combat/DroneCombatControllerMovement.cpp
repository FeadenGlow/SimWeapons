// Fill out your copyright notice in the Description page of Project Settings.

#include "Combat/DroneCombatControllerComponent.h"

#include "Carriers/Drone/SimDroneCarrier.h"
#include "CollisionQueryParams.h"
#include "CollisionShape.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

void UDroneCombatControllerComponent::MoveTowardCurrentTarget(float DeltaTime)
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

	if (!DroneCarrier->IsAlive())
	{
		SetCurrentTarget(nullptr);
		ClearTargetMemory();
		SetCombatState(EDroneCombatState::Disabled);
		return;
	}

	AActor* OwnerActor = GetOwner();

	if (!OwnerActor)
	{
		SetCombatState(EDroneCombatState::Disabled);
		return;
	}

	if (TryRecoverFromBodyOverlap(DeltaTime))
	{
		return;
	}

	FVector NavigationTargetLocation = FVector::ZeroVector;
	bool bUsingTargetMemory = false;

	if (!GetCurrentNavigationTargetLocation(NavigationTargetLocation, bUsingTargetMemory))
	{
		if (
			CombatState == EDroneCombatState::TargetFound ||
			CombatState == EDroneCombatState::LostSight ||
			CombatState == EDroneCombatState::InvestigatingLastKnownPosition ||
			CombatState == EDroneCombatState::Approaching ||
			CombatState == EDroneCombatState::AvoidingObstacle ||
			CombatState == EDroneCombatState::InAttackRange ||
			CombatState == EDroneCombatState::AttackLineBlocked ||
			CombatState == EDroneCombatState::PositioningForAttack ||
			CombatState == EDroneCombatState::Aiming ||
			CombatState == EDroneCombatState::ReadyToFire ||
			CombatState == EDroneCombatState::RecoveringFromCollision
		)
		{
			SetCurrentTarget(nullptr);
			ClearTargetMemory();
			SetCombatState(EDroneCombatState::Idle);
		}

		ApplyStationaryAltitudeControl(DeltaTime);
		return;
	}

	const FVector OwnerLocation = OwnerActor->GetActorLocation();

	const FVector HorizontalToTarget = FVector(
		NavigationTargetLocation.X - OwnerLocation.X,
		NavigationTargetLocation.Y - OwnerLocation.Y,
		0.0f
	);

	const float HorizontalDistance = HorizontalToTarget.Size();

	if (HorizontalDistance <= KINDA_SMALL_NUMBER)
	{
		SetCombatState(
			bUsingTargetMemory
				? EDroneCombatState::InvestigatingLastKnownPosition
				: EDroneCombatState::InAttackRange
		);

		ApplyStationaryAltitudeControl(DeltaTime);
		return;
	}

	const FVector DesiredMoveDirection = HorizontalToTarget.GetSafeNormal();

	const float DesiredYaw = DesiredMoveDirection.Rotation().Yaw;
	const FRotator CurrentRotation = OwnerActor->GetActorRotation();

	const FRotator DesiredRotation = bKeepDroneLevelWhileRotating
		? FRotator(0.0f, DesiredYaw, 0.0f)
		: DesiredMoveDirection.Rotation();

	const FRotator NewRotation = FMath::RInterpTo(
		CurrentRotation,
		DesiredRotation,
		DeltaTime,
		RotationInterpSpeed
	);

	TrySetDroneRotationSafely(NewRotation);

	if (bUsingTargetMemory)
	{
		if (HorizontalDistance <= LastKnownPositionAcceptanceRadiusCm)
		{
			SetCombatState(EDroneCombatState::InvestigatingLastKnownPosition);

			if (bDrawDebugLastKnownTargetLocation)
			{
				DrawDebugSphere(
					GetWorld(),
					LastKnownTargetLocation,
					LastKnownPositionAcceptanceRadiusCm,
					24,
					FColor::Magenta,
					false,
					DebugDrawDuration
				);
			}

			ApplyStationaryAltitudeControl(DeltaTime);
			return;
		}
	}
	else
	{
		const float DistanceToAttackRange = HorizontalDistance - AttackRangeCm;

		if (DistanceToAttackRange <= MoveAcceptanceToleranceCm)
		{
			ApplyStationaryAltitudeControl(DeltaTime);

			FHitResult AttackLineHit;
			const bool bHasClearAttackLine = TraceAttackLineToCurrentTarget(AttackLineHit);

			if (bHasClearAttackLine)
			{
				SetCombatState(EDroneCombatState::InAttackRange);
			}
			else
			{
				SetCombatState(EDroneCombatState::AttackLineBlocked);

				PrintDebugMessage(
					FString::Printf(
						TEXT("Attack line blocked by: %s"),
						AttackLineHit.GetActor() ? *AttackLineHit.GetActor()->GetName() : TEXT("Unknown")
					)
				);
			}

			return;
		}
	}

	FVector MoveDirection = DesiredMoveDirection;
	bool bAvoidingObstacle = false;

	if (bUseObstacleAvoidance)
	{
		bAvoidingObstacle = CalculateAvoidanceMoveDirection(DesiredMoveDirection, MoveDirection);
	}

	const float DesiredMoveDistance = ApproachSpeedCmPerSecond * DeltaTime;

	float RemainingMoveDistance = HorizontalDistance;

	if (!bUsingTargetMemory)
	{
		RemainingMoveDistance = FMath::Max(0.0f, HorizontalDistance - AttackRangeCm);
	}

	const float MoveDistance = FMath::Min(DesiredMoveDistance, RemainingMoveDistance);

	if (MoveDistance <= KINDA_SMALL_NUMBER)
	{
		SetCombatState(
			bUsingTargetMemory
				? EDroneCombatState::InvestigatingLastKnownPosition
				: EDroneCombatState::InAttackRange
		);

		ApplyStationaryAltitudeControl(DeltaTime);
		return;
	}

	const FVector HorizontalMoveDelta = MoveDirection * MoveDistance;
	const FVector FullMoveDelta = ApplyAltitudeCorrectionToMove(HorizontalMoveDelta, DeltaTime);

	if (FullMoveDelta.IsNearlyZero())
	{
		ApplyStationaryAltitudeControl(DeltaTime);
		return;
	}

	FHitResult BodyMoveHit;
	if (IsDroneBodyMoveBlocked(FullMoveDelta, BodyMoveHit))
	{
		PrintDebugMessage(
			FString::Printf(
				TEXT("Drone body movement blocked by: %s"),
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

		FHitResult HorizontalOnlyHit;
		const bool bHorizontalOnlyBlocked = IsDroneBodyMoveBlocked(HorizontalMoveDelta, HorizontalOnlyHit);

		if (!HorizontalMoveDelta.IsNearlyZero() && !bHorizontalOnlyBlocked)
		{
			if (TryApplyMovementMoveDelta(HorizontalMoveDelta, DeltaTime, FColor::Orange))
			{
				SetCombatState(EDroneCombatState::AvoidingObstacle);
				return;
			}
		}

		FVector RecoveryMoveDelta = FVector::ZeroVector;

		if (BuildBlockedMovementRecoveryMoveDelta(
			DesiredMoveDirection,
			HorizontalMoveDelta,
			BodyMoveHit,
			DeltaTime,
			RecoveryMoveDelta
		))
		{
			if (TryApplyMovementMoveDelta(RecoveryMoveDelta, DeltaTime, FColor::Orange))
			{
				SetCombatState(EDroneCombatState::AvoidingObstacle);
				return;
			}
		}

		SetCombatState(EDroneCombatState::AvoidingObstacle);
		return;
	}

	if (!TryApplyMovementMoveDelta(
		FullMoveDelta,
		DeltaTime,
		bAvoidingObstacle ? FColor::Orange : (bUsingTargetMemory ? FColor::Magenta : FColor::Yellow)
	))
	{
		SetCombatState(EDroneCombatState::AvoidingObstacle);
		return;
	}

	if (bAvoidingObstacle)
	{
		SetCombatState(EDroneCombatState::AvoidingObstacle);
	}
	else if (bUsingTargetMemory)
	{
		SetCombatState(EDroneCombatState::InvestigatingLastKnownPosition);
	}
	else
	{
		SetCombatState(EDroneCombatState::Approaching);
	}
}

bool UDroneCombatControllerComponent::CalculateAvoidanceMoveDirection(
	const FVector& DesiredMoveDirection,
	FVector& OutMoveDirection
) const
{
	OutMoveDirection = DesiredMoveDirection;

	const AActor* OwnerActor = GetOwner();

	if (!OwnerActor || DesiredMoveDirection.IsNearlyZero())
	{
		return false;
	}

	const FVector StartLocation = OwnerActor->GetActorLocation();

	FHitResult ForwardHit;
	const bool bForwardBlocked = IsMoveDirectionBlocked(
		StartLocation,
		DesiredMoveDirection,
		ObstacleCheckDistanceCm,
		ForwardHit
	);

	if (!bForwardBlocked)
	{
		return false;
	}

	if (bDrawDebugObstacleTrace)
	{
		DrawDebugSphere(
			GetWorld(),
			ForwardHit.ImpactPoint,
			ObstacleSphereRadiusCm,
			16,
			FColor::Red,
			false,
			DebugDrawDuration
		);
	}

	const FVector HorizontalDirection = FVector(
		DesiredMoveDirection.X,
		DesiredMoveDirection.Y,
		0.0f
	).GetSafeNormal();

	if (HorizontalDirection.IsNearlyZero())
	{
		OutMoveDirection = FVector::UpVector;
		return true;
	}

	const FVector RightDirection = FVector::CrossProduct(
		FVector::UpVector,
		HorizontalDirection
	).GetSafeNormal();

	const FVector LeftDirection = -RightDirection;

	const TArray<FVector> CandidateDirections =
	{
		(HorizontalDirection + RightDirection * ObstacleSideStepStrength + FVector::UpVector * ObstacleUpStepStrength).GetSafeNormal(),
		(HorizontalDirection + LeftDirection * ObstacleSideStepStrength + FVector::UpVector * ObstacleUpStepStrength).GetSafeNormal(),
		(HorizontalDirection + FVector::UpVector * (ObstacleUpStepStrength * 1.5f)).GetSafeNormal(),
		(FVector::VectorPlaneProject(HorizontalDirection, ForwardHit.ImpactNormal) + FVector::UpVector * ObstacleUpStepStrength).GetSafeNormal()
	};

	for (const FVector& CandidateDirection : CandidateDirections)
	{
		if (CandidateDirection.IsNearlyZero())
		{
			continue;
		}

		FHitResult CandidateHit;
		const bool bCandidateBlocked = IsMoveDirectionBlocked(
			StartLocation,
			CandidateDirection,
			ObstacleCheckDistanceCm,
			CandidateHit
		);

		if (!bCandidateBlocked)
		{
			OutMoveDirection = CandidateDirection;

			if (bDrawDebugObstacleTrace)
			{
				DrawDebugLine(
					GetWorld(),
					StartLocation,
					StartLocation + CandidateDirection * ObstacleCheckDistanceCm,
					FColor::Orange,
					false,
					DebugDrawDuration,
					0,
					2.0f
				);
			}

			return true;
		}
	}

	OutMoveDirection = (HorizontalDirection + FVector::UpVector * ObstacleUpStepStrength).GetSafeNormal();

	return true;
}

bool UDroneCombatControllerComponent::IsMoveDirectionBlocked(
	const FVector& StartLocation,
	const FVector& Direction,
	float Distance,
	FHitResult& OutHit
) const
{
	const UWorld* World = GetWorld();
	const AActor* OwnerActor = GetOwner();

	if (!World || !OwnerActor || Direction.IsNearlyZero() || Distance <= 0.0f)
	{
		return false;
	}

	const FVector MoveDelta = Direction.GetSafeNormal() * Distance;

	if (bUseDroneBodySafetyCollision)
	{
		return IsDroneBodyMoveBlocked(MoveDelta, OutHit);
	}

	const FVector EndLocation = StartLocation + MoveDelta;

	FCollisionQueryParams QueryParams(TEXT("DroneCombatMovementObstacleTrace"), false);
	AddDroneIgnoredActors(QueryParams, true);

	const bool bBlocked = World->SweepSingleByChannel(
		OutHit,
		StartLocation,
		EndLocation,
		FQuat::Identity,
		MovementObstacleTraceChannel,
		FCollisionShape::MakeSphere(ObstacleSphereRadiusCm),
		QueryParams
	);

	if (bDrawDebugObstacleTrace)
	{
		DrawDebugLine(
			GetWorld(),
			StartLocation,
			EndLocation,
			bBlocked ? FColor::Red : FColor::Blue,
			false,
			DebugDrawDuration,
			0,
			1.5f
		);
	}

	return bBlocked && OutHit.bBlockingHit;
}

bool UDroneCombatControllerComponent::TryApplyMovementMoveDelta(
	const FVector& MoveDelta,
	float DeltaTime,
	const FColor& DebugColor
)
{
	if (MoveDelta.IsNearlyZero() || DeltaTime <= 0.0f)
	{
		return false;
	}

	AActor* OwnerActor = GetOwner();

	if (!OwnerActor || !IsValid(DroneCarrier))
	{
		return false;
	}

	FHitResult BodyMoveHit;
	if (IsDroneBodyMoveBlocked(MoveDelta, BodyMoveHit))
	{
		PrintDebugMessage(
			FString::Printf(
				TEXT("Movement fallback blocked by body safety: %s"),
				BodyMoveHit.GetActor() ? *BodyMoveHit.GetActor()->GetName() : TEXT("Unknown")
			)
		);

		return false;
	}

	const FVector OwnerLocation = OwnerActor->GetActorLocation();
	const FVector NewLocation = OwnerLocation + MoveDelta;

	FHitResult EndLocationHit;
	if (IsDroneBodyLocationBlocked(NewLocation, EndLocationHit))
	{
		PrintDebugMessage(
			FString::Printf(
				TEXT("Movement fallback target location blocked by: %s"),
				EndLocationHit.GetActor() ? *EndLocationHit.GetActor()->GetName() : TEXT("Unknown")
			)
		);

		return false;
	}

	const float ActualSpeedCmPerSecond = MoveDelta.Size() / DeltaTime;
	const float MovementEnergyCost = CalculateMovementEnergyCost(ActualSpeedCmPerSecond, DeltaTime);

	if (MovementEnergyCost > 0.0f && !DroneCarrier->ConsumeEnergy(MovementEnergyCost))
	{
		SetCurrentTarget(nullptr);
		ClearTargetMemory();
		SetCombatState(EDroneCombatState::Disabled);
		PrintDebugMessage(TEXT("Drone has no energy for movement."));
		return false;
	}

	FHitResult SweepHit;
	const bool bMoved = OwnerActor->SetActorLocation(
		NewLocation,
		bUseSweepMovement,
		&SweepHit,
		ETeleportType::None
	);

	if (bUseSweepMovement && SweepHit.bBlockingHit)
	{
		PrintDebugMessage(
			FString::Printf(
				TEXT("Drone movement blocked by root sweep: %s"),
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

		return false;
	}

	if (!bMoved)
	{
		PrintDebugMessage(TEXT("Drone movement failed."));
		return false;
	}

	UpdateLastSafeDroneTransform();

	if (bDrawDebugMovementLine)
	{
		DrawDebugLine(
			GetWorld(),
			OwnerLocation,
			NewLocation,
			DebugColor,
			false,
			DebugDrawDuration,
			0,
			2.0f
		);
	}

	return true;
}

bool UDroneCombatControllerComponent::BuildBlockedMovementRecoveryMoveDelta(
	const FVector& DesiredMoveDirection,
	const FVector& HorizontalMoveDelta,
	const FHitResult& BlockHit,
	float DeltaTime,
	FVector& OutMoveDelta
) const
{
	OutMoveDelta = FVector::ZeroVector;

	const AActor* OwnerActor = GetOwner();

	if (!OwnerActor || DeltaTime <= 0.0f)
	{
		return false;
	}

	const FVector OwnerLocation = OwnerActor->GetActorLocation();

	FVector HorizontalDesiredDirection = FVector(
		DesiredMoveDirection.X,
		DesiredMoveDirection.Y,
		0.0f
	).GetSafeNormal();

	if (HorizontalDesiredDirection.IsNearlyZero())
	{
		HorizontalDesiredDirection = OwnerActor->GetActorForwardVector();
	}

	FVector HorizontalBlockNormal = FVector(
		BlockHit.ImpactNormal.X,
		BlockHit.ImpactNormal.Y,
		0.0f
	).GetSafeNormal();

	if (HorizontalBlockNormal.IsNearlyZero())
	{
		HorizontalBlockNormal = -HorizontalDesiredDirection;
	}

	const FVector TangentA = FVector::CrossProduct(FVector::UpVector, HorizontalBlockNormal).GetSafeNormal();
	const FVector TangentB = -TangentA;

	const float TangentADot = FVector::DotProduct(TangentA, HorizontalDesiredDirection);
	const float TangentBDot = FVector::DotProduct(TangentB, HorizontalDesiredDirection);

	const FVector PreferredTangent = TangentADot >= TangentBDot ? TangentA : TangentB;
	const FVector OtherTangent = TangentADot >= TangentBDot ? TangentB : TangentA;

	const FVector AwayFromObstacle = HorizontalBlockNormal;
	const FVector BackFromGoal = -HorizontalDesiredDirection;

	float MoveDistance = HorizontalMoveDelta.Size();

	if (MoveDistance <= KINDA_SMALL_NUMBER)
	{
		MoveDistance = ApproachSpeedCmPerSecond * DeltaTime;
	}

	MoveDistance = FMath::Max(10.0f, MoveDistance);

	const TArray<FVector> CandidateDirections =
	{
		PreferredTangent,
		(PreferredTangent + FVector::UpVector * 0.25f).GetSafeNormal(),
		(PreferredTangent + AwayFromObstacle * 0.35f).GetSafeNormal(),
		(PreferredTangent + AwayFromObstacle * 0.35f + FVector::UpVector * 0.25f).GetSafeNormal(),

		OtherTangent,
		(OtherTangent + FVector::UpVector * 0.25f).GetSafeNormal(),
		(OtherTangent + AwayFromObstacle * 0.35f).GetSafeNormal(),
		(OtherTangent + AwayFromObstacle * 0.35f + FVector::UpVector * 0.25f).GetSafeNormal(),

		(AwayFromObstacle + FVector::UpVector * 0.35f).GetSafeNormal(),
		(BackFromGoal + PreferredTangent * 0.5f + FVector::UpVector * 0.25f).GetSafeNormal(),
		(BackFromGoal + OtherTangent * 0.5f + FVector::UpVector * 0.25f).GetSafeNormal(),
		FVector::UpVector
	};

	FVector NavigationTargetLocation = FVector::ZeroVector;
	bool bUsingTargetMemory = false;
	const bool bHasNavigationTarget = GetCurrentNavigationTargetLocation(
		NavigationTargetLocation,
		bUsingTargetMemory
	);

	float BestScore = -FLT_MAX;
	FVector BestMoveDelta = FVector::ZeroVector;

	for (const FVector& CandidateDirection : CandidateDirections)
	{
		if (CandidateDirection.IsNearlyZero())
		{
			continue;
		}

		const FVector CandidateMoveDelta = CandidateDirection.GetSafeNormal() * MoveDistance;
		const FVector CandidateLocation = OwnerLocation + CandidateMoveDelta;

		FHitResult CandidateMoveHit;
		if (IsDroneBodyMoveBlocked(CandidateMoveDelta, CandidateMoveHit))
		{
			continue;
		}

		FHitResult CandidateLocationHit;
		if (IsDroneBodyLocationBlocked(CandidateLocation, CandidateLocationHit))
		{
			continue;
		}

		float Score = 0.0f;

		Score += FVector::DotProduct(
			FVector(CandidateDirection.X, CandidateDirection.Y, 0.0f).GetSafeNormal(),
			HorizontalDesiredDirection
		) * 2.0f;

		Score += FVector::DotProduct(
			FVector(CandidateDirection.X, CandidateDirection.Y, 0.0f).GetSafeNormal(),
			PreferredTangent
		) * 1.0f;

		Score += FVector::DotProduct(
			FVector(CandidateDirection.X, CandidateDirection.Y, 0.0f).GetSafeNormal(),
			AwayFromObstacle
		) * 0.7f;

		Score += CandidateDirection.Z * 0.35f;

		if (bHasNavigationTarget)
		{
			const float CurrentDistanceToTarget = FVector::Dist2D(
				OwnerLocation,
				NavigationTargetLocation
			);

			const float CandidateDistanceToTarget = FVector::Dist2D(
				CandidateLocation,
				NavigationTargetLocation
			);

			Score += (CurrentDistanceToTarget - CandidateDistanceToTarget) * 0.01f;
		}

		if (Score > BestScore)
		{
			BestScore = Score;
			BestMoveDelta = CandidateMoveDelta;
		}

		if (bDrawDebugObstacleTrace)
		{
			DrawDebugLine(
				GetWorld(),
				OwnerLocation,
				CandidateLocation,
				FColor::Orange,
				false,
				DebugDrawDuration,
				0,
				1.5f
			);
		}
	}

	if (BestMoveDelta.IsNearlyZero())
	{
		return false;
	}

	OutMoveDelta = BestMoveDelta;
	return true;
}

float UDroneCombatControllerComponent::CalculateMovementEnergyCost(
	float SpeedCmPerSecond,
	float DeltaTime
) const
{
	if (!IsValid(DroneCarrier) || DeltaTime <= 0.0f)
	{
		return 0.0f;
	}

	const float MovementCostPer100CmPerSecond = DroneCarrier->GetMovementCostPer100CmPerSecond();

	return FMath::Max(
		0.0f,
		(SpeedCmPerSecond / 100.0f) * MovementCostPer100CmPerSecond * DeltaTime
	);
}