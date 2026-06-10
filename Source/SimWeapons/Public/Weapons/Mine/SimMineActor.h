// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Core/SimProjectileBase.h"
#include "TimerManager.h"
#include "SimMineActor.generated.h"

class USphereComponent;
class UProjectileMovementComponent;
class USimExplosionComponent;
class UPrimitiveComponent;

UCLASS(Blueprintable, BlueprintType)
class SIMWEAPONS_API ASimMineActor : public ASimProjectileBase
{
	GENERATED_BODY()

public:
	ASimMineActor();

	virtual void DetonateFromExplosion_Implementation(const FVector& TriggerLocation) override;

	UFUNCTION(BlueprintCallable, Category = "Sim Weapons|Mine")
	bool IsArmed() const;

	UFUNCTION(BlueprintCallable, Category = "Sim Weapons|Mine")
	bool IsDeployed() const;

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnMineHit(
		UPrimitiveComponent* HitComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		FVector NormalImpulse,
		const FHitResult& Hit
	);

	UFUNCTION()
	void OnMineTriggerBeginOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult
	);

	void DeployMine(const FHitResult& Hit);

	void ArmMine();

	void CheckExistingTriggerOverlaps();

	bool ShouldExplodeFromHit(AActor* OtherActor) const;

	bool IsValidTriggerActor(AActor* OtherActor, UPrimitiveComponent* OtherComp) const;

	void Explode(const FVector& ExplosionLocation);

	void ExplodeByFuse();

	UFUNCTION(BlueprintImplementableEvent, Category = "Sim Weapons|Mine")
	void OnMineDeployed();

	UFUNCTION(BlueprintImplementableEvent, Category = "Sim Weapons|Mine")
	void OnMineArmed();

	UFUNCTION(BlueprintImplementableEvent, Category = "Sim Weapons|Mine")
	void OnMineExploded(const FVector& ExplosionLocation);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sim Weapons|Mine")
	USphereComponent* CollisionComponent = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sim Weapons|Mine")
	USphereComponent* TriggerSphereComponent = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sim Weapons|Mine")
	UProjectileMovementComponent* ProjectileMovementComponent = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sim Weapons|Mine")
	USimExplosionComponent* ExplosionComponent = nullptr;

	// If true, mine stops moving after first surface hit.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Mine")
	bool bDeployOnFirstHit = true;

	// If true, mine arms itself after ArmDelay.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Mine")
	bool bArmAfterDelay = true;

	// Delay before mine becomes active.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Mine", meta = (ClampMin = "0.0"))
	float ArmDelay = 1.5f;

	// Trigger radius for armed mine.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Mine", meta = (ClampMin = "1.0"))
	float TriggerRadius = 400.0f;

	// If true, mine can explode when armed and physically hit by a valid target.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Mine")
	bool bExplodeOnHitWhenArmed = true;

	// If true, direct projectile hit detonates the mine even before it is armed.
	// This allows shotgun pellets to neutralize mines.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Mine")
	bool bExplodeWhenHitByProjectile = true;

	// If true, mine can be triggered by actors with SimWeaponCarrierComponent.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Mine|Trigger")
	bool bTriggerOnCarriers = true;

	// If true, mine can be triggered by explosive projectiles, such as rockets and bombs.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Mine|Trigger")
	bool bTriggerOnExplosiveProjectiles = true;

	// If true, mine can be triggered by physics objects.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Mine|Trigger")
	bool bTriggerOnPhysicsObjects = false;

	// If false, mine ignores the actor that placed it.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Mine|Trigger")
	bool bCanTriggerOwner = false;

	// If true, mine explodes after FuseTime even if nothing triggered it.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Mine|Fuse")
	bool bExplodeAfterFuse = false;

	// Time before timed explosion.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Mine|Fuse", meta = (ClampMin = "0.0"))
	float FuseTime = 30.0f;

	// Gravity scale while mine is being dropped.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Mine|Movement", meta = (ClampMin = "0.0"))
	float GravityScale = 1.0f;

	// Small forward speed when mine is placed/dropped.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Mine|Movement", meta = (ClampMin = "0.0"))
	float ForwardDropSpeed = 0.0f;

	// Small downward speed when mine is placed/dropped.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Mine|Movement", meta = (ClampMin = "0.0"))
	float DownwardDropSpeed = 100.0f;

	// Maximum speed while mine is falling. 0 means no limit.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Mine|Movement", meta = (ClampMin = "0.0"))
	float MaximumFallSpeed = 2500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Mine")
	bool bIgnoreOwnerCollision = true;

private:
	bool bIsDeployed = false;

	bool bIsArmed = false;

	bool bHasExploded = false;

	FTimerHandle ArmTimerHandle;

	FTimerHandle FuseTimerHandle;
};