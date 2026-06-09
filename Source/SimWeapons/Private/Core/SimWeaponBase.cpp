// Fill out your copyright notice in the Description page of Project Settings.

#include "Core/SimWeaponBase.h"

ASimWeaponBase::ASimWeaponBase()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ASimWeaponBase::BeginPlay()
{
	Super::BeginPlay();

	CurrentAmmo = MaxAmmo;
}

void ASimWeaponBase::Fire_Implementation()
{
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

int32 ASimWeaponBase::GetWeaponSlotCost() const
{
	return FMath::Max(1, WeaponSlotCost);
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