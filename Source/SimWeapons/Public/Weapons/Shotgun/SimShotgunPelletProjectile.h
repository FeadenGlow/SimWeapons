// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Core/SimProjectileBase.h"
#include "SimShotgunPelletProjectile.generated.h"

class USphereComponent;
class UProjectileMovementComponent;
class UPrimitiveComponent;

UCLASS(Blueprintable, BlueprintType)
class SIMWEAPONS_API ASimShotgunPelletProjectile : public ASimProjectileBase
{
	GENERATED_BODY()

public:
	ASimShotgunPelletProjectile();

	void InitializePellet(const FVector& LaunchDirection, const TArray<AActor*>& ActorsToIgnore);

	void AddIgnoredActor(AActor* ActorToIgnore);

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnPelletHit(
		UPrimitiveComponent* HitComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		FVector NormalImpulse,
		const FHitResult& Hit
	);

	UFUNCTION()
	void OnPelletStopped(const FHitResult& ImpactResult);

	void HandlePelletImpact(AActor* HitActor);

	void ApplyPelletDamage(AActor* HitActor);

	void TryDetonateExplosiveTarget(AActor* HitActor);

	void FinishPelletAfterImpact();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sim Weapons|Shotgun")
	USphereComponent* CollisionComponent = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sim Weapons|Shotgun")
	UProjectileMovementComponent* ProjectileMovementComponent = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Shotgun")
	bool bIgnoreOwnerCollision = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Shotgun|Damage")
	bool bDamageCarriers = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Shotgun|Damage")
	bool bCanDetonateExplosivesOnHit = true;

	// Pellet should disappear after the first valid impact.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Shotgun")
	bool bDestroyOnHit = true;

	// Hide pellet immediately after impact, before Destroy fully removes the actor.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Shotgun")
	bool bHideImmediatelyOnHit = true;

	// Small delay before actual destroy. 0 means destroy immediately.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Shotgun", meta = (ClampMin = "0.0"))
	float DestroyDelayAfterHit = 0.02f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Shotgun|Movement", meta = (ClampMin = "0.0"))
	float MaximumSpeed = 8000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Shotgun|Movement", meta = (ClampMin = "0.0"))
	float GravityScale = 0.0f;

private:
	bool bHasHit = false;
};