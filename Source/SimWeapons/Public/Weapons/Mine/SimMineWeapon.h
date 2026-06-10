// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Core/SimWeaponBase.h"
#include "SimMineWeapon.generated.h"

class ASimMineActor;

UCLASS(Blueprintable, BlueprintType)
class SIMWEAPONS_API ASimMineWeapon : public ASimWeaponBase
{
	GENERATED_BODY()

public:
	ASimMineWeapon();

	virtual void Fire_Implementation() override;

protected:
	UFUNCTION(BlueprintImplementableEvent, Category = "Sim Weapons|Mine")
	void OnMinePlaced(ASimMineActor* SpawnedMine);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Mine")
	TSubclassOf<ASimMineActor> MineClass;

	// Forward offset from weapon position.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Mine", meta = (ClampMin = "0.0"))
	float SpawnForwardOffset = 50.0f;

	// Downward offset from weapon position.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Mine", meta = (ClampMin = "0.0"))
	float SpawnDownOffset = 80.0f;
};