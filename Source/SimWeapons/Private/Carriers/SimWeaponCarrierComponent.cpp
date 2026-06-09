// Fill out your copyright notice in the Description page of Project Settings.

#include "Carriers/SimWeaponCarrierComponent.h"

USimWeaponCarrierComponent::USimWeaponCarrierComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	UsedWeaponSlots = 0;
}

// Called when the game starts
void USimWeaponCarrierComponent::BeginPlay()
{
	Super::BeginPlay();

	CurrentHealth = MaxHealth;
	CurrentEnergy = MaxEnergy;

	// Do not reset UsedWeaponSlots here.
	// Weapon mounts can register themselves during BeginPlay, and component BeginPlay order is not guaranteed.
}

bool USimWeaponCarrierComponent::IsAlive() const
{
	return CurrentHealth > 0.0f;
}

bool USimWeaponCarrierComponent::HasEnoughEnergy(float EnergyAmount) const
{
	return CurrentEnergy >= EnergyAmount;
}

bool USimWeaponCarrierComponent::ConsumeEnergy(float EnergyAmount)
{
	if (EnergyAmount <= 0.0f)
	{
		return true;
	}

	if (!HasEnoughEnergy(EnergyAmount))
	{
		return false;
	}

	CurrentEnergy -= EnergyAmount;

	if (CurrentEnergy < 0.0f)
	{
		CurrentEnergy = 0.0f;
	}

	return true;
}

void USimWeaponCarrierComponent::RestoreEnergy(float EnergyAmount)
{
	if (EnergyAmount <= 0.0f)
	{
		return;
	}

	CurrentEnergy += EnergyAmount;

	if (CurrentEnergy > MaxEnergy)
	{
		CurrentEnergy = MaxEnergy;
	}
}

void USimWeaponCarrierComponent::ApplyCarrierDamage(float DamageAmount)
{
	if (DamageAmount <= 0.0f || !IsAlive())
	{
		return;
	}

	CurrentHealth -= DamageAmount;

	if (CurrentHealth < 0.0f)
	{
		CurrentHealth = 0.0f;
	}

	if (CurrentHealth <= 0.0f && bDestroyOwnerOnDeath)
	{
		if (AActor* OwnerActor = GetOwner())
		{
			OwnerActor->Destroy();
		}
	}
}

bool USimWeaponCarrierComponent::CanAttachWeapon(int32 RequiredSlots) const
{
	const int32 SafeRequiredSlots = FMath::Max(1, RequiredSlots);

	return UsedWeaponSlots + SafeRequiredSlots <= MaxWeaponSlots;
}

bool USimWeaponCarrierComponent::RegisterWeapon(int32 RequiredSlots)
{
	const int32 SafeRequiredSlots = FMath::Max(1, RequiredSlots);

	if (!CanAttachWeapon(SafeRequiredSlots))
	{
		return false;
	}

	UsedWeaponSlots += SafeRequiredSlots;

	UE_LOG(
		LogTemp,
		Log,
		TEXT("Weapon registered. Required slots: %d | Used slots: %d | Max slots: %d"),
		SafeRequiredSlots,
		UsedWeaponSlots,
		MaxWeaponSlots
	);

	return true;
}

void USimWeaponCarrierComponent::UnregisterWeapon(int32 ReleasedSlots)
{
	const int32 SafeReleasedSlots = FMath::Max(1, ReleasedSlots);

	UsedWeaponSlots -= SafeReleasedSlots;

	if (UsedWeaponSlots < 0)
	{
		UsedWeaponSlots = 0;
	}

	UE_LOG(
		LogTemp,
		Log,
		TEXT("Weapon unregistered. Released slots: %d | Used slots: %d | Max slots: %d"),
		SafeReleasedSlots,
		UsedWeaponSlots,
		MaxWeaponSlots
	);
}

float USimWeaponCarrierComponent::GetCurrentHealth() const
{
	return CurrentHealth;
}

float USimWeaponCarrierComponent::GetMaxHealth() const
{
	return MaxHealth;
}

float USimWeaponCarrierComponent::GetCurrentEnergy() const
{
	return CurrentEnergy;
}

float USimWeaponCarrierComponent::GetMaxEnergy() const
{
	return MaxEnergy;
}

int32 USimWeaponCarrierComponent::GetMaxWeaponSlots() const
{
	return MaxWeaponSlots;
}

int32 USimWeaponCarrierComponent::GetUsedWeaponSlots() const
{
	return UsedWeaponSlots;
}