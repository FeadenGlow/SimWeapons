// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Core/SimWeaponBase.h"
#include "SimDefensiveLauncherWeapon.generated.h"

class ASimDefensiveProjectile;

UCLASS(Blueprintable, BlueprintType)
class SIMWEAPONS_API ASimDefensiveLauncherWeapon : public ASimWeaponBase
{
	GENERATED_BODY()

public:
	ASimDefensiveLauncherWeapon();

	virtual void Fire_Implementation() override;

protected:
	UFUNCTION(BlueprintImplementableEvent, Category = "Sim Weapons|Defensive")
	void OnDefensiveLauncherFired(int32 SpawnedProjectiles);

	UFUNCTION(BlueprintImplementableEvent, Category = "Sim Weapons|Defensive")
	void OnDefensiveProjectileSpawned(ASimDefensiveProjectile* SpawnedProjectile);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Defensive")
	TSubclassOf<ASimDefensiveProjectile> DefensiveProjectileClass;

	// Number of defensive projectiles launched per fire.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Defensive", meta = (ClampMin = "1"))
	int32 ProjectileCount = 4;

	// Base pitch angle in degrees. 90 means straight up.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Defensive", meta = (ClampMin = "0.0", ClampMax = "90.0"))
	float LaunchPitchDegrees = 75.0f;

	// Random cone spread around the base launch direction.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Defensive", meta = (ClampMin = "0.0", ClampMax = "90.0"))
	float SpreadAngleDegrees = 25.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Defensive", meta = (ClampMin = "0.0"))
	float SpawnForwardOffset = 40.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Defensive", meta = (ClampMin = "0.0"))
	float SpawnUpOffset = 80.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Defensive")
	bool bRandomizeProjectileRoll = true;
};