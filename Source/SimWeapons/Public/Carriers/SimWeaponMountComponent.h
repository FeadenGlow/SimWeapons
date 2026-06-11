// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "Core/SimWeaponBase.h"
#include "SimWeaponMountComponent.generated.h"

class USimWeaponCarrierComponent;

UCLASS(ClassGroup = (SimWeapons), meta = (BlueprintSpawnableComponent))
class SIMWEAPONS_API USimWeaponMountComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	USimWeaponMountComponent();

	UFUNCTION(BlueprintCallable, Category = "Sim Weapons|Mount")
	ASimWeaponBase* SpawnWeapon();

	// Old simple fire method. Kept for compatibility with existing Blueprints.
	UFUNCTION(BlueprintCallable, Category = "Sim Weapons|Mount")
	void FireMountedWeapon();

	// Preferred fire method for battle systems.
	// Returns true if the weapon was fired.
	// ReloadTime is the delay in seconds before this weapon should be fired again.
	UFUNCTION(BlueprintCallable, Category = "Sim Weapons|Mount")
	bool FireMountedWeaponWithReloadTime(float& ReloadTime);

	UFUNCTION(BlueprintCallable, Category = "Sim Weapons|Mount")
	bool CanFireMountedWeapon() const;

	UFUNCTION(BlueprintCallable, Category = "Sim Weapons|Mount")
	float GetMountedWeaponReloadTime() const;

	UFUNCTION(BlueprintCallable, Category = "Sim Weapons|Mount")
	ASimWeaponBase* GetSpawnedWeapon() const;

	UFUNCTION(BlueprintCallable, Category = "Sim Weapons|Mount")
	USimWeaponCarrierComponent* GetCarrierComponent() const;

protected:
	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Mount")
	TSubclassOf<ASimWeaponBase> WeaponClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Mount")
	bool bSpawnWeaponOnBeginPlay = true;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sim Weapons|Mount")
	ASimWeaponBase* SpawnedWeapon = nullptr;

private:
	bool bWeaponSlotRegistered = false;

	int32 RegisteredWeaponSlotCost = 0;
};