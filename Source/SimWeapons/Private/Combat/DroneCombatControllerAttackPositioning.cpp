// Fill out your copyright notice in the Description page of Project Settings.

#include "Combat/DroneCombatControllerComponent.h"

#include "Carriers/Drone/SimDroneCarrier.h"
#include "Carriers/SimWeaponMountComponent.h"
#include "CollisionQueryParams.h"
#include "CollisionShape.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

bool UDroneCombatControllerComponent::ApplyAttackPositioningMove(
	const FVector& AimLocation,
	float DeltaTime
)
{
	if (DeltaTime <= 0.0f)
	{
		return false;
	}

	AActor* OwnerActor = GetOwner();

	if (!OwnerActor || !IsValid(DroneCarrier))
	{
		ClearCurrentAttackSlot();
		return false;
	}

	if (TryRecoverFromBodyOverlap(DeltaTime))
	{
		ClearCurrentAttackSlot();
		return false;
	}

	ClearExpiredBadAttackSlots();

	if (!bUseAttackSlotRepositioning)
	{
		const FVector OwnerLocation = OwnerActor->GetActorLocation();
		const FVector RocketStartLocation = GetAttackTraceStartLocation();

		const FVector HorizontalToAim = FVector(
			AimLocation.X - OwnerLocation.X,
			AimLocation.Y - OwnerLocation.Y,
			0.0f
		);

		const float HorizontalDistance = HorizontalToAim.Size();
		FVector MoveDelta = FVector::ZeroVector;
		bool bDistanceReady = true;

		if (HorizontalDistance > KINDA_SMALL_NUMBER)
		{
			const FVector DirectionToAim = HorizontalToAim.GetSafeNormal();

			if (HorizontalDistance < MinimumSafeAttackDistanceCm)
			{
				const float DesiredMoveDistance = FMath::Min(
					ApproachSpeedCmPerSecond * DeltaTime,
					PreferredAttackDistanceCm - HorizontalDistance
				);

				MoveDelta += -DirectionToAim * FMath::Max(0.0f, DesiredMoveDistance);
				bDistanceReady = false;
			}
			else if (HorizontalDistance > PreferredAttackDistanceCm + AttackPositionDistanceToleranceCm)
			{
				const float DesiredMoveDistance = FMath::Min(
					ApproachSpeedCmPerSecond * DeltaTime,
					HorizontalDistance - PreferredAttackDistanceCm
				);

				MoveDelta += DirectionToAim * FMath::Max(0.0f, DesiredMoveDistance);
				bDistanceReady = false;
			}
			else if (HorizontalDistance < PreferredAttackDistanceCm - AttackPositionDistanceToleranceCm)
			{
				const float DesiredMoveDistance = FMath::Min(
					ApproachSpeedCmPerSecond * DeltaTime,
					PreferredAttackDistanceCm - HorizontalDistance
				);

				MoveDelta += -DirectionToAim * FMath::Max(0.0f, DesiredMoveDistance);
				bDistanceReady = false;
			}
		}

		const float HeightError = AimLocation.Z - RocketStartLocation.Z;
		const bool bHeightReady = FMath::Abs(HeightError) <= AimHeightToleranceCm;

		if (!bHeightReady)
		{
			const float DesiredCorrectionZ = HeightError * AttackAltitudeInterpSpeed * DeltaTime;
			const float MaxCorrectionZ = MaxAttackAltitudeCorrectionSpeedCmPerSecond * DeltaTime;

			MoveDelta.Z += FMath::Clamp(
				DesiredCorrectionZ,
				-MaxCorrectionZ,
				MaxCorrectionZ
			);
		}

		const bool bPositionReady = bDistanceReady && bHeightReady;

		if (MoveDelta.IsNearlyZero())
		{
			UpdateLastSafeDroneTransform();
			return bPositionReady;
		}

		FHitResult MoveHit;
		if (IsAttackMoveBlocked(MoveDelta, MoveHit))
		{
			PrintDebugMessage(
				FString::Printf(
					TEXT("Attack positioning direct move blocked by: %s"),
					MoveHit.GetActor() ? *MoveHit.GetActor()->GetName() : TEXT("Unknown")
				)
			);

			return false;
		}

		return TryApplyAttackMoveDelta(MoveDelta, DeltaTime, FColor::Purple) && bPositionReady;
	}

	FHitResult CurrentSlotValidationHit;
	if (
		bHasCurrentAttackSlot &&
		(
			IsAttackSlotNearBadLocation(CurrentAttackSlotLocation) ||
			!IsAttackSlotLocationValid(
				CurrentAttackSlotLocation,
				AimLocation,
				true,
				bRequireAttackLineForAttackSlot,
				CurrentSlotValidationHit
			)
		)
	)
	{
		PrintDebugMessage(TEXT("Current attack slot became invalid. Selecting another slot."));
		ClearCurrentAttackSlot();
	}

	if (!bHasCurrentAttackSlot)
	{
		FVector NewAttackSlotLocation = FVector::ZeroVector;

		if (!FindBestAttackSlot(AimLocation, NewAttackSlotLocation))
		{
			PrintDebugMessage(TEXT("No valid attack slot found."));
			return false;
		}

		CurrentAttackSlotLocation = NewAttackSlotLocation;
		bHasCurrentAttackSlot = true;
		RepeatedAttackPositioningBlockCount = 0;
		ResetAttackSlotProgressTracking();

		PrintDebugMessage(
			FString::Printf(
				TEXT("New attack slot selected: %s"),
				*CurrentAttackSlotLocation.ToString()
			)
		);
	}

	const FVector OwnerLocation = OwnerActor->GetActorLocation();
	const FVector HorizontalToAttackSlot = FVector(
		CurrentAttackSlotLocation.X - OwnerLocation.X,
		CurrentAttackSlotLocation.Y - OwnerLocation.Y,
		0.0f
	);

	const float HorizontalDistanceToAttackSlot = HorizontalToAttackSlot.Size();
	const float HorizontalAcceptanceRadius = FMath::Max(
		10.0f,
		bUseTwoPhaseAttackSlotMovement ? AttackSlotHorizontalAcceptanceRadiusCm : AttackSlotAcceptanceRadiusCm
	);

	if (HorizontalDistanceToAttackSlot > HorizontalAcceptanceRadius)
	{
		if (HasAttackSlotMovementTimedOut(HorizontalDistanceToAttackSlot))
		{
			PrintDebugMessage(TEXT("Attack slot movement made no progress. Marking slot as bad."));
			RememberBadAttackSlot(CurrentAttackSlotLocation);
			ClearCurrentAttackSlot();
			return false;
		}

		const float MoveDistance = FMath::Min(
			ApproachSpeedCmPerSecond * DeltaTime,
			HorizontalDistanceToAttackSlot
		);

		FVector MoveDelta = HorizontalToAttackSlot.GetSafeNormal() * MoveDistance;

		FHitResult SlotMoveHit;
		if (IsAttackMoveBlocked(MoveDelta, SlotMoveHit))
		{
			PrintDebugMessage(
				FString::Printf(
					TEXT("Attack slot horizontal movement blocked by: %s"),
					SlotMoveHit.GetActor() ? *SlotMoveHit.GetActor()->GetName() : TEXT("Unknown")
				)
			);

			FVector FallbackMoveDelta = FVector::ZeroVector;

			if (BuildAttackObstacleAwareFallbackMoveDelta(
				AimLocation,
				MoveDelta,
				SlotMoveHit,
				DeltaTime,
				FallbackMoveDelta
			))
			{
				FallbackMoveDelta.Z = FMath::Max(0.0f, FallbackMoveDelta.Z);

				if (!FallbackMoveDelta.IsNearlyZero() && TryApplyAttackMoveDelta(FallbackMoveDelta, DeltaTime, FColor::Orange))
				{
					const FVector NewOwnerLocation = OwnerLocation + FallbackMoveDelta;
					const float NewHorizontalDistance = FVector::Dist2D(NewOwnerLocation, CurrentAttackSlotLocation);
					HasAttackSlotMovementTimedOut(NewHorizontalDistance);
					return false;
				}
			}

			if (HasAttackSlotMovementTimedOut(HorizontalDistanceToAttackSlot))
			{
				PrintDebugMessage(TEXT("Attack slot horizontal path stayed blocked too long. Marking slot as bad."));
				RememberBadAttackSlot(CurrentAttackSlotLocation);
				ClearCurrentAttackSlot();
			}

			return false;
		}

		if (!TryApplyAttackMoveDelta(MoveDelta, DeltaTime, FColor::Purple))
		{
			PrintDebugMessage(TEXT("Attack slot horizontal movement failed."));

			if (HasAttackSlotMovementTimedOut(HorizontalDistanceToAttackSlot))
			{
				RememberBadAttackSlot(CurrentAttackSlotLocation);
				ClearCurrentAttackSlot();
			}

			return false;
		}

		const FVector NewOwnerLocation = OwnerLocation + MoveDelta;
		const float NewHorizontalDistance = FVector::Dist2D(NewOwnerLocation, CurrentAttackSlotLocation);
		HasAttackSlotMovementTimedOut(NewHorizontalDistance);
		RepeatedAttackPositioningBlockCount = 0;
		return false;
	}

	ResetAttackSlotProgressTracking();

	const FVector RocketStartLocation = GetAttackTraceStartLocation();
	const float RocketHeightError = AimLocation.Z - RocketStartLocation.Z;
	const bool bRocketHeightReady = FMath::Abs(RocketHeightError) <= AimHeightToleranceCm;

	if (!bRocketHeightReady)
	{
		const float DesiredCorrectionZ = RocketHeightError * AttackAltitudeInterpSpeed * DeltaTime;
		const float MaxCorrectionZ = MaxAttackAltitudeCorrectionSpeedCmPerSecond * DeltaTime;

		const FVector HeightMoveDelta = FVector(
			0.0f,
			0.0f,
			FMath::Clamp(DesiredCorrectionZ, -MaxCorrectionZ, MaxCorrectionZ)
		);

		if (HeightMoveDelta.IsNearlyZero())
		{
			return false;
		}

		FHitResult HeightMoveHit;
		if (IsAttackMoveBlocked(HeightMoveDelta, HeightMoveHit))
		{
			++RepeatedAttackPositioningBlockCount;

			PrintDebugMessage(
				FString::Printf(
					TEXT("Attack slot vertical adjustment blocked by: %s | rocket height error: %.2f | count: %d / %d"),
					HeightMoveHit.GetActor() ? *HeightMoveHit.GetActor()->GetName() : TEXT("Unknown"),
					RocketHeightError,
					RepeatedAttackPositioningBlockCount,
					AttackSlotBlockThreshold
				)
			);

			if (RepeatedAttackPositioningBlockCount >= AttackSlotBlockThreshold)
			{
				RememberBadAttackSlot(CurrentAttackSlotLocation);
				ClearCurrentAttackSlot();
			}

			return false;
		}

		if (!TryApplyAttackMoveDelta(HeightMoveDelta, DeltaTime, FColor::Purple))
		{
			++RepeatedAttackPositioningBlockCount;

			PrintDebugMessage(
				FString::Printf(
					TEXT("Attack slot vertical adjustment failed | rocket height error: %.2f | count: %d / %d"),
					RocketHeightError,
					RepeatedAttackPositioningBlockCount,
					AttackSlotBlockThreshold
				)
			);

			if (RepeatedAttackPositioningBlockCount >= AttackSlotBlockThreshold)
			{
				RememberBadAttackSlot(CurrentAttackSlotLocation);
				ClearCurrentAttackSlot();
			}

			return false;
		}

		RepeatedAttackPositioningBlockCount = 0;

		PrintDebugMessage(
			FString::Printf(
				TEXT("Attack slot vertical adjustment applied | rocket height error: %.2f"),
				RocketHeightError
			)
		);

		return false;
	}

	RepeatedAttackPositioningBlockCount = 0;
	UpdateLastSafeDroneTransform();
	return true;
}

bool UDroneCombatControllerComponent::TryApplyAttackMoveDelta(
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
				TEXT("Attack body movement blocked by: %s"),
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

		return false;
	}

	const float ActualSpeedCmPerSecond = MoveDelta.Size() / DeltaTime;
	const float MovementEnergyCost = CalculateMovementEnergyCost(ActualSpeedCmPerSecond, DeltaTime);

	if (MovementEnergyCost > 0.0f && !DroneCarrier->ConsumeEnergy(MovementEnergyCost))
	{
		SetCurrentTarget(nullptr);
		ClearTargetMemory();
		SetCombatState(EDroneCombatState::Disabled);
		PrintDebugMessage(TEXT("Drone has no energy for attack positioning."));
		return false;
	}

	const FVector OwnerLocation = OwnerActor->GetActorLocation();
	const FVector NewLocation = OwnerLocation + MoveDelta;

	FHitResult EndLocationHit;
	if (IsDroneBodyLocationBlocked(NewLocation, EndLocationHit))
	{
		PrintDebugMessage(
			FString::Printf(
				TEXT("Attack target location blocked for drone body by: %s"),
				EndLocationHit.GetActor() ? *EndLocationHit.GetActor()->GetName() : TEXT("Unknown")
			)
		);

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
				TEXT("Attack positioning root sweep blocked by: %s"),
				SweepHit.GetActor() ? *SweepHit.GetActor()->GetName() : TEXT("Unknown")
			)
		);

		DrawDebugSphere(
			GetWorld(),
			SweepHit.ImpactPoint,
			35.0f,
			12,
			FColor::Red,
			false,
			DebugDrawDuration
		);

		return false;
	}

	if (!bMoved)
	{
		PrintDebugMessage(TEXT("Attack positioning movement failed."));
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

bool UDroneCombatControllerComponent::IsAttackMoveBlocked(
	const FVector& MoveDelta,
	FHitResult& OutHit
) const
{
	if (MoveDelta.IsNearlyZero())
	{
		return false;
	}

	if (bUseDroneBodySafetyCollision)
	{
		return IsDroneBodyMoveBlocked(MoveDelta, OutHit);
	}

	const UWorld* World = GetWorld();
	const AActor* OwnerActor = GetOwner();

	if (!World || !OwnerActor)
	{
		return false;
	}

	const FVector StartLocation = OwnerActor->GetActorLocation();
	const FVector EndLocation = StartLocation + MoveDelta;

	FCollisionQueryParams QueryParams(TEXT("DroneCombatAttackPositionTrace"), false);
	AddDroneIgnoredActors(QueryParams, true);

	const bool bBlocked = World->SweepSingleByChannel(
		OutHit,
		StartLocation,
		EndLocation,
		FQuat::Identity,
		MovementObstacleTraceChannel,
		FCollisionShape::MakeSphere(AttackPositionObstacleSphereRadiusCm),
		QueryParams
	);

	if (bDrawDebugAttackOrbit)
	{
		DrawDebugLine(
			GetWorld(),
			StartLocation,
			EndLocation,
			bBlocked ? FColor::Red : FColor::Orange,
			false,
			DebugDrawDuration,
			0,
			2.0f
		);
	}

	return bBlocked && OutHit.bBlockingHit;
}

bool UDroneCombatControllerComponent::BuildAttackOrbitFallbackMoveDelta(
	const FVector& AimLocation,
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

	const FVector HorizontalFromAimToOwner = FVector(
		OwnerLocation.X - AimLocation.X,
		OwnerLocation.Y - AimLocation.Y,
		0.0f
	);

	if (HorizontalFromAimToOwner.IsNearlyZero())
	{
		return false;
	}

	const FVector RadialAwayFromAim = HorizontalFromAimToOwner.GetSafeNormal();
	const FVector RadialTowardAim = -RadialAwayFromAim;

	const FVector OrbitRight = FVector::CrossProduct(FVector::UpVector, RadialAwayFromAim).GetSafeNormal();
	const FVector OrbitLeft = -OrbitRight;

	const float HorizontalDistance = HorizontalFromAimToOwner.Size();
	const float OrbitMoveDistance = AttackOrbitMoveSpeedCmPerSecond * DeltaTime;

	if (OrbitMoveDistance <= KINDA_SMALL_NUMBER)
	{
		return false;
	}

	FVector RadialCorrection = FVector::ZeroVector;

	if (HorizontalDistance < PreferredAttackDistanceCm - AttackPositionDistanceToleranceCm)
	{
		RadialCorrection = RadialAwayFromAim * AttackOrbitRadialCorrectionStrength;
	}
	else if (HorizontalDistance > PreferredAttackDistanceCm + AttackPositionDistanceToleranceCm)
	{
		RadialCorrection = RadialTowardAim * AttackOrbitRadialCorrectionStrength;
	}

	const TArray<FVector> CandidateDirections =
	{
		(OrbitRight + RadialCorrection).GetSafeNormal(),
		(OrbitLeft + RadialCorrection).GetSafeNormal(),
		(OrbitRight + RadialCorrection + FVector::UpVector * 0.35f).GetSafeNormal(),
		(OrbitLeft + RadialCorrection + FVector::UpVector * 0.35f).GetSafeNormal(),
		(RadialAwayFromAim + FVector::UpVector * 0.25f).GetSafeNormal(),
		FVector::UpVector
	};

	float BestScore = -1000000000.0f;
	FVector BestMoveDelta = FVector::ZeroVector;

	for (const FVector& CandidateDirection : CandidateDirections)
	{
		if (CandidateDirection.IsNearlyZero())
		{
			continue;
		}

		const FVector CandidateMoveDelta = CandidateDirection * OrbitMoveDistance;
		const FVector CandidateLocation = OwnerLocation + CandidateMoveDelta;

		FHitResult CandidateMoveHit;
		if (IsAttackMoveBlocked(CandidateMoveDelta, CandidateMoveHit))
		{
			continue;
		}

		FHitResult CandidateLocationHit;
		if (IsDroneBodyLocationBlocked(CandidateLocation, CandidateLocationHit))
		{
			continue;
		}

		const float CandidateDistanceToAim = FVector::Dist2D(CandidateLocation, AimLocation);
		const float CurrentDistanceError = FMath::Abs(HorizontalDistance - PreferredAttackDistanceCm);
		const float CandidateDistanceError = FMath::Abs(CandidateDistanceToAim - PreferredAttackDistanceCm);

		float Score = 0.0f;
		Score += (CurrentDistanceError - CandidateDistanceError) * 0.02f;
		Score += FVector::DotProduct(
			FVector(CandidateDirection.X, CandidateDirection.Y, 0.0f).GetSafeNormal(),
			OrbitRight
		) * 0.35f;
		Score += CandidateDirection.Z * 0.2f;

		if (Score > BestScore)
		{
			BestScore = Score;
			BestMoveDelta = CandidateMoveDelta;
		}

		if (bDrawDebugAttackOrbit)
		{
			DrawDebugLine(
				GetWorld(),
				OwnerLocation,
				CandidateLocation,
				FColor::Orange,
				false,
				DebugDrawDuration,
				0,
				3.0f
			);

			DrawDebugSphere(
				GetWorld(),
				CandidateLocation,
				35.0f,
				12,
				FColor::Orange,
				false,
				DebugDrawDuration
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

bool UDroneCombatControllerComponent::BuildAttackObstacleAwareFallbackMoveDelta(
	const FVector& AimLocation,
	const FVector& DesiredMoveDelta,
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

	FVector DesiredHorizontalDirection = FVector(
		DesiredMoveDelta.X,
		DesiredMoveDelta.Y,
		0.0f
	).GetSafeNormal();

	if (DesiredHorizontalDirection.IsNearlyZero())
	{
		const FVector HorizontalToAim = FVector(
			AimLocation.X - OwnerLocation.X,
			AimLocation.Y - OwnerLocation.Y,
			0.0f
		);

		if (!HorizontalToAim.IsNearlyZero())
		{
			DesiredHorizontalDirection = HorizontalToAim.GetSafeNormal();
		}
	}

	if (DesiredHorizontalDirection.IsNearlyZero())
	{
		DesiredHorizontalDirection = FVector(
			OwnerActor->GetActorForwardVector().X,
			OwnerActor->GetActorForwardVector().Y,
			0.0f
		).GetSafeNormal();
	}

	if (DesiredHorizontalDirection.IsNearlyZero())
	{
		DesiredHorizontalDirection = FVector::ForwardVector;
	}

	FVector ObstacleNormal = FVector(
		BlockHit.ImpactNormal.X,
		BlockHit.ImpactNormal.Y,
		0.0f
	).GetSafeNormal();

	if (ObstacleNormal.IsNearlyZero())
	{
		ObstacleNormal = -DesiredHorizontalDirection;
	}

	const FVector ObstacleTangentA = FVector::CrossProduct(FVector::UpVector, ObstacleNormal).GetSafeNormal();
	const FVector ObstacleTangentB = -ObstacleTangentA;

	const float TangentADesiredDot = FVector::DotProduct(ObstacleTangentA, DesiredHorizontalDirection);
	const float TangentBDesiredDot = FVector::DotProduct(ObstacleTangentB, DesiredHorizontalDirection);

	const FVector PreferredObstacleTangent = TangentADesiredDot >= TangentBDesiredDot
		? ObstacleTangentA
		: ObstacleTangentB;

	const FVector OtherObstacleTangent = TangentADesiredDot >= TangentBDesiredDot
		? ObstacleTangentB
		: ObstacleTangentA;

	const FVector HorizontalFromAimToOwner = FVector(
		OwnerLocation.X - AimLocation.X,
		OwnerLocation.Y - AimLocation.Y,
		0.0f
	);

	FVector RadialAwayFromAim = -DesiredHorizontalDirection;

	if (!HorizontalFromAimToOwner.IsNearlyZero())
	{
		RadialAwayFromAim = HorizontalFromAimToOwner.GetSafeNormal();
	}

	const FVector RadialTowardAim = -RadialAwayFromAim;

	const FVector OrbitRight = FVector::CrossProduct(FVector::UpVector, RadialAwayFromAim).GetSafeNormal();
	const FVector OrbitLeft = -OrbitRight;

	const float OrbitRightDot = FVector::DotProduct(OrbitRight, DesiredHorizontalDirection);
	const float OrbitLeftDot = FVector::DotProduct(OrbitLeft, DesiredHorizontalDirection);

	const FVector PreferredOrbitDirection = OrbitRightDot >= OrbitLeftDot
		? OrbitRight
		: OrbitLeft;

	const FVector OtherOrbitDirection = OrbitRightDot >= OrbitLeftDot
		? OrbitLeft
		: OrbitRight;

	const FVector AwayFromObstacle = ObstacleNormal;

	float MoveDistance = DesiredMoveDelta.Size();

	if (MoveDistance <= KINDA_SMALL_NUMBER)
	{
		MoveDistance = AttackOrbitMoveSpeedCmPerSecond * DeltaTime;
	}

	if (MoveDistance <= KINDA_SMALL_NUMBER)
	{
		MoveDistance = ApproachSpeedCmPerSecond * DeltaTime;
	}

	MoveDistance = FMath::Max(10.0f, MoveDistance);

	const float DesiredVerticalSign = FMath::Sign(DesiredMoveDelta.Z);

	const TArray<FVector> CandidateDirections =
	{
		PreferredObstacleTangent,
		(PreferredObstacleTangent + AwayFromObstacle * 0.35f).GetSafeNormal(),
		(PreferredObstacleTangent + AwayFromObstacle * 0.35f + FVector::UpVector * 0.25f).GetSafeNormal(),
		(PreferredObstacleTangent + FVector::UpVector * 0.35f).GetSafeNormal(),

		OtherObstacleTangent,
		(OtherObstacleTangent + AwayFromObstacle * 0.35f).GetSafeNormal(),
		(OtherObstacleTangent + AwayFromObstacle * 0.35f + FVector::UpVector * 0.25f).GetSafeNormal(),
		(OtherObstacleTangent + FVector::UpVector * 0.35f).GetSafeNormal(),

		PreferredOrbitDirection,
		(PreferredOrbitDirection + AwayFromObstacle * 0.35f).GetSafeNormal(),
		(PreferredOrbitDirection + AwayFromObstacle * 0.35f + FVector::UpVector * 0.25f).GetSafeNormal(),

		OtherOrbitDirection,
		(OtherOrbitDirection + AwayFromObstacle * 0.35f).GetSafeNormal(),
		(OtherOrbitDirection + AwayFromObstacle * 0.35f + FVector::UpVector * 0.25f).GetSafeNormal(),

		(AwayFromObstacle + FVector::UpVector * 0.35f).GetSafeNormal(),
		(RadialAwayFromAim + AwayFromObstacle * 0.25f + FVector::UpVector * 0.2f).GetSafeNormal(),
		(RadialTowardAim + PreferredObstacleTangent * 0.5f).GetSafeNormal(),
		FVector::UpVector
	};

	const float CurrentDistanceToAim = FVector::Dist2D(OwnerLocation, AimLocation);
	const float CurrentDistanceError = FMath::Abs(CurrentDistanceToAim - PreferredAttackDistanceCm);

	float BestScore = -1000000000.0f;
	FVector BestMoveDelta = FVector::ZeroVector;

	for (const FVector& CandidateDirection : CandidateDirections)
	{
		if (CandidateDirection.IsNearlyZero())
		{
			continue;
		}

		const FVector NormalizedCandidateDirection = CandidateDirection.GetSafeNormal();
		const FVector CandidateHorizontalDirection = FVector(
			NormalizedCandidateDirection.X,
			NormalizedCandidateDirection.Y,
			0.0f
		).GetSafeNormal();

		const FVector CandidateMoveDelta = NormalizedCandidateDirection * MoveDistance;
		const FVector CandidateLocation = OwnerLocation + CandidateMoveDelta;

		FHitResult CandidateMoveHit;
		if (IsAttackMoveBlocked(CandidateMoveDelta, CandidateMoveHit))
		{
			if (bDrawDebugAttackOrbit)
			{
				DrawDebugLine(
					GetWorld(),
					OwnerLocation,
					CandidateLocation,
					FColor::Red,
					false,
					DebugDrawDuration,
					0,
					1.5f
				);
			}

			continue;
		}

		FHitResult CandidateLocationHit;
		if (IsDroneBodyLocationBlocked(CandidateLocation, CandidateLocationHit))
		{
			if (bDrawDebugAttackOrbit)
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

			continue;
		}

		const float CandidateDistanceToAim = FVector::Dist2D(CandidateLocation, AimLocation);
		const float CandidateDistanceError = FMath::Abs(CandidateDistanceToAim - PreferredAttackDistanceCm);

		float Score = 0.0f;

		Score += (CurrentDistanceError - CandidateDistanceError) * 0.035f;

		if (!CandidateHorizontalDirection.IsNearlyZero())
		{
			Score += FVector::DotProduct(CandidateHorizontalDirection, PreferredObstacleTangent) * 1.25f;
			Score += FVector::DotProduct(CandidateHorizontalDirection, AwayFromObstacle) * 0.9f;
			Score += FVector::DotProduct(CandidateHorizontalDirection, PreferredOrbitDirection) * 0.75f;
			Score += FVector::DotProduct(CandidateHorizontalDirection, DesiredHorizontalDirection) * 0.45f;
		}

		if (!FMath::IsNearlyZero(DesiredVerticalSign))
		{
			Score += DesiredVerticalSign * NormalizedCandidateDirection.Z * 0.8f;
		}
		else
		{
			Score += NormalizedCandidateDirection.Z * 0.15f;
		}

		if (CandidateDistanceToAim < MinimumSafeAttackDistanceCm)
		{
			Score -= (MinimumSafeAttackDistanceCm - CandidateDistanceToAim) * 0.05f;
		}

		if (Score > BestScore)
		{
			BestScore = Score;
			BestMoveDelta = CandidateMoveDelta;
		}

		if (bDrawDebugAttackOrbit)
		{
			DrawDebugLine(
				GetWorld(),
				OwnerLocation,
				CandidateLocation,
				FColor::Orange,
				false,
				DebugDrawDuration,
				0,
				2.5f
			);

			DrawDebugSphere(
				GetWorld(),
				CandidateLocation,
				35.0f,
				12,
				FColor::Orange,
				false,
				DebugDrawDuration
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

bool UDroneCombatControllerComponent::FindBestAttackSlot(
	const FVector& AimLocation,
	FVector& OutAttackSlotLocation
) const
{
	OutAttackSlotLocation = FVector::ZeroVector;

	const AActor* OwnerActor = GetOwner();

	if (!OwnerActor)
	{
		return false;
	}

	const FVector OwnerLocation = OwnerActor->GetActorLocation();

	FVector BaseRadialDirection = FVector(
		OwnerLocation.X - AimLocation.X,
		OwnerLocation.Y - AimLocation.Y,
		0.0f
	).GetSafeNormal();

	if (BaseRadialDirection.IsNearlyZero())
	{
		BaseRadialDirection = FVector(
			OwnerActor->GetActorForwardVector().X,
			OwnerActor->GetActorForwardVector().Y,
			0.0f
		).GetSafeNormal();
	}

	if (BaseRadialDirection.IsNearlyZero())
	{
		BaseRadialDirection = FVector::ForwardVector;
	}

	const float RocketMountZOffset = GetRocketMountZOffsetFromOwner();
	const float DesiredOwnerZ = AimLocation.Z - RocketMountZOffset;

	TArray<float> VerticalOffsets;
	VerticalOffsets.Add(0.0f);
	VerticalOffsets.Add(AimHeightToleranceCm * 0.5f);
	VerticalOffsets.Add(AimHeightToleranceCm);

	const int32 VerticalLevels = FMath::Max(0, AttackSlotVerticalSearchLevels);
	const float VerticalStep = FMath::Max(10.0f, AttackSlotVerticalSearchStepCm);

	for (int32 LevelIndex = 1; LevelIndex <= VerticalLevels; ++LevelIndex)
	{
		VerticalOffsets.Add(VerticalStep * static_cast<float>(LevelIndex));
	}

	TArray<float> AngleOffsets;
	AngleOffsets.Add(0.0f);

	const int32 RadialCount = FMath::Max(4, AttackSlotRadialCandidateCount);
	const float AngleStep = 360.0f / static_cast<float>(RadialCount);

	for (int32 Index = 1; Index <= RadialCount / 2; ++Index)
	{
		AngleOffsets.Add(AngleStep * static_cast<float>(Index));
		AngleOffsets.Add(-AngleStep * static_cast<float>(Index));
	}

	float BestScore = -1000000000.0f;
	FVector BestSlotLocation = FVector::ZeroVector;

	for (const float VerticalOffset : VerticalOffsets)
	{
		const float CandidateOwnerZ = DesiredOwnerZ + VerticalOffset;

		for (const float AngleOffset : AngleOffsets)
		{
			const FVector CandidateRadialDirection = BaseRadialDirection.RotateAngleAxis(
				AngleOffset,
				FVector::UpVector
			).GetSafeNormal();

			if (CandidateRadialDirection.IsNearlyZero())
			{
				continue;
			}

			const FVector CandidateSlotLocation = FVector(
				AimLocation.X + CandidateRadialDirection.X * PreferredAttackDistanceCm,
				AimLocation.Y + CandidateRadialDirection.Y * PreferredAttackDistanceCm,
				CandidateOwnerZ
			);

			if (IsAttackSlotNearBadLocation(CandidateSlotLocation))
			{
				continue;
			}

			FHitResult SlotValidationHit;
			if (!IsAttackSlotLocationValid(
				CandidateSlotLocation,
				AimLocation,
				true,
				bRequireAttackLineForAttackSlot,
				SlotValidationHit
			))
			{
				continue;
			}

			bool bInitialPathBlocked = false;
			const FVector ToCandidate = CandidateSlotLocation - OwnerLocation;

			if (!ToCandidate.IsNearlyZero())
			{
				const FVector HorizontalToCandidate = FVector(
					ToCandidate.X,
					ToCandidate.Y,
					0.0f
				);

				FVector ProbeDirection = HorizontalToCandidate.GetSafeNormal();

				if (ProbeDirection.IsNearlyZero())
				{
					ProbeDirection = ToCandidate.GetSafeNormal();
				}

				const float ProbeDistance = FMath::Min(
					FMath::Max(50.0f, AttackSlotInitialPathProbeDistanceCm),
					ToCandidate.Size()
				);

				const FVector InitialProbeMoveDelta = ProbeDirection * ProbeDistance;

				FHitResult InitialProbeHit;
				bInitialPathBlocked = IsAttackMoveBlocked(InitialProbeMoveDelta, InitialProbeHit);
			}

			const float HorizontalDistanceToAim = FVector::Dist2D(CandidateSlotLocation, AimLocation);
			const float DistanceError = FMath::Abs(HorizontalDistanceToAim - PreferredAttackDistanceCm);
			const float CandidateDistanceFromOwner = FVector::Distance(OwnerLocation, CandidateSlotLocation);
			const float HeightError = FMath::Abs(AimLocation.Z - (CandidateSlotLocation.Z + RocketMountZOffset));

			float Score = 0.0f;
			Score -= DistanceError * 0.02f;
			Score -= CandidateDistanceFromOwner * 0.001f;
			Score -= HeightError * 0.08f;
			Score -= FMath::Abs(VerticalOffset) * 0.01f;

			if (bInitialPathBlocked)
			{
				Score -= 2.5f;
			}
			else
			{
				Score += 1.0f;
			}

			const float AnglePreference = 1.0f - FMath::Clamp(FMath::Abs(AngleOffset) / 180.0f, 0.0f, 1.0f);
			Score += AnglePreference * 1.25f;

			if (FMath::Abs(HeightError) <= AimHeightToleranceCm * 0.5f)
			{
				Score += 3.0f;
			}

			if (VerticalOffset > 0.0f)
			{
				Score += 0.35f;
			}

			if (Score > BestScore)
			{
				BestScore = Score;
				BestSlotLocation = CandidateSlotLocation;
			}

			if (bDrawDebugAttackOrbit)
			{
				DrawDebugSphere(
					GetWorld(),
					CandidateSlotLocation,
					40.0f,
					12,
					bInitialPathBlocked ? FColor::Orange : FColor::Cyan,
					false,
					DebugDrawDuration
				);
			}
		}
	}

	if (BestSlotLocation.IsNearlyZero())
	{
		return false;
	}

	OutAttackSlotLocation = BestSlotLocation;
	return true;
}

bool UDroneCombatControllerComponent::IsAttackSlotLocationValid(
	const FVector& SlotLocation,
	const FVector& AimLocation,
	bool bRequireHeightReady,
	bool bRequireAttackLine,
	FHitResult& OutHit
) const
{
	OutHit = FHitResult();

	if (IsDroneBodyLocationBlocked(SlotLocation, OutHit))
	{
		return false;
	}

	if (bRequireHeightReady)
	{
		const float RocketMountZOffset = GetRocketMountZOffsetFromOwner();
		const float HeightError = AimLocation.Z - (SlotLocation.Z + RocketMountZOffset);

		if (FMath::Abs(HeightError) > AimHeightToleranceCm)
		{
			return false;
		}
	}

	if (bRequireAttackLine)
	{
		const UWorld* World = GetWorld();
		const AActor* OwnerActor = GetOwner();

		if (!World || !OwnerActor || !IsValid(CurrentTarget))
		{
			return false;
		}

		FVector RocketMountOffset = FVector::ZeroVector;

		if (IsValid(RocketWeaponMount))
		{
			RocketMountOffset = RocketWeaponMount->GetComponentLocation() - OwnerActor->GetActorLocation();
		}

		const FVector TraceStartLocation = SlotLocation + RocketMountOffset;

		FCollisionQueryParams QueryParams(TEXT("DroneCombatAttackSlotLineTrace"), false);
		AddDroneIgnoredActors(QueryParams, false);

		const bool bHit = World->LineTraceSingleByChannel(
			OutHit,
			TraceStartLocation,
			AimLocation,
			AttackLineTraceChannel,
			QueryParams
		);

		const bool bClearLine = !bHit || OutHit.GetActor() == CurrentTarget;

		if (!bClearLine)
		{
			return false;
		}
	}

	return true;
}

bool UDroneCombatControllerComponent::IsAttackSlotNearBadLocation(const FVector& SlotLocation) const
{
	const UWorld* World = GetWorld();

	if (!World)
	{
		return false;
	}

	const float CurrentTimeSeconds = World->GetTimeSeconds();
	const float BadRadius = FMath::Max(1.0f, AttackSlotBadLocationRadiusCm);

	for (int32 Index = 0; Index < BadAttackSlotLocations.Num(); ++Index)
	{
		if (BadAttackSlotExpireTimeSeconds.IsValidIndex(Index))
		{
			if (BadAttackSlotExpireTimeSeconds[Index] <= CurrentTimeSeconds)
			{
				continue;
			}
		}

		if (FVector::Dist2D(SlotLocation, BadAttackSlotLocations[Index]) <= BadRadius)
		{
			return true;
		}
	}

	return false;
}

void UDroneCombatControllerComponent::RememberBadAttackSlot(const FVector& SlotLocation)
{
	const UWorld* World = GetWorld();

	if (!World)
	{
		return;
	}

	const float ExpireTimeSeconds = World->GetTimeSeconds() + AttackSlotBadLocationMemorySeconds;
	const int32 MaxBadLocations = FMath::Max(1, AttackSlotMaxBadLocations);

	BadAttackSlotLocations.Add(SlotLocation);
	BadAttackSlotExpireTimeSeconds.Add(ExpireTimeSeconds);

	while (BadAttackSlotLocations.Num() > MaxBadLocations)
	{
		BadAttackSlotLocations.RemoveAt(0);

		if (BadAttackSlotExpireTimeSeconds.Num() > 0)
		{
			BadAttackSlotExpireTimeSeconds.RemoveAt(0);
		}
	}

	PrintDebugMessage(
		FString::Printf(
			TEXT("Bad attack slot remembered: %s | total bad slots: %d"),
			*SlotLocation.ToString(),
			BadAttackSlotLocations.Num()
		)
	);
}

void UDroneCombatControllerComponent::ClearExpiredBadAttackSlots()
{
	const UWorld* World = GetWorld();

	if (!World)
	{
		return;
	}

	const float CurrentTimeSeconds = World->GetTimeSeconds();

	for (int32 Index = BadAttackSlotLocations.Num() - 1; Index >= 0; --Index)
	{
		if (!BadAttackSlotExpireTimeSeconds.IsValidIndex(Index))
		{
			BadAttackSlotLocations.RemoveAt(Index);
			continue;
		}

		if (BadAttackSlotExpireTimeSeconds[Index] <= CurrentTimeSeconds)
		{
			BadAttackSlotLocations.RemoveAt(Index);
			BadAttackSlotExpireTimeSeconds.RemoveAt(Index);
		}
	}
}

void UDroneCombatControllerComponent::ClearCurrentAttackSlot()
{
	CurrentAttackSlotLocation = FVector::ZeroVector;
	bHasCurrentAttackSlot = false;
	RepeatedAttackPositioningBlockCount = 0;
	ResetAttackSlotProgressTracking();
}

void UDroneCombatControllerComponent::ResetAttackSlotProgressTracking()
{
	LastAttackSlotProgressDistanceCm = -1.0f;
	LastAttackSlotProgressTimeSeconds = -1.0f;
}

bool UDroneCombatControllerComponent::HasAttackSlotMovementTimedOut(float CurrentDistanceToSlot)
{
	const UWorld* World = GetWorld();

	if (!World)
	{
		return false;
	}

	const float CurrentTimeSeconds = World->GetTimeSeconds();
	const float ProgressTolerance = FMath::Max(1.0f, AttackSlotProgressToleranceCm);

	if (LastAttackSlotProgressDistanceCm < 0.0f || LastAttackSlotProgressTimeSeconds < 0.0f)
	{
		LastAttackSlotProgressDistanceCm = CurrentDistanceToSlot;
		LastAttackSlotProgressTimeSeconds = CurrentTimeSeconds;
		return false;
	}

	if (CurrentDistanceToSlot < LastAttackSlotProgressDistanceCm - ProgressTolerance)
	{
		LastAttackSlotProgressDistanceCm = CurrentDistanceToSlot;
		LastAttackSlotProgressTimeSeconds = CurrentTimeSeconds;
		return false;
	}

	return CurrentTimeSeconds - LastAttackSlotProgressTimeSeconds >= AttackSlotNoProgressTimeoutSeconds;
}
