// Fill out your copyright notice in the Description page of Project Settings.

#include "Weapons/Shotgun/SimShotgunWeapon.h"

#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Weapons/Shotgun/SimShotgunPelletProjectile.h"

ASimShotgunWeapon::ASimShotgunWeapon()
{
	BatteryCost = 8.0f;
	MaxAmmo = 6;

	// For now shotgun uses one slot.
	// If it feels too strong later, set this to 2 in C++ or Blueprint.
	WeaponSlotCost = 1;

	// Delay between shotgun shots.
	FireCooldown = 0.4f;
	bUseFireCooldown = true;

	PelletCount = 12;
	SpreadAngleDegrees = 10.0f;
	SpawnForwardOffset = 100.0f;
}

void ASimShotgunWeapon::Fire_Implementation()
{
	if (!CanFire())
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("Shotgun cannot fire. Ammo: %d | Cooldown left: %.2f"),
			GetCurrentAmmo(),
			GetRemainingFireCooldown()
		);

		return;
	}

	if (!PelletProjectileClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("Shotgun cannot fire: PelletProjectileClass is not set."));
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
		GetActorForwardVector() * SpawnForwardOffset;

	const FVector BaseForwardDirection = GetActorForwardVector().GetSafeNormal();
	const float SpreadRadians = FMath::DegreesToRadians(SpreadAngleDegrees);

	TArray<ASimShotgunPelletProjectile*> SpawnedPellets;
	SpawnedPellets.Reserve(PelletCount);

	int32 SpawnedPelletCount = 0;

	for (int32 PelletIndex = 0; PelletIndex < PelletCount; PelletIndex++)
	{
		FVector PelletDirection = BaseForwardDirection;

		if (SpreadAngleDegrees > 0.0f)
		{
			PelletDirection = FMath::VRandCone(BaseForwardDirection, SpreadRadians).GetSafeNormal();
		}

		FRotator SpawnRotation = PelletDirection.Rotation();

		if (bRandomizePelletRoll)
		{
			SpawnRotation.Roll = FMath::FRandRange(0.0f, 360.0f);
		}

		FActorSpawnParameters SpawnParams;

		// Owner is the carrier. Instigator is also the carrier if it is a pawn.
		SpawnParams.Owner = GetOwner() ? GetOwner() : this;
		SpawnParams.Instigator = Cast<APawn>(GetOwner());

		// AlwaysSpawn prevents Unreal from moving pellets upward or sideways
		// when many pellets are spawned from the same muzzle point.
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		ASimShotgunPelletProjectile* SpawnedPellet = World->SpawnActor<ASimShotgunPelletProjectile>(
			PelletProjectileClass,
			SpawnLocation,
			SpawnRotation,
			SpawnParams
		);

		if (!SpawnedPellet)
		{
			UE_LOG(LogTemp, Warning, TEXT("Shotgun failed to spawn pellet %d."), PelletIndex);
			continue;
		}

		TArray<AActor*> ActorsToIgnore;

		if (AActor* OwnerActor = GetOwner())
		{
			ActorsToIgnore.Add(OwnerActor);
		}

		ActorsToIgnore.Add(this);

		for (ASimShotgunPelletProjectile* PreviousPellet : SpawnedPellets)
		{
			if (!PreviousPellet)
			{
				continue;
			}

			ActorsToIgnore.Add(PreviousPellet);
			PreviousPellet->AddIgnoredActor(SpawnedPellet);
		}

		SpawnedPellet->InitializePellet(PelletDirection, ActorsToIgnore);

		SpawnedPellets.Add(SpawnedPellet);
		SpawnedPelletCount++;

		OnShotgunPelletSpawned(SpawnedPellet);
	}

	OnShotgunFired(SpawnedPelletCount);

	UE_LOG(
		LogTemp,
		Log,
		TEXT("Shotgun fired. Requested pellets: %d | Spawned pellets: %d | Ammo left: %d"),
		PelletCount,
		SpawnedPelletCount,
		GetCurrentAmmo()
	);
}