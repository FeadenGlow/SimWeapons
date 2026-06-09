// Fill out your copyright notice in the Description page of Project Settings.

#include "Carriers/SimWeaponMountComponent.h"

#include "Carriers/SimWeaponCarrierComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"

USimWeaponMountComponent::USimWeaponMountComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void USimWeaponMountComponent::BeginPlay()
{
	Super::BeginPlay();

	if (bSpawnWeaponOnBeginPlay)
	{
		SpawnWeapon();
	}
}

void USimWeaponMountComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (USimWeaponCarrierComponent* CarrierComponent = GetCarrierComponent())
	{
		if (bWeaponSlotRegistered)
		{
			CarrierComponent->UnregisterWeapon(RegisteredWeaponSlotCost);
			bWeaponSlotRegistered = false;
			RegisteredWeaponSlotCost = 0;
		}
	}

	Super::EndPlay(EndPlayReason);
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

	USimWeaponCarrierComponent* CarrierComponent = GetCarrierComponent();

	if (!CarrierComponent)
	{
		UE_LOG(LogTemp, Warning, TEXT("SimWeaponMountComponent: Owner actor does not have SimWeaponCarrierComponent."));
		return nullptr;
	}

	const ASimWeaponBase* WeaponDefaultObject = WeaponClass->GetDefaultObject<ASimWeaponBase>();
	const int32 WeaponSlotCost = WeaponDefaultObject ? WeaponDefaultObject->GetWeaponSlotCost() : 1;

	if (!CarrierComponent->CanAttachWeapon(WeaponSlotCost))
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("SimWeaponMountComponent: Carrier has no free weapon slots. Required: %d, Used: %d, Max: %d"),
			WeaponSlotCost,
			CarrierComponent->GetUsedWeaponSlots(),
			CarrierComponent->GetMaxWeaponSlots()
		);

		return nullptr;
	}

	if (!CarrierComponent->RegisterWeapon(WeaponSlotCost))
	{
		UE_LOG(LogTemp, Warning, TEXT("SimWeaponMountComponent: Failed to register weapon slot."));
		return nullptr;
	}

	bWeaponSlotRegistered = true;
	RegisteredWeaponSlotCost = WeaponSlotCost;

	UWorld* World = GetWorld();

	if (!World)
	{
		CarrierComponent->UnregisterWeapon(RegisteredWeaponSlotCost);
		bWeaponSlotRegistered = false;
		RegisteredWeaponSlotCost = 0;
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
		CarrierComponent->UnregisterWeapon(RegisteredWeaponSlotCost);
		bWeaponSlotRegistered = false;
		RegisteredWeaponSlotCost = 0;

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

	USimWeaponCarrierComponent* CarrierComponent = GetCarrierComponent();

	if (!CarrierComponent)
	{
		UE_LOG(LogTemp, Warning, TEXT("SimWeaponMountComponent: Cannot fire because owner actor does not have SimWeaponCarrierComponent."));
		return;
	}

	if (!CarrierComponent->IsAlive())
	{
		UE_LOG(LogTemp, Warning, TEXT("SimWeaponMountComponent: Cannot fire because carrier is destroyed."));
		return;
	}

	if (!SpawnedWeapon->CanFire())
	{
		UE_LOG(LogTemp, Warning, TEXT("SimWeaponMountComponent: Mounted weapon cannot fire."));
		return;
	}

	const float EnergyCost = SpawnedWeapon->GetBatteryCost();

	if (!CarrierComponent->ConsumeEnergy(EnergyCost))
	{
		UE_LOG(LogTemp, Warning, TEXT("SimWeaponMountComponent: Not enough energy to fire weapon."));
		return;
	}

	SpawnedWeapon->Fire();
}

bool USimWeaponMountComponent::CanFireMountedWeapon() const
{
	if (!SpawnedWeapon)
	{
		return false;
	}

	const USimWeaponCarrierComponent* CarrierComponent = GetCarrierComponent();

	if (!CarrierComponent)
	{
		return false;
	}

	if (!CarrierComponent->IsAlive())
	{
		return false;
	}

	if (!SpawnedWeapon->CanFire())
	{
		return false;
	}

	return CarrierComponent->HasEnoughEnergy(SpawnedWeapon->GetBatteryCost());
}

ASimWeaponBase* USimWeaponMountComponent::GetSpawnedWeapon() const
{
	return SpawnedWeapon;
}

USimWeaponCarrierComponent* USimWeaponMountComponent::GetCarrierComponent() const
{
	AActor* OwnerActor = GetOwner();

	if (!OwnerActor)
	{
		return nullptr;
	}

	return OwnerActor->FindComponentByClass<USimWeaponCarrierComponent>();
}