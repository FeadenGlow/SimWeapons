// Fill out your copyright notice in the Description page of Project Settings.

#include "Weapons/Defensive/SimDefensiveLauncherWeapon.h"

#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Weapons/Defensive/SimDefensiveProjectile.h"

ASimDefensiveLauncherWeapon::ASimDefensiveLauncherWeapon()
{
	BatteryCost = 0.0f;
	MaxAmmo = 3;

	// Active protection system is useful but compact.
	WeaponSlotCost = 1;

	// It should not be spammed instantly.
	FireCooldown = 1.0f;
	bUseFireCooldown = true;

	ProjectileCount = 4;
	LaunchPitchDegrees = 75.0f;
	SpreadAngleDegrees = 25.0f;
	SpawnForwardOffset = 40.0f;
	SpawnUpOffset = 80.0f;
}

void ASimDefensiveLauncherWeapon::Fire_Implementation()
{
	if (!CanFire())
	{
		UE_LOG(LogTemp, Warning, TEXT("Defensive launcher cannot fire: no ammo or weapon is on cooldown."));
		return;
	}

	if (!DefensiveProjectileClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("Defensive launcher cannot fire: DefensiveProjectileClass is not set."));
		return;
	}

	UWorld* World = GetWorld();

	if (!World)
	{
		return;
	}

	if (!TryConsumeAmmo())
	{
		return;
	}

	const FVector SpawnLocation =
		GetActorLocation() +
		GetActorForwardVector() * SpawnForwardOffset +
		GetActorUpVector() * SpawnUpOffset;

	// Direction mostly upward, but with some forward component.
	const FVector BaseLaunchDirection =
		(
			GetActorForwardVector() * FMath::Cos(FMath::DegreesToRadians(LaunchPitchDegrees)) +
			GetActorUpVector() * FMath::Sin(FMath::DegreesToRadians(LaunchPitchDegrees))
		).GetSafeNormal();

	const float SpreadRadians = FMath::DegreesToRadians(SpreadAngleDegrees);

	TArray<ASimDefensiveProjectile*> SpawnedProjectiles;
	SpawnedProjectiles.Reserve(ProjectileCount);

	int32 SpawnedProjectileCount = 0;

	for (int32 ProjectileIndex = 0; ProjectileIndex < ProjectileCount; ProjectileIndex++)
	{
		FVector LaunchDirection = BaseLaunchDirection;

		if (SpreadAngleDegrees > 0.0f)
		{
			LaunchDirection = FMath::VRandCone(BaseLaunchDirection, SpreadRadians).GetSafeNormal();
		}

		FRotator SpawnRotation = LaunchDirection.Rotation();

		if (bRandomizeProjectileRoll)
		{
			SpawnRotation.Roll = FMath::FRandRange(0.0f, 360.0f);
		}

		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = GetOwner() ? GetOwner() : this;
		SpawnParams.Instigator = Cast<APawn>(GetOwner());
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		ASimDefensiveProjectile* SpawnedProjectile = World->SpawnActor<ASimDefensiveProjectile>(
			DefensiveProjectileClass,
			SpawnLocation,
			SpawnRotation,
			SpawnParams
		);

		if (!SpawnedProjectile)
		{
			UE_LOG(LogTemp, Warning, TEXT("Defensive launcher failed to spawn projectile %d."), ProjectileIndex);
			continue;
		}

		TArray<AActor*> ActorsToIgnore;

		if (AActor* OwnerActor = GetOwner())
		{
			ActorsToIgnore.Add(OwnerActor);
		}

		ActorsToIgnore.Add(this);

		for (ASimDefensiveProjectile* PreviousProjectile : SpawnedProjectiles)
		{
			if (!PreviousProjectile)
			{
				continue;
			}

			ActorsToIgnore.Add(PreviousProjectile);
			PreviousProjectile->AddIgnoredActor(SpawnedProjectile);
		}

		SpawnedProjectile->InitializeDefensiveProjectile(LaunchDirection, ActorsToIgnore);

		SpawnedProjectiles.Add(SpawnedProjectile);
		SpawnedProjectileCount++;

		OnDefensiveProjectileSpawned(SpawnedProjectile);
	}

	OnDefensiveLauncherFired(SpawnedProjectileCount);

	UE_LOG(
		LogTemp,
		Log,
		TEXT("Defensive launcher fired. Requested projectiles: %d | Spawned projectiles: %d | Ammo left: %d"),
		ProjectileCount,
		SpawnedProjectileCount,
		GetCurrentAmmo()
	);
}