// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Core/SimWeaponBase.h"
#include "SimRocketWeapon.generated.h"

class ASimRocketProjectile;

UCLASS(Blueprintable, BlueprintType)
class SIMWEAPONS_API ASimRocketWeapon : public ASimWeaponBase
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ASimRocketWeapon();

	virtual void Fire_Implementation() override;

protected:
	// Called in Blueprint after rocket projectile was successfully spawned.
	UFUNCTION(BlueprintImplementableEvent, Category = "Sim Weapons|Rocket")
	void OnRocketFired(ASimRocketProjectile* SpawnedProjectile);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Rocket")
	TSubclassOf<ASimRocketProjectile> RocketProjectileClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Rocket")
	float SpawnForwardOffset = 100.0f;
};