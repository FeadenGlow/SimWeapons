// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SimProjectileBase.generated.h"

UCLASS(Blueprintable, BlueprintType)
class SIMWEAPONS_API ASimProjectileBase : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ASimProjectileBase();

	UFUNCTION(BlueprintCallable, Category = "Sim Weapons|Projectile")
	float GetDamage() const;

	UFUNCTION(BlueprintCallable, Category = "Sim Weapons|Projectile")
	float GetSpeed() const;

	UFUNCTION(BlueprintCallable, Category = "Sim Weapons|Projectile")
	float GetLifeTime() const;

	UFUNCTION(BlueprintCallable, Category = "Sim Weapons|Projectile")
	bool CanDetonateFromExplosion() const;

	// Called when another explosion tries to detonate this projectile.
	// Rocket and Bomb projectiles can override this. Bullet projectiles should not.
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Sim Weapons|Projectile")
	void DetonateFromExplosion(const FVector& TriggerLocation);

	virtual void DetonateFromExplosion_Implementation(const FVector& TriggerLocation);

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// Damage dealt by this projectile.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Projectile")
	float Damage = 10.0f;

	// Projectile movement speed.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Projectile")
	float Speed = 1000.0f;

	// How long projectile exists before being destroyed.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Projectile")
	float LifeTime = 5.0f;

	// If true, this projectile can be detonated by another explosion.
	// Rockets and bombs should use true. Bullets should use false.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Projectile")
	bool bCanDetonateFromExplosion = false;
};