// Fill out your copyright notice in the Description page of Project Settings.

#include "Weapons/SimExplosionComponent.h"

#include "Carriers/SimWeaponCarrierComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Core/SimProjectileBase.h"
#include "DrawDebugHelpers.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "EngineUtils.h"

// Sets default values for this component's properties
USimExplosionComponent::USimExplosionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void USimExplosionComponent::BeginPlay()
{
	Super::BeginPlay();
}

void USimExplosionComponent::ExplodeAtLocation(
	const FVector& ExplosionLocation,
	AActor* IgnoredActor,
	AActor* ExplosionOwner
)
{
	AActor* ActorToIgnore = IgnoredActor;

	if (!ActorToIgnore)
	{
		ActorToIgnore = GetOwner();
	}

	if (bApplyDamage)
	{
		ApplyExplosionDamage(ExplosionLocation, ActorToIgnore, ExplosionOwner);
	}

	if (bApplyImpulse)
	{
		ApplyExplosionImpulse(ExplosionLocation, ActorToIgnore);
	}

	if (bTriggerChainReaction)
	{
		ApplyExplosionChainReaction(ExplosionLocation, ActorToIgnore);
	}

	if (bDrawDebugExplosionRadius)
	{
		DrawExplosionDebug(ExplosionLocation);
	}
}

void USimExplosionComponent::ApplyExplosionDamage(
	const FVector& ExplosionLocation,
	AActor* IgnoredActor,
	AActor* ExplosionOwner
)
{
	UWorld* World = GetWorld();

	if (!World)
	{
		return;
	}

	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldStatic);
	ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldDynamic);
	ObjectQueryParams.AddObjectTypesToQuery(ECC_Pawn);
	ObjectQueryParams.AddObjectTypesToQuery(ECC_PhysicsBody);
	ObjectQueryParams.AddObjectTypesToQuery(ECC_Vehicle);

	FCollisionQueryParams QueryParams;

	if (IgnoredActor)
	{
		QueryParams.AddIgnoredActor(IgnoredActor);
	}

	TArray<FOverlapResult> Overlaps;

	const FCollisionShape ExplosionShape = FCollisionShape::MakeSphere(ExplosionRadius);

	const bool bHasOverlaps = World->OverlapMultiByObjectType(
		Overlaps,
		ExplosionLocation,
		FQuat::Identity,
		ObjectQueryParams,
		ExplosionShape,
		QueryParams
	);

	if (!bHasOverlaps)
	{
		return;
	}

	TSet<AActor*> DamagedActors;

	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* HitActor = Overlap.GetActor();

		if (!HitActor || HitActor == IgnoredActor)
		{
			continue;
		}

		if (!bDamageOwner && ExplosionOwner && HitActor == ExplosionOwner)
		{
			continue;
		}

		if (DamagedActors.Contains(HitActor))
		{
			continue;
		}

		USimWeaponCarrierComponent* CarrierComponent =
			HitActor->FindComponentByClass<USimWeaponCarrierComponent>();

		if (!CarrierComponent)
		{
			continue;
		}

		const float Distance = FVector::Distance(ExplosionLocation, HitActor->GetActorLocation());
		const float FinalDamage = CalculateDamageByDistance(Distance);

		if (FinalDamage <= 0.0f)
		{
			continue;
		}

		CarrierComponent->ApplyCarrierDamage(FinalDamage);
		DamagedActors.Add(HitActor);

		UE_LOG(
			LogTemp,
			Log,
			TEXT("Explosion damaged carrier: %s | Distance: %.2f | Damage: %.2f | Health left: %.2f"),
			*HitActor->GetName(),
			Distance,
			FinalDamage,
			CarrierComponent->GetCurrentHealth()
		);
	}
}

void USimExplosionComponent::ApplyExplosionImpulse(
	const FVector& ExplosionLocation,
	AActor* IgnoredActor
)
{
	UWorld* World = GetWorld();

	if (!World)
	{
		return;
	}

	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldStatic);
	ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldDynamic);
	ObjectQueryParams.AddObjectTypesToQuery(ECC_Pawn);
	ObjectQueryParams.AddObjectTypesToQuery(ECC_PhysicsBody);
	ObjectQueryParams.AddObjectTypesToQuery(ECC_Vehicle);

	FCollisionQueryParams QueryParams;

	if (IgnoredActor)
	{
		QueryParams.AddIgnoredActor(IgnoredActor);
	}

	TArray<FOverlapResult> Overlaps;

	const FCollisionShape ExplosionShape = FCollisionShape::MakeSphere(ExplosionRadius);

	const bool bHasOverlaps = World->OverlapMultiByObjectType(
		Overlaps,
		ExplosionLocation,
		FQuat::Identity,
		ObjectQueryParams,
		ExplosionShape,
		QueryParams
	);

	if (!bHasOverlaps)
	{
		return;
	}

	TSet<UPrimitiveComponent*> PushedComponents;

	for (const FOverlapResult& Overlap : Overlaps)
	{
		UPrimitiveComponent* PrimitiveComponent = Overlap.GetComponent();

		if (!PrimitiveComponent || PushedComponents.Contains(PrimitiveComponent))
		{
			continue;
		}

		if (!PrimitiveComponent->IsSimulatingPhysics())
		{
			continue;
		}

		PrimitiveComponent->AddRadialImpulse(
			ExplosionLocation,
			ExplosionRadius,
			ExplosionImpulseStrength,
			ERadialImpulseFalloff::RIF_Linear,
			false
		);

		PushedComponents.Add(PrimitiveComponent);
	}
}

void USimExplosionComponent::ApplyExplosionChainReaction(
	const FVector& ExplosionLocation,
	AActor* IgnoredActor
)
{
	UWorld* World = GetWorld();

	if (!World)
	{
		return;
	}

	TArray<ASimProjectileBase*> ProjectilesToDetonate;

	for (TActorIterator<ASimProjectileBase> ProjectileIterator(World); ProjectileIterator; ++ProjectileIterator)
	{
		ASimProjectileBase* Projectile = *ProjectileIterator;

		if (!Projectile || Projectile == IgnoredActor)
		{
			continue;
		}

		if (!Projectile->CanDetonateFromExplosion())
		{
			continue;
		}

		const float Distance = FVector::Distance(
			ExplosionLocation,
			Projectile->GetActorLocation()
		);

		if (Distance > ExplosionRadius)
		{
			continue;
		}

		ProjectilesToDetonate.Add(Projectile);
	}

	for (ASimProjectileBase* Projectile : ProjectilesToDetonate)
	{
		if (!IsValid(Projectile))
		{
			continue;
		}

		UE_LOG(
			LogTemp,
			Log,
			TEXT("Explosion triggered projectile detonation: %s"),
			*Projectile->GetName()
		);

		Projectile->DetonateFromExplosion(ExplosionLocation);
	}
}

float USimExplosionComponent::CalculateDamageByDistance(float Distance) const
{
	const float SafeExplosionRadius = FMath::Max(ExplosionRadius, InnerDamageRadius + 1.0f);

	if (Distance <= InnerDamageRadius)
	{
		return Damage;
	}

	if (Distance >= SafeExplosionRadius)
	{
		return MinimumExplosionDamage;
	}

	const float FalloffAlpha = (Distance - InnerDamageRadius) / (SafeExplosionRadius - InnerDamageRadius);

	return FMath::Lerp(Damage, MinimumExplosionDamage, FalloffAlpha);
}

void USimExplosionComponent::DrawExplosionDebug(const FVector& ExplosionLocation) const
{
	UWorld* World = GetWorld();

	if (!World)
	{
		return;
	}

	DrawDebugSphere(
		World,
		ExplosionLocation,
		InnerDamageRadius,
		32,
		FColor::Red,
		false,
		3.0f
	);

	DrawDebugSphere(
		World,
		ExplosionLocation,
		ExplosionRadius,
		32,
		FColor::Orange,
		false,
		3.0f
	);
}