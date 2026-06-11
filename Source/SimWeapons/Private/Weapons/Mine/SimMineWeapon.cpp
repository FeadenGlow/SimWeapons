// Fill out your copyright notice in the Description page of Project Settings.

#include "Weapons/Mine/SimMineWeapon.h"

#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Weapons/Mine/SimMineActor.h"

ASimMineWeapon::ASimMineWeapon()
{
	BatteryCost = 0.0f;
	MaxAmmo = 3;

	// Mine weapon uses one weapon slot.
	WeaponSlotCost = 1;

	SpawnForwardOffset = 50.0f;
	SpawnDownOffset = 80.0f;
}

void ASimMineWeapon::Fire_Implementation()
{
	if (!CanFire())
	{
		UE_LOG(LogTemp, Warning, TEXT("Mine weapon cannot place mine: no ammo or weapon is on cooldown."));
		return;
	}

	if (!MineClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("Mine weapon cannot place mine: MineClass is not set."));
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
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	ASimMineActor* SpawnedMine = World->SpawnActor<ASimMineActor>(
		MineClass,
		SpawnLocation,
		SpawnRotation,
		SpawnParams
	);

	if (!SpawnedMine)
	{
		UE_LOG(LogTemp, Warning, TEXT("Mine weapon failed to spawn mine."));
		return;
	}

	OnMinePlaced(SpawnedMine);

	UE_LOG(LogTemp, Log, TEXT("Mine placed. Ammo left: %d"), GetCurrentAmmo());
}