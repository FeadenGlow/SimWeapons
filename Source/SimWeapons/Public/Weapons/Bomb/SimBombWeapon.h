// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Core/SimWeaponBase.h"
#include "SimBombWeapon.generated.h"

class ASimBombProjectile;

UCLASS(Blueprintable, BlueprintType)
class SIMWEAPONS_API ASimBombWeapon : public ASimWeaponBase
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ASimBombWeapon();

	virtual void Fire_Implementation() override;

protected:
	// Called in Blueprint after bomb projectile was successfully spawned.
	UFUNCTION(BlueprintImplementableEvent, Category = "Sim Weapons|Bomb")
	void OnBombDropped(ASimBombProjectile* SpawnedProjectile);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Bomb")
	TSubclassOf<ASimBombProjectile> BombProjectileClass;

	// Forward offset from weapon position.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Bomb", meta = (ClampMin = "0.0"))
	float SpawnForwardOffset = 0.0f;

	// Downward offset from weapon position.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Bomb", meta = (ClampMin = "0.0"))
	float SpawnDownOffset = 80.0f;
};