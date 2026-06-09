// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SimWeaponCarrierComponent.generated.h"

UCLASS(Blueprintable, BlueprintType, ClassGroup = (SimWeapons), meta = (BlueprintSpawnableComponent))
class SIMWEAPONS_API USimWeaponCarrierComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USimWeaponCarrierComponent();

	UFUNCTION(BlueprintCallable, Category = "Sim Weapons|Carrier")
	bool IsAlive() const;

	UFUNCTION(BlueprintCallable, Category = "Sim Weapons|Carrier")
	bool HasEnoughEnergy(float EnergyAmount) const;

	UFUNCTION(BlueprintCallable, Category = "Sim Weapons|Carrier")
	bool ConsumeEnergy(float EnergyAmount);

	UFUNCTION(BlueprintCallable, Category = "Sim Weapons|Carrier")
	void RestoreEnergy(float EnergyAmount);

	UFUNCTION(BlueprintCallable, Category = "Sim Weapons|Carrier")
	void ApplyCarrierDamage(float DamageAmount);

	UFUNCTION(BlueprintCallable, Category = "Sim Weapons|Carrier")
	bool CanAttachWeapon(int32 RequiredSlots = 1) const;

	UFUNCTION(BlueprintCallable, Category = "Sim Weapons|Carrier")
	bool RegisterWeapon(int32 RequiredSlots = 1);

	UFUNCTION(BlueprintCallable, Category = "Sim Weapons|Carrier")
	void UnregisterWeapon(int32 ReleasedSlots = 1);

	UFUNCTION(BlueprintCallable, Category = "Sim Weapons|Carrier")
	float GetCurrentHealth() const;

	UFUNCTION(BlueprintCallable, Category = "Sim Weapons|Carrier")
	float GetMaxHealth() const;

	UFUNCTION(BlueprintCallable, Category = "Sim Weapons|Carrier")
	float GetCurrentEnergy() const;

	UFUNCTION(BlueprintCallable, Category = "Sim Weapons|Carrier")
	float GetMaxEnergy() const;

	UFUNCTION(BlueprintCallable, Category = "Sim Weapons|Carrier")
	int32 GetMaxWeaponSlots() const;

	UFUNCTION(BlueprintCallable, Category = "Sim Weapons|Carrier")
	int32 GetUsedWeaponSlots() const;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Carrier")
	float MaxHealth = 100.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sim Weapons|Carrier")
	float CurrentHealth = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Carrier")
	float MaxEnergy = 100.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sim Weapons|Carrier")
	float CurrentEnergy = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Carrier")
	int32 MaxWeaponSlots = 1;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sim Weapons|Carrier")
	int32 UsedWeaponSlots = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Carrier")
	bool bDestroyOwnerOnDeath = true;
};