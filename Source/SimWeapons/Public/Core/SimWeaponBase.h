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
	float GetBatteryCost() const;

	UFUNCTION(BlueprintCallable, Category = "Sim Weapons")
	int32 GetCurrentAmmo() const;

	UFUNCTION(BlueprintCallable, Category = "Sim Weapons")
	int32 GetMaxAmmo() const;

	UFUNCTION(BlueprintCallable, Category = "Sim Weapons")
	int32 GetWeaponSlotCost() const;

protected:
	virtual void BeginPlay() override;

	bool TryConsumeAmmo();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons")
	float BatteryCost = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons")
	int32 MaxAmmo = 1;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sim Weapons")
	int32 CurrentAmmo = 1;

	// How many weapon slots this weapon uses on the carrier.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons", meta = (ClampMin = "1"))
	int32 WeaponSlotCost = 1;
};