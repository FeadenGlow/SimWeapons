// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SimWeaponBase.generated.h"

UCLASS(Blueprintable, BlueprintType)
class SIMWEAPONS_API ASimWeaponBase : public AActor
{
	GENERATED_BODY()

public:
	ASimWeaponBase();

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Sim Weapons")
	void Fire();

	virtual void Fire_Implementation();

	UFUNCTION(BlueprintCallable, Category = "Sim Weapons")
	bool CanFire() const;

	UFUNCTION(BlueprintCallable, Category = "Sim Weapons")
	bool IsFireOnCooldown() const;

	UFUNCTION(BlueprintCallable, Category = "Sim Weapons")
	float GetRemainingFireCooldown() const;

	UFUNCTION(BlueprintCallable, Category = "Sim Weapons")
	float GetBatteryCost() const;

	UFUNCTION(BlueprintCallable, Category = "Sim Weapons")
	float GetFireCooldown() const;

	UFUNCTION(BlueprintCallable, Category = "Sim Weapons")
	int32 GetCurrentAmmo() const;

	UFUNCTION(BlueprintCallable, Category = "Sim Weapons")
	int32 GetMaxAmmo() const;

	UFUNCTION(BlueprintCallable, Category = "Sim Weapons")
	int32 GetWeaponSlotCost() const;

protected:
	virtual void BeginPlay() override;

	bool TryConsumeAmmo();

	void StartFireCooldown();

	void NotifyAmmoChanged();

	UFUNCTION(BlueprintImplementableEvent, Category = "Sim Weapons|Ammo")
	void OnAmmoChanged(int32 NewCurrentAmmo, int32 NewMaxAmmo);

	UFUNCTION(BlueprintImplementableEvent, Category = "Sim Weapons|Ammo")
	void OnOutOfAmmo();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons")
	float BatteryCost = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons")
	int32 MaxAmmo = 1;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sim Weapons")
	int32 CurrentAmmo = 1;

	// How many weapon slots this weapon uses on the carrier.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons", meta = (ClampMin = "1"))
	int32 WeaponSlotCost = 1;

	// Delay between shots in seconds.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Fire Rate", meta = (ClampMin = "0.0"))
	float FireCooldown = 0.5f;

	// If false, weapon can fire without delay.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Fire Rate")
	bool bUseFireCooldown = true;

private:
	float LastFireTime = -100000.0f;
};