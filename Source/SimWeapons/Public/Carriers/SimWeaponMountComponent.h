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

	UFUNCTION(BlueprintCallable, Category = "Sim Weapons|Mount")
	void FireMountedWeapon();

	UFUNCTION(BlueprintCallable, Category = "Sim Weapons|Mount")
	bool CanFireMountedWeapon() const;

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