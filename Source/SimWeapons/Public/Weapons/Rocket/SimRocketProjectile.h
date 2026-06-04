// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Core/SimProjectileBase.h"
#include "SimRocketProjectile.generated.h"

class USphereComponent;
class UProjectileMovementComponent;

UCLASS(Blueprintable, BlueprintType)
class SIMWEAPONS_API ASimRocketProjectile : public ASimProjectileBase
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ASimRocketProjectile();

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

	void HandleRocketImpact(AActor* OtherActor);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sim Weapons|Rocket")
	USphereComponent* CollisionComponent = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sim Weapons|Rocket")
	UProjectileMovementComponent* ProjectileMovementComponent = nullptr;
};