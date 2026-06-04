// Fill out your copyright notice in the Description page of Project Settings.

#include "Carriers/SimWeaponMountComponent.h"

#include "Engine/World.h"

// Sets default values for this component's properties
USimWeaponMountComponent::USimWeaponMountComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

// Called when the game starts
void USimWeaponMountComponent::BeginPlay()
{
	Super::BeginPlay();

	if (bSpawnWeaponOnBeginPlay)
	{
		SpawnWeapon();
	}
}

ASimWeaponBase* USimWeaponMountComponent::SpawnWeapon()
{
	if (SpawnedWeapon)
	{
		return SpawnedWeapon;
	}

	if (!WeaponClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("SimWeaponMountComponent: WeaponClass is not set."));
		return nullptr;
	}

	UWorld* World = GetWorld();

	if (!World)
	{
		return nullptr;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = GetOwner();
	SpawnParams.Instigator = Cast<APawn>(GetOwner());
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	SpawnedWeapon = World->SpawnActor<ASimWeaponBase>(
		WeaponClass,
		GetComponentLocation(),
		GetComponentRotation(),
		SpawnParams
	);

	if (!SpawnedWeapon)
	{
		UE_LOG(LogTemp, Warning, TEXT("SimWeaponMountComponent: Failed to spawn weapon."));
		return nullptr;
	}

	SpawnedWeapon->AttachToComponent(this, FAttachmentTransformRules::SnapToTargetNotIncludingScale);

	return SpawnedWeapon;
}

void USimWeaponMountComponent::FireMountedWeapon()
{
	if (!SpawnedWeapon)
	{
		SpawnWeapon();
	}

	if (!SpawnedWeapon)
	{
		return;
	}

	SpawnedWeapon->Fire();
}

ASimWeaponBase* USimWeaponMountComponent::GetSpawnedWeapon() const
{
	return SpawnedWeapon;
}