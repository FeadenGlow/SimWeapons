// Fill out your copyright notice in the Description page of Project Settings.

#include "Weapons/Rocket/SimRocketWeapon.h"

#include "Engine/World.h"
#include "Weapons/Rocket/SimRocketProjectile.h"

ASimRocketWeapon::ASimRocketWeapon()
{
	BatteryCost = 20.0f;
	MaxAmmo = 1;
	WeaponSlotCost = 2;
}

void ASimRocketWeapon::Fire_Implementation()
{
	if (!CanFire())
	{
		UE_LOG(LogTemp, Warning, TEXT("Rocket weapon cannot fire: no ammo."));
		return;
	}

	if (!RocketProjectileClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("Rocket weapon cannot fire: RocketProjectileClass is not set."));
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

	const FVector SpawnLocation = GetActorLocation() + GetActorForwardVector() * SpawnForwardOffset;
	const FRotator SpawnRotation = GetActorRotation();

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = GetOwner() ? GetOwner() : this;
	SpawnParams.Instigator = Cast<APawn>(GetOwner());
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	ASimRocketProjectile* SpawnedProjectile = World->SpawnActor<ASimRocketProjectile>(
		RocketProjectileClass,
		SpawnLocation,
		SpawnRotation,
		SpawnParams
	);

	if (!SpawnedProjectile)
	{
		UE_LOG(LogTemp, Warning, TEXT("Rocket weapon failed to spawn projectile."));
		return;
	}

	OnRocketFired(SpawnedProjectile);

	UE_LOG(LogTemp, Log, TEXT("Rocket fired. Ammo left: %d"), GetCurrentAmmo());
}