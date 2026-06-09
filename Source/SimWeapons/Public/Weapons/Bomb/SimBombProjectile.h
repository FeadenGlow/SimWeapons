// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Core/SimProjectileBase.h"
#include "TimerManager.h"
#include "SimBombProjectile.generated.h"

class USphereComponent;
class UProjectileMovementComponent;
class USimExplosionComponent;
class UPrimitiveComponent;

UCLASS(Blueprintable, BlueprintType)
class SIMWEAPONS_API ASimBombProjectile : public ASimProjectileBase
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ASimBombProjectile();

	virtual void DetonateFromExplosion_Implementation(const FVector& TriggerLocation) override;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnBombHit(
		UPrimitiveComponent* HitComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		FVector NormalImpulse,
		const FHitResult& Hit
	);

	UFUNCTION()
	void OnBombStopped(const FHitResult& ImpactResult);

	void Explode(const FVector& ExplosionLocation);

	void ExplodeByFuse();

	UFUNCTION(BlueprintImplementableEvent, Category = "Sim Weapons|Bomb")
	void OnExplosionTriggered(const FVector& ExplosionLocation);

	UFUNCTION(BlueprintImplementableEvent, Category = "Sim Weapons|Bomb")
	void OnBombDropped();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sim Weapons|Bomb")
	USphereComponent* CollisionComponent = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sim Weapons|Bomb")
	UProjectileMovementComponent* ProjectileMovementComponent = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sim Weapons|Bomb")
	USimExplosionComponent* ExplosionComponent = nullptr;

	// If true, bomb explodes when it hits something.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Bomb")
	bool bExplodeOnHit = true;

	// If true, bomb explodes after FuseTime even if it has not hit anything.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Bomb|Fuse")
	bool bExplodeAfterFuse = true;

	// Time in seconds before timed explosion.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Bomb|Fuse", meta = (ClampMin = "0.0"))
	float FuseTime = 8.0f;

	// Gravity scale for bomb falling.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Bomb|Movement", meta = (ClampMin = "0.0"))
	float GravityScale = 1.0f;

	// Small forward speed when bomb is dropped.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Bomb|Movement", meta = (ClampMin = "0.0"))
	float ForwardDropSpeed = 0.0f;

	// Small downward speed when bomb is dropped.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Bomb|Movement", meta = (ClampMin = "0.0"))
	float DownwardDropSpeed = 100.0f;

	// Maximum speed while falling. 0 means no limit.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Bomb|Movement", meta = (ClampMin = "0.0"))
	float MaximumFallSpeed = 4000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Bomb")
	bool bIgnoreOwnerCollision = true;

private:
	bool bHasExploded = false;

	FTimerHandle FuseTimerHandle;
};