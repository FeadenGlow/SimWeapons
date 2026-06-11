// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Carriers/SimWeaponCarrierComponent.h"
#include "SimDroneCarrier.generated.h"

class ASimWeaponBase;
class USimWeaponMountComponent;

UCLASS(Blueprintable, BlueprintType, ClassGroup = (SimWeapons), meta = (BlueprintSpawnableComponent))
class SIMWEAPONS_API USimDroneCarrier : public USimWeaponCarrierComponent
{
	GENERATED_BODY()

public:
	USimDroneCarrier();

	UFUNCTION(BlueprintCallable, Category = "Sim Weapons|Drone|Energy")
	float GetEnergyPercent() const;

	UFUNCTION(BlueprintCallable, Category = "Sim Weapons|Drone|Energy")
	void SetEnergy(float NewEnergy);

	UFUNCTION(BlueprintCallable, Category = "Sim Weapons|Drone|Health")
	float GetHealthPercent() const;

	UFUNCTION(BlueprintCallable, Category = "Sim Weapons|Drone|Health")
	void SetHealth(float NewHealth);

	UFUNCTION(BlueprintCallable, Category = "Sim Weapons|Drone|Health")
	void ApplyDamage(float DamageAmount);

	UFUNCTION(BlueprintCallable, Category = "Sim Weapons|Drone|Health")
	bool IsDestroyed() const;

	UFUNCTION(BlueprintCallable, Category = "Sim Weapons|Drone|Weapons")
	int32 GetMountedWeaponAmmo(int32 MountIndex) const;

	UFUNCTION(BlueprintCallable, Category = "Sim Weapons|Drone|Weapons")
	int32 GetMountedWeaponCurrentAmmo(int32 MountIndex) const;

	UFUNCTION(BlueprintCallable, Category = "Sim Weapons|Drone|Weapons")
	int32 GetMountedWeaponMaxAmmo(int32 MountIndex) const;

	UFUNCTION(BlueprintCallable, Category = "Sim Weapons|Drone|Weapons")
	int32 GetMountedWeaponSlotCost(int32 MountIndex) const;

	UFUNCTION(BlueprintCallable, Category = "Sim Weapons|Drone|Sensors")
	int32 GetMaxSensorSlots() const;

	UFUNCTION(BlueprintCallable, Category = "Sim Weapons|Drone|Sensors")
	int32 GetUsedSensorSlots() const;

	UFUNCTION(BlueprintCallable, Category = "Sim Weapons|Drone|Sensors")
	bool CanAttachSensor(int32 RequiredSlots = 1) const;

	UFUNCTION(BlueprintCallable, Category = "Sim Weapons|Drone|Sensors")
	bool RegisterSensor(int32 RequiredSlots = 1);

	UFUNCTION(BlueprintCallable, Category = "Sim Weapons|Drone|Sensors")
	void UnregisterSensor(int32 ReleasedSlots = 1);

	UFUNCTION(BlueprintCallable, Category = "Sim Weapons|Drone|Energy Costs")
	float GetVisionRadarSensorCost() const;

	UFUNCTION(BlueprintCallable, Category = "Sim Weapons|Drone|Energy Costs")
	float GetRadioMessageSendCost() const;

	UFUNCTION(BlueprintCallable, Category = "Sim Weapons|Drone|Energy Costs")
	float GetReceiverCostPerSecond() const;

	UFUNCTION(BlueprintCallable, Category = "Sim Weapons|Drone|Energy Costs")
	float GetMovementCostPer100CmPerSecond() const;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Drone|Sensors", meta = (ClampMin = "0"))
	int32 MaxSensorSlots = 3;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sim Weapons|Drone|Sensors")
	int32 UsedSensorSlots = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Drone|Energy Costs", meta = (ClampMin = "0.0"))
	float VisionRadarSensorCost = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Drone|Energy Costs", meta = (ClampMin = "0.0"))
	float RadioMessageSendCost = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Drone|Energy Costs", meta = (ClampMin = "0.0"))
	float ReceiverCostPerSecond = 0.1f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Drone|Energy Costs", meta = (ClampMin = "0.0"))
	float MovementCostPer100CmPerSecond = 0.1f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Drone|Energy")
	float LowEnergyThreshold = 20.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Drone|Energy")
	float CriticalEnergyThreshold = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Drone|Movement")
	float RecommendedCruiseSpeedCmPerSecond = 300.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Drone|Movement")
	float RecommendedCombatSpeedCmPerSecond = 600.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Drone|Movement")
	float RecommendedMaxSpeedCmPerSecond = 800.0f;

private:
	ASimWeaponBase* GetMountedWeaponByIndex(int32 MountIndex) const;

	TArray<USimWeaponMountComponent*> GetWeaponMounts() const;
};