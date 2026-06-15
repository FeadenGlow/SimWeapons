// Fill out your copyright notice in the Description page of Project Settings.

#include "Combat/DroneCombatControllerComponent.h"

#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

bool UDroneCombatControllerComponent::HasValidTargetMemory() const
{
	if (!bUseTargetMemory)
	{
		return false;
	}

	if (!bHasLastKnownTargetLocation)
	{
		return false;
	}

	if (LastSeenTargetTimeSeconds < 0.0f)
	{
		return false;
	}

	const UWorld* World = GetWorld();

	if (!World)
	{
		return false;
	}

	const float CurrentTimeSeconds = World->GetTimeSeconds();
	const float TimeSinceLastSeen = CurrentTimeSeconds - LastSeenTargetTimeSeconds;

	return TimeSinceLastSeen <= TargetMemoryDurationSeconds;
}

FVector UDroneCombatControllerComponent::GetLastKnownTargetLocation() const
{
	return LastKnownTargetLocation;
}

void UDroneCombatControllerComponent::RememberTarget(AActor* Target)
{
	if (!bUseTargetMemory)
	{
		return;
	}

	if (!IsValid(Target))
	{
		return;
	}

	const UWorld* World = GetWorld();

	if (!World)
	{
		return;
	}

	LastKnownTargetActor = Target;
	LastKnownTargetLocation = Target->GetActorLocation();
	LastSeenTargetTimeSeconds = World->GetTimeSeconds();
	bHasLastKnownTargetLocation = true;

	if (bDrawDebugLastKnownTargetLocation)
	{
		DrawDebugSphere(
			GetWorld(),
			LastKnownTargetLocation,
			80.0f,
			16,
			FColor::Magenta,
			false,
			DebugDrawDuration
		);
	}
}

void UDroneCombatControllerComponent::ClearTargetMemory()
{
	LastKnownTargetActor = nullptr;
	LastKnownTargetLocation = FVector::ZeroVector;
	LastSeenTargetTimeSeconds = -1.0f;
	bHasLastKnownTargetLocation = false;
}

bool UDroneCombatControllerComponent::GetCurrentNavigationTargetLocation(
	FVector& OutLocation,
	bool& bOutUsingTargetMemory
) const
{
	bOutUsingTargetMemory = false;
	OutLocation = FVector::ZeroVector;

	if (IsValid(CurrentTarget))
	{
		OutLocation = CurrentTarget->GetActorLocation();
		return true;
	}

	if (HasValidTargetMemory())
	{
		OutLocation = LastKnownTargetLocation;
		bOutUsingTargetMemory = true;
		return true;
	}

	return false;
}