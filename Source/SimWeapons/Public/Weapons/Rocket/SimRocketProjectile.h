// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Core/SimProjectileBase.h"
#include "TimerManager.h"
#include "SimRocketProjectile.generated.h"

class USphereComponent;
class UProjectileMovementComponent;
class USimExplosionComponent;
class UPrimitiveComponent;

UCLASS(Blueprintable, BlueprintType)
class SIMWEAPONS_API ASimRocketProjectile : public ASimProjectileBase
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ASimRocketProjectile();

	virtual void DetonateFromExplosion_Implementation(const FVector& TriggerLocation) override;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnRocketHit(
		UPrimitiveComponent* HitComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		FVector NormalImpulse,
		const FHitResult& Hit
	);

	UFUNCTION()
	void OnRocketStopped(const FHitResult& ImpactResult);

	void Explode(const FVector& ExplosionLocation);

	void EnableGravityAfterStraightFlight();

	UFUNCTION(BlueprintImplementableEvent, Category = "Sim Weapons|Rocket")
	void OnExplosionTriggered(const FVector& ExplosionLocation);

	UFUNCTION(BlueprintImplementableEvent, Category = "Sim Weapons|Rocket")
	void OnRocketLaunched();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sim Weapons|Rocket")
	USphereComponent* CollisionComponent = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sim Weapons|Rocket")
	UProjectileMovementComponent* ProjectileMovementComponent = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sim Weapons|Rocket")
	USimExplosionComponent* ExplosionComponent = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Rocket|Flight")
	bool bUseStraightFlightBeforeGravity = true;

	// Time in seconds while rocket flies straight without gravity.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Rocket|Flight", meta = (ClampMin = "0.0"))
	float StraightFlightTime = 1.2f;

	// Gravity scale after straight flight phase.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Rocket|Flight", meta = (ClampMin = "0.0"))
	float GravityScaleAfterStraightFlight = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Rocket")
	bool bIgnoreOwnerCollision = true;

private:
	bool bHasExploded = false;

	FTimerHandle StraightFlightTimerHandle;
};