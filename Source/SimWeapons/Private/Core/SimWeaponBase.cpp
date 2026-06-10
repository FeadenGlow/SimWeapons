// Fill out your copyright notice in the Description page of Project Settings.

#include "Core/SimWeaponBase.h"

#include "Engine/World.h"

ASimWeaponBase::ASimWeaponBase()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ASimWeaponBase::BeginPlay()
{
	Super::BeginPlay();

	CurrentAmmo = MaxAmmo;
	LastFireTime = -100000.0f;

	NotifyAmmoChanged();
}

void ASimWeaponBase::Fire_Implementation()
{
	TryConsumeAmmo();
}

bool ASimWeaponBase::CanFire() const
{
	return CurrentAmmo > 0 && !IsFireOnCooldown();
}

bool ASimWeaponBase::IsFireOnCooldown() const
{
	if (!bUseFireCooldown || FireCooldown <= 0.0f)
	{
		return false;
	}

	const UWorld* World = GetWorld();

	if (!World)
	{
		return false;
	}

	const float CurrentTime = World->GetTimeSeconds();
	const float TimeSinceLastFire = CurrentTime - LastFireTime;

	return TimeSinceLastFire < FireCooldown;
}

float ASimWeaponBase::GetRemainingFireCooldown() const
{
	if (!IsFireOnCooldown())
	{
		return 0.0f;
	}

	const UWorld* World = GetWorld();

	if (!World)
	{
		return 0.0f;
	}

	const float CurrentTime = World->GetTimeSeconds();
	const float TimeSinceLastFire = CurrentTime - LastFireTime;
	const float RemainingCooldown = FireCooldown - TimeSinceLastFire;

	return FMath::Max(0.0f, RemainingCooldown);
}

float ASimWeaponBase::GetBatteryCost() const
{
	return BatteryCost;
}

float ASimWeaponBase::GetFireCooldown() const
{
	return FireCooldown;
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

	StartFireCooldown();
	NotifyAmmoChanged();

	return true;
}

void ASimWeaponBase::StartFireCooldown()
{
	if (!bUseFireCooldown)
	{
		return;
	}

	UWorld* World = GetWorld();

	if (!World)
	{
		return;
	}

	LastFireTime = World->GetTimeSeconds();
}

void ASimWeaponBase::NotifyAmmoChanged()
{
	OnAmmoChanged(CurrentAmmo, MaxAmmo);

	if (CurrentAmmo <= 0)
	{
		OnOutOfAmmo();
	}
}