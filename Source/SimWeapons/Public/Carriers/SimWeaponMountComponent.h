// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "Core/SimWeaponBase.h"
#include "SimWeaponMountComponent.generated.h"

UCLASS(ClassGroup = (SimWeapons), meta = (BlueprintSpawnableComponent))
class SIMWEAPONS_API USimWeaponMountComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	USimWeaponMountComponent();

	// Spawns weapon actor and attaches it to this mount component.
	UFUNCTION(BlueprintCallable, Category = "Sim Weapons|Mount")
	ASimWeaponBase* SpawnWeapon();

	// Calls Fire() on the spawned weapon.
	UFUNCTION(BlueprintCallable, Category = "Sim Weapons|Mount")
	void FireMountedWeapon();

	// Returns currently spawned weapon.
	UFUNCTION(BlueprintCallable, Category = "Sim Weapons|Mount")
	ASimWeaponBase* GetSpawnedWeapon() const;

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	// Weapon class that will be spawned on this mount.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Mount")
	TSubclassOf<ASimWeaponBase> WeaponClass;

	// If true, weapon will be spawned automatically on BeginPlay.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Mount")
	bool bSpawnWeaponOnBeginPlay = true;

	// Spawned weapon instance.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sim Weapons|Mount")
	ASimWeaponBase* SpawnedWeapon = nullptr;
};