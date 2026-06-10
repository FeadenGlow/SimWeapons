// Copyright Epic Games, Inc. All Rights Reserved.

#include "Carriers/GenericUGV/SimGenericUGVCarrier.h"

USimGenericUGVCarrier::USimGenericUGVCarrier()
{
	PrimaryComponentTick.bCanEverTick = true;

	MaxHealth = 300.0f;
	CurrentHealth = 300.0f;
	MaxEnergy = 200.0f;
	CurrentEnergy = 200.0f;
	MaxWeaponSlots = 4;
}

void USimGenericUGVCarrier::BeginPlay()
{
	Super::BeginPlay();
	bEnergyDepletedEventFired = false;

	UE_LOG(LogTemp, Log,
		TEXT("USimGenericUGVCarrier: Initialized. Health=%.1f, Energy=%.1f, Armor=%.1f, RegenRate=%.1f, Slots=%d"),
		MaxHealth, MaxEnergy, ArmorRating, EnergyRegenRate, MaxWeaponSlots);
}

void USimGenericUGVCarrier::TickComponent(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!IsAlive())
	{
		return;
	}

	if (EnergyRegenRate > 0.0f)
	{
		RestoreEnergy(EnergyRegenRate * DeltaTime);
	}

	if (GetCurrentEnergy() <= 0.0f)
	{
		if (!bEnergyDepletedEventFired)
		{
			bEnergyDepletedEventFired = true;
			UE_LOG(LogTemp, Warning, TEXT("USimGenericUGVCarrier: Energy depleted on '%s'."),
				GetOwner() ? *GetOwner()->GetName() : TEXT("Unknown"));
			OnEnergyDepleted();
		}
	}
	else
	{
		bEnergyDepletedEventFired = false;
	}
}

void USimGenericUGVCarrier::ApplyCarrierDamage(float DamageAmount)
{
	if (DamageAmount <= 0.0f || !IsAlive())
	{
		return;
	}

	const float EffectiveDamage = FMath::Max(0.0f, DamageAmount - ArmorRating);

	UE_LOG(LogTemp, Log,
		TEXT("USimGenericUGVCarrier: Incoming=%.1f, Armor=%.1f, Effective=%.1f"),
		DamageAmount, ArmorRating, EffectiveDamage);

	if (EffectiveDamage >= GetCurrentHealth())
	{
		OnCarrierDestroyed();
	}

	Super::ApplyCarrierDamage(EffectiveDamage);
}

bool USimGenericUGVCarrier::CanFireWhileMoving() const { return bCanFireWhileMoving; }
float USimGenericUGVCarrier::GetArmorRating() const { return ArmorRating; }
float USimGenericUGVCarrier::GetEnergyRegenRate() const { return EnergyRegenRate; }
