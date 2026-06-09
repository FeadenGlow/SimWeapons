// Fill out your copyright notice in the Description page of Project Settings.

#include "Weapons/Bomb/SimBombWeapon.h"

#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Weapons/Bomb/SimBombProjectile.h"

// Sets default values
ASimBombWeapon::ASimBombWeapon()
{
	BatteryCost = 5.0f;
	MaxAmmo = 1;

	// Bomb uses only one weapon slot.
	WeaponSlotCost = 1;
}

void ASimBombWeapon::Fire_Implementation()
{
	if (!CanFire())
	{
		UE_LOG(LogTemp, Warning, TEXT("Bomb weapon cannot drop bomb: no ammo."));
		return;
	}

	if (!BombProjectileClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("Bomb weapon cannot drop bomb: BombProjectileClass is not set."));
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
		GetActorForwardVector() * SpawnForwardOffset -
		GetActorUpVector() * SpawnDownOffset;

	const FRotator SpawnRotation = GetActorRotation();

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = GetOwner() ? GetOwner() : this;
	SpawnParams.Instigator = Cast<APawn>(GetOwner());
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	ASimBombProjectile* SpawnedProjectile = World->SpawnActor<ASimBombProjectile>(
		BombProjectileClass,
		SpawnLocation,
		SpawnRotation,
		SpawnParams
	);

	if (!SpawnedProjectile)
	{
		UE_LOG(LogTemp, Warning, TEXT("Bomb weapon failed to spawn projectile."));
		return;
	}

	OnBombDropped(SpawnedProjectile);

	UE_LOG(LogTemp, Log, TEXT("Bomb dropped. Ammo left: %d"), GetCurrentAmmo());
}