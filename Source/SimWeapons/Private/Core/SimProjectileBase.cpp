// Fill out your copyright notice in the Description page of Project Settings.

#include "Core/SimProjectileBase.h"

// Sets default values
ASimProjectileBase::ASimProjectileBase()
{
	// Base projectile does not need Tick yet.
	PrimaryActorTick.bCanEverTick = false;
}

// Called when the game starts or when spawned
void ASimProjectileBase::BeginPlay()
{
	Super::BeginPlay();

	if (LifeTime > 0.0f)
	{
		SetLifeSpan(LifeTime);
	}
}

float ASimProjectileBase::GetDamage() const
{
	return Damage;
}

float ASimProjectileBase::GetSpeed() const
{
	return Speed;
}

float ASimProjectileBase::GetLifeTime() const
{
	return LifeTime;
}