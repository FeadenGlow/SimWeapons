// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SimExplosionComponent.generated.h"

UCLASS(Blueprintable, BlueprintType, ClassGroup = (SimWeapons), meta = (BlueprintSpawnableComponent))
class SIMWEAPONS_API USimExplosionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	USimExplosionComponent();

	// Runs the full explosion logic at the given world location.
	// IgnoredActor is usually the projectile that is exploding.
	// ExplosionOwner is usually the actor that fired/dropped the projectile.
	UFUNCTION(BlueprintCallable, Category = "Sim Weapons|Explosion")
	void ExplodeAtLocation(
		const FVector& ExplosionLocation,
		AActor* IgnoredActor = nullptr,
		AActor* ExplosionOwner = nullptr
	);

	UFUNCTION(BlueprintCallable, Category = "Sim Weapons|Explosion")
	float CalculateDamageByDistance(float Distance) const;

protected:
	virtual void BeginPlay() override;

	void ApplyExplosionDamage(
		const FVector& ExplosionLocation,
		AActor* IgnoredActor,
		AActor* ExplosionOwner
	);

	void ApplyExplosionImpulse(
		const FVector& ExplosionLocation,
		AActor* IgnoredActor
	);

	void ApplyExplosionChainReaction(
		const FVector& ExplosionLocation,
		AActor* IgnoredActor
	);

	void DrawExplosionDebug(const FVector& ExplosionLocation) const;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Explosion|Damage")
	bool bApplyDamage = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Explosion|Impulse")
	bool bApplyImpulse = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Explosion|Chain Reaction")
	bool bTriggerChainReaction = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Explosion|Damage")
	bool bDamageOwner = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Explosion|Debug")
	bool bDrawDebugExplosionRadius = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Explosion|Damage", meta = (ClampMin = "0.0"))
	float Damage = 120.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Explosion|Damage", meta = (ClampMin = "0.0"))
	float InnerDamageRadius = 300.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Explosion|Damage", meta = (ClampMin = "1.0"))
	float ExplosionRadius = 1200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Explosion|Damage", meta = (ClampMin = "0.0"))
	float MinimumExplosionDamage = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sim Weapons|Explosion|Impulse", meta = (ClampMin = "0.0"))
	float ExplosionImpulseStrength = 120000.0f;
};