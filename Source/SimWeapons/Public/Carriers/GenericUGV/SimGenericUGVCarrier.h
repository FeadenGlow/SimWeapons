// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Carriers/SimWeaponCarrierComponent.h"
#include "SimGenericUGVCarrier.generated.h"

UCLASS(Blueprintable, BlueprintType, ClassGroup = (SimWeapons), meta = (BlueprintSpawnableComponent))
class SIMWEAPONS_API USimGenericUGVCarrier : public USimWeaponCarrierComponent
{
	GENERATED_BODY()

public:
	USimGenericUGVCarrier();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

	virtual void ApplyCarrierDamage(float DamageAmount) override;

	UFUNCTION(BlueprintCallable, Category = "Sim Weapons|Carrier")
	bool CanFireWhileMoving() const;

	UFUNCTION(BlueprintCallable, Category = "Sim Weapons|Carrier")
	float GetArmorRating() const;

	UFUNCTION(BlueprintCallable, Category = "Sim Weapons|Carrier")
	float GetEnergyRegenRate() const;

protected:
	virtual void BeginPlay() override;

	UFUNCTION(BlueprintImplementableEvent, Category = "Sim Weapons|Carrier")
	void OnCarrierDestroyed();

	UFUNCTION(BlueprintImplementableEvent, Category = "Sim Weapons|Carrier")
	void OnEnergyDepleted();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Carrier", meta = (ClampMin = "0.0"))
	float ArmorRating = 20.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Carrier", meta = (ClampMin = "0.0"))
	float EnergyRegenRate = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Carrier")
	bool bCanFireWhileMoving = true;

private:
	bool bEnergyDepletedEventFired = false;
};
