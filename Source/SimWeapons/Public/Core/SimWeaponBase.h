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
	// Sets default values for this actor's properties
	ASimWeaponBase();

	// Main weapon action. Rocket and Bomb weapons will override this method.
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

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// Tries to spend one ammo unit.
	// Child classes should call this before actually firing.
	bool TryConsumeAmmo();

	// Battery charge cost for one weapon launch/fire action.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons")
	float BatteryCost = 5.0f;

	// Maximum ammo count for this weapon.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons")
	int32 MaxAmmo = 1;

	// Current ammo count.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sim Weapons")
	int32 CurrentAmmo = 1;
};