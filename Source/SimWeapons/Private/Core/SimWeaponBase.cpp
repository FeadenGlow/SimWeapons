// Fill out your copyright notice in the Description page of Project Settings.

#include "Core/SimWeaponBase.h"

// Sets default values
ASimWeaponBase::ASimWeaponBase()
{
	// Base weapon does not need Tick by default.
	PrimaryActorTick.bCanEverTick = false;
}

// Called when the game starts or when spawned
void ASimWeaponBase::BeginPlay()
{
	Super::BeginPlay();

	CurrentAmmo = MaxAmmo;
}

void ASimWeaponBase::Fire_Implementation()
{
	// Base weapon only consumes ammo.
	// Rocket and Bomb classes will override this method later.
	TryConsumeAmmo();
}

bool ASimWeaponBase::CanFire() const
{
	return CurrentAmmo > 0;
}

float ASimWeaponBase::GetBatteryCost() const
{
	return BatteryCost;
}

int32 ASimWeaponBase::GetCurrentAmmo() const
{
	return CurrentAmmo;
}

int32 ASimWeaponBase::GetMaxAmmo() const
{
	return MaxAmmo;
}

bool ASimWeaponBase::TryConsumeAmmo()
{
	if (!CanFire())
	{
		return false;
	}

	CurrentAmmo--;

	return true;
}