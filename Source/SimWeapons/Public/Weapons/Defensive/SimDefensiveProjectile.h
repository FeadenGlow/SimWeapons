// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Core/SimProjectileBase.h"
#include "TimerManager.h"
#include "SimDefensiveProjectile.generated.h"

class USphereComponent;
class UProjectileMovementComponent;
class USimExplosionComponent;
class UPrimitiveComponent;

UCLASS(Blueprintable, BlueprintType)
class SIMWEAPONS_API ASimDefensiveProjectile : public ASimProjectileBase
{
	GENERATED_BODY()

public:
	ASimDefensiveProjectile();

	virtual void DetonateFromExplosion_Implementation(const FVector& TriggerLocation) override;

	void InitializeDefensiveProjectile(const FVector& LaunchDirection, const TArray<AActor*>& ActorsToIgnore);

	void AddIgnoredActor(AActor* ActorToIgnore);

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnDefensiveProjectileHit(
		UPrimitiveComponent* HitComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		FVector NormalImpulse,
		const FHitResult& Hit
	);

	UFUNCTION()
	void OnDefensiveProjectileStopped(const FHitResult& ImpactResult);

	void Explode(const FVector& ExplosionLocation);

	void ExplodeByAirBurst();

	UFUNCTION(BlueprintImplementableEvent, Category = "Sim Weapons|Defensive")
	void OnDefensiveProjectileLaunched();

	UFUNCTION(BlueprintImplementableEvent, Category = "Sim Weapons|Defensive")
	void OnDefensiveProjectileExploded(const FVector& ExplosionLocation);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sim Weapons|Defensive")
	USphereComponent* CollisionComponent = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sim Weapons|Defensive")
	UProjectileMovementComponent* ProjectileMovementComponent = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sim Weapons|Defensive")
	USimExplosionComponent* ExplosionComponent = nullptr;

	// If true, projectile explodes when it hits something before air burst.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Defensive")
	bool bExplodeOnHit = true;

	// If true, projectile explodes in the air after AirBurstTime.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Defensive|Air Burst")
	bool bUseAirBurst = true;

	// Time in seconds before the defensive projectile explodes in the air.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Defensive|Air Burst", meta = (ClampMin = "0.0"))
	float AirBurstTime = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Defensive")
	bool bIgnoreOwnerCollision = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Defensive|Movement", meta = (ClampMin = "0.0"))
	float MaximumSpeed = 3000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Defensive|Movement", meta = (ClampMin = "0.0"))
	float GravityScale = 0.4f;

private:
	bool bHasExploded = false;

	FTimerHandle AirBurstTimerHandle;
};