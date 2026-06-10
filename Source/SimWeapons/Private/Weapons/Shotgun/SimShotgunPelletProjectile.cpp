// Fill out your copyright notice in the Description page of Project Settings.

#include "Weapons/Shotgun/SimShotgunPelletProjectile.h"

#include "Carriers/SimWeaponCarrierComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"

ASimShotgunPelletProjectile::ASimShotgunPelletProjectile()
{
	PrimaryActorTick.bCanEverTick = false;

	CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionComponent"));
	SetRootComponent(CollisionComponent);

	CollisionComponent->InitSphereRadius(5.0f);
	CollisionComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	CollisionComponent->SetCollisionObjectType(ECC_WorldDynamic);
	CollisionComponent->SetCollisionResponseToAllChannels(ECR_Block);
	CollisionComponent->SetNotifyRigidBodyCollision(true);

	ProjectileMovementComponent = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovementComponent"));
	ProjectileMovementComponent->UpdatedComponent = CollisionComponent;
	ProjectileMovementComponent->bRotationFollowsVelocity = true;
	ProjectileMovementComponent->bShouldBounce = false;
	ProjectileMovementComponent->bSweepCollision = true;
	ProjectileMovementComponent->ProjectileGravityScale = 0.0f;

	Damage = 15.0f;
	Speed = 6000.0f;
	LifeTime = 0.7f;

	MaximumSpeed = 8000.0f;
	GravityScale = 0.0f;

	bDestroyOnHit = true;
	bHideImmediatelyOnHit = true;
	DestroyDelayAfterHit = 0.02f;

	// Pellet itself should not be detonated by explosion chain reaction.
	// But it can detonate rockets, bombs, mines, or other explosive projectiles by direct hit.
	bCanDetonateFromExplosion = false;
}

void ASimShotgunPelletProjectile::BeginPlay()
{
	Super::BeginPlay();

	// Force these values here because Blueprint instances can keep old default values.
	bCanDetonateFromExplosion = false;
	bDestroyOnHit = true;

	if (CollisionComponent)
	{
		CollisionComponent->OnComponentHit.AddDynamic(this, &ASimShotgunPelletProjectile::OnPelletHit);

		if (bIgnoreOwnerCollision)
		{
			if (AActor* OwnerActor = GetOwner())
			{
				CollisionComponent->IgnoreActorWhenMoving(OwnerActor, true);
			}

			if (APawn* InstigatorPawn = GetInstigator())
			{
				CollisionComponent->IgnoreActorWhenMoving(InstigatorPawn, true);
			}
		}
	}

	if (ProjectileMovementComponent)
	{
		ProjectileMovementComponent->InitialSpeed = Speed;
		ProjectileMovementComponent->MaxSpeed = MaximumSpeed;
		ProjectileMovementComponent->ProjectileGravityScale = GravityScale;
		ProjectileMovementComponent->Velocity = GetActorForwardVector() * Speed;

		ProjectileMovementComponent->OnProjectileStop.AddDynamic(this, &ASimShotgunPelletProjectile::OnPelletStopped);
	}
}

void ASimShotgunPelletProjectile::InitializePellet(
	const FVector& LaunchDirection,
	const TArray<AActor*>& ActorsToIgnore
)
{
	const FVector SafeLaunchDirection = LaunchDirection.IsNearlyZero()
		? GetActorForwardVector()
		: LaunchDirection.GetSafeNormal();

	SetActorRotation(SafeLaunchDirection.Rotation());

	for (AActor* ActorToIgnore : ActorsToIgnore)
	{
		AddIgnoredActor(ActorToIgnore);
	}

	if (ProjectileMovementComponent)
	{
		ProjectileMovementComponent->Velocity = SafeLaunchDirection * Speed;
		ProjectileMovementComponent->UpdateComponentVelocity();
	}
}

void ASimShotgunPelletProjectile::AddIgnoredActor(AActor* ActorToIgnore)
{
	if (!ActorToIgnore || ActorToIgnore == this)
	{
		return;
	}

	if (CollisionComponent)
	{
		CollisionComponent->IgnoreActorWhenMoving(ActorToIgnore, true);
	}
}

void ASimShotgunPelletProjectile::OnPelletHit(
	UPrimitiveComponent* HitComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	FVector NormalImpulse,
	const FHitResult& Hit
)
{
	if (bHasHit)
	{
		return;
	}

	HandlePelletImpact(OtherActor);
}

void ASimShotgunPelletProjectile::OnPelletStopped(const FHitResult& ImpactResult)
{
	if (bHasHit)
	{
		return;
	}

	HandlePelletImpact(ImpactResult.GetActor());
}

void ASimShotgunPelletProjectile::HandlePelletImpact(AActor* HitActor)
{
	if (bHasHit)
	{
		return;
	}

	bHasHit = true;

	if (HitActor && HitActor != this && HitActor != GetOwner())
	{
		ApplyPelletDamage(HitActor);
		TryDetonateExplosiveTarget(HitActor);
	}

	if (bDestroyOnHit)
	{
		FinishPelletAfterImpact();
	}
}

void ASimShotgunPelletProjectile::ApplyPelletDamage(AActor* HitActor)
{
	if (!bDamageCarriers || !HitActor)
	{
		return;
	}

	USimWeaponCarrierComponent* CarrierComponent =
		HitActor->FindComponentByClass<USimWeaponCarrierComponent>();

	if (!CarrierComponent)
	{
		return;
	}

	CarrierComponent->ApplyCarrierDamage(Damage);

	UE_LOG(
		LogTemp,
		Log,
		TEXT("Shotgun pellet damaged carrier: %s | Damage: %.2f | Health left: %.2f"),
		*HitActor->GetName(),
		Damage,
		CarrierComponent->GetCurrentHealth()
	);
}

void ASimShotgunPelletProjectile::TryDetonateExplosiveTarget(AActor* HitActor)
{
	if (!bCanDetonateExplosivesOnHit || !HitActor)
	{
		return;
	}

	ASimProjectileBase* Projectile = Cast<ASimProjectileBase>(HitActor);

	if (!Projectile)
	{
		return;
	}

	if (!Projectile->CanDetonateFromExplosion())
	{
		return;
	}

	UE_LOG(
		LogTemp,
		Log,
		TEXT("Shotgun pellet triggered explosive projectile detonation: %s"),
		*Projectile->GetName()
	);

	Projectile->DetonateFromExplosion(GetActorLocation());
}

void ASimShotgunPelletProjectile::FinishPelletAfterImpact()
{
	if (ProjectileMovementComponent)
	{
		ProjectileMovementComponent->StopMovementImmediately();
		ProjectileMovementComponent->Deactivate();
	}

	if (CollisionComponent)
	{
		CollisionComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	SetActorEnableCollision(false);

	if (bHideImmediatelyOnHit)
	{
		SetActorHiddenInGame(true);
	}

	if (DestroyDelayAfterHit <= 0.0f)
	{
		Destroy();
	}
	else
	{
		SetLifeSpan(DestroyDelayAfterHit);
	}
}