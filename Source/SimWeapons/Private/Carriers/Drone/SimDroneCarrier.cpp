// Copyright Epic Games, Inc. All Rights Reserved.

#include "Carriers/Drone/SimDroneCarrier.h"

#include "Carriers/SimWeaponMountComponent.h"
#include "Core/SimWeaponBase.h"
#include "GameFramework/Actor.h"

USimDroneCarrier::USimDroneCarrier()
{
	PrimaryComponentTick.bCanEverTick = false;

	MaxHealth = 100.0f;
	CurrentHealth = MaxHealth;

	MaxEnergy = 100.0f;
	CurrentEnergy = MaxEnergy;

	MaxWeaponSlots = 3;
	UsedWeaponSlots = 0;

	MaxSensorSlots = 3;
	UsedSensorSlots = 0;

	VisionRadarSensorCost = 1.0f;
	RadioMessageSendCost = 1.0f;
	ReceiverCostPerSecond = 0.1f;
	MovementCostPer100CmPerSecond = 0.1f;

	LowEnergyThreshold = 20.0f;
	CriticalEnergyThreshold = 5.0f;

	RecommendedCruiseSpeedCmPerSecond = 300.0f;
	RecommendedCombatSpeedCmPerSecond = 600.0f;
	RecommendedMaxSpeedCmPerSecond = 800.0f;
}

void USimDroneCarrier::BeginPlay()
{
	Super::BeginPlay();

	MaxSensorSlots = FMath::Max(0, MaxSensorSlots);
	UsedSensorSlots = FMath::Clamp(UsedSensorSlots, 0, MaxSensorSlots);

	VisionRadarSensorCost = FMath::Max(0.0f, VisionRadarSensorCost);
	RadioMessageSendCost = FMath::Max(0.0f, RadioMessageSendCost);
	ReceiverCostPerSecond = FMath::Max(0.0f, ReceiverCostPerSecond);
	MovementCostPer100CmPerSecond = FMath::Max(0.0f, MovementCostPer100CmPerSecond);

	LowEnergyThreshold = FMath::Clamp(LowEnergyThreshold, 0.0f, MaxEnergy);
	CriticalEnergyThreshold = FMath::Clamp(CriticalEnergyThreshold, 0.0f, MaxEnergy);
}

float USimDroneCarrier::GetEnergyPercent() const
{
	if (MaxEnergy <= 0.0f)
	{
		return 0.0f;
	}

	return (CurrentEnergy / MaxEnergy) * 100.0f;
}

void USimDroneCarrier::SetEnergy(float NewEnergy)
{
	CurrentEnergy = FMath::Clamp(NewEnergy, 0.0f, MaxEnergy);
}

float USimDroneCarrier::GetHealthPercent() const
{
	if (MaxHealth <= 0.0f)
	{
		return 0.0f;
	}

	return (CurrentHealth / MaxHealth) * 100.0f;
}

void USimDroneCarrier::SetHealth(float NewHealth)
{
	const bool bWasAlive = IsAlive();

	CurrentHealth = FMath::Clamp(NewHealth, 0.0f, MaxHealth);

	if (bWasAlive && CurrentHealth <= 0.0f && bDestroyOwnerOnDeath)
	{
		if (AActor* OwnerActor = GetOwner())
		{
			OwnerActor->Destroy();
		}
	}
}

void USimDroneCarrier::ApplyDamage(float DamageAmount)
{
	ApplyCarrierDamage(DamageAmount);
}

bool USimDroneCarrier::IsDestroyed() const
{
	return !IsAlive();
}

int32 USimDroneCarrier::GetMountedWeaponAmmo(int32 MountIndex) const
{
	return GetMountedWeaponCurrentAmmo(MountIndex);
}

int32 USimDroneCarrier::GetMountedWeaponCurrentAmmo(int32 MountIndex) const
{
	const ASimWeaponBase* MountedWeapon = GetMountedWeaponByIndex(MountIndex);

	if (!MountedWeapon)
	{
		return -1;
	}

	return MountedWeapon->GetCurrentAmmo();
}

int32 USimDroneCarrier::GetMountedWeaponMaxAmmo(int32 MountIndex) const
{
	const ASimWeaponBase* MountedWeapon = GetMountedWeaponByIndex(MountIndex);

	if (!MountedWeapon)
	{
		return -1;
	}

	return MountedWeapon->GetMaxAmmo();
}

int32 USimDroneCarrier::GetMountedWeaponSlotCost(int32 MountIndex) const
{
	const ASimWeaponBase* MountedWeapon = GetMountedWeaponByIndex(MountIndex);

	if (!MountedWeapon)
	{
		return -1;
	}

	return MountedWeapon->GetWeaponSlotCost();
}

int32 USimDroneCarrier::GetMaxSensorSlots() const
{
	return MaxSensorSlots;
}

int32 USimDroneCarrier::GetUsedSensorSlots() const
{
	return UsedSensorSlots;
}

bool USimDroneCarrier::CanAttachSensor(int32 RequiredSlots) const
{
	const int32 SafeRequiredSlots = FMath::Max(1, RequiredSlots);

	return UsedSensorSlots + SafeRequiredSlots <= MaxSensorSlots;
}

bool USimDroneCarrier::RegisterSensor(int32 RequiredSlots)
{
	const int32 SafeRequiredSlots = FMath::Max(1, RequiredSlots);

	if (!CanAttachSensor(SafeRequiredSlots))
	{
		return false;
	}

	UsedSensorSlots += SafeRequiredSlots;
	UsedSensorSlots = FMath::Clamp(UsedSensorSlots, 0, MaxSensorSlots);

	UE_LOG(
		LogTemp,
		Log,
		TEXT("Drone sensor registered. Required slots: %d | Used sensor slots: %d | Max sensor slots: %d"),
		SafeRequiredSlots,
		UsedSensorSlots,
		MaxSensorSlots
	);

	return true;
}

void USimDroneCarrier::UnregisterSensor(int32 ReleasedSlots)
{
	const int32 SafeReleasedSlots = FMath::Max(1, ReleasedSlots);

	UsedSensorSlots -= SafeReleasedSlots;
	UsedSensorSlots = FMath::Clamp(UsedSensorSlots, 0, MaxSensorSlots);

	UE_LOG(
		LogTemp,
		Log,
		TEXT("Drone sensor unregistered. Released slots: %d | Used sensor slots: %d | Max sensor slots: %d"),
		SafeReleasedSlots,
		UsedSensorSlots,
		MaxSensorSlots
	);
}

float USimDroneCarrier::GetVisionRadarSensorCost() const
{
	return VisionRadarSensorCost;
}

float USimDroneCarrier::GetRadioMessageSendCost() const
{
	return RadioMessageSendCost;
}

float USimDroneCarrier::GetReceiverCostPerSecond() const
{
	return ReceiverCostPerSecond;
}

float USimDroneCarrier::GetMovementCostPer100CmPerSecond() const
{
	return MovementCostPer100CmPerSecond;
}

ASimWeaponBase* USimDroneCarrier::GetMountedWeaponByIndex(int32 MountIndex) const
{
	if (MountIndex < 0)
	{
		return nullptr;
	}

	const TArray<USimWeaponMountComponent*> WeaponMounts = GetWeaponMounts();

	if (!WeaponMounts.IsValidIndex(MountIndex))
	{
		return nullptr;
	}

	const USimWeaponMountComponent* WeaponMount = WeaponMounts[MountIndex];

	if (!WeaponMount)
	{
		return nullptr;
	}

	return WeaponMount->GetSpawnedWeapon();
}

TArray<USimWeaponMountComponent*> USimDroneCarrier::GetWeaponMounts() const
{
	TArray<USimWeaponMountComponent*> WeaponMounts;

	const AActor* OwnerActor = GetOwner();

	if (!OwnerActor)
	{
		return WeaponMounts;
	}

	OwnerActor->GetComponents<USimWeaponMountComponent>(WeaponMounts);

	return WeaponMounts;
}