// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Core/SimWeaponBase.h"
#include "SimShotgunWeapon.generated.h"

class ASimShotgunPelletProjectile;

UCLASS(Blueprintable, BlueprintType)
class SIMWEAPONS_API ASimShotgunWeapon : public ASimWeaponBase
{
	GENERATED_BODY()

public:
	ASimShotgunWeapon();

	virtual void Fire_Implementation() override;

protected:
	UFUNCTION(BlueprintImplementableEvent, Category = "Sim Weapons|Shotgun")
	void OnShotgunFired(int32 SpawnedPellets);

	UFUNCTION(BlueprintImplementableEvent, Category = "Sim Weapons|Shotgun")
	void OnShotgunPelletSpawned(ASimShotgunPelletProjectile* SpawnedPellet);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Shotgun")
	TSubclassOf<ASimShotgunPelletProjectile> PelletProjectileClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Shotgun", meta = (ClampMin = "1"))
	int32 PelletCount = 12;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Shotgun", meta = (ClampMin = "0.0", ClampMax = "90.0"))
	float SpreadAngleDegrees = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Shotgun", meta = (ClampMin = "0.0"))
	float SpawnForwardOffset = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Shotgun")
	bool bRandomizePelletRoll = true;
};