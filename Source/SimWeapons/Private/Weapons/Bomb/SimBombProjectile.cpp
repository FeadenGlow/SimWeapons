// Fill out your copyright notice in the Description page of Project Settings.

#include "Weapons/Bomb/SimBombProjectile.h"

#include "Components/PrimitiveComponent.h"
#include "Components/SphereComponent.h"
#include "Engine/World.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Weapons/SimExplosionComponent.h"

// Sets default values
ASimBombProjectile::ASimBombProjectile()
{
	PrimaryActorTick.bCanEverTick = false;

	CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionComponent"));
	SetRootComponent(CollisionComponent);

	CollisionComponent->InitSphereRadius(25.0f);
	CollisionComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	CollisionComponent->SetCollisionObjectType(ECC_WorldDynamic);
	CollisionComponent->SetCollisionResponseToAllChannels(ECR_Block);
	CollisionComponent->SetNotifyRigidBodyCollision(true);

	ProjectileMovementComponent = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovementComponent"));
	ProjectileMovementComponent->UpdatedComponent = CollisionComponent;
	ProjectileMovementComponent->bRotationFollowsVelocity = true;
	ProjectileMovementComponent->bShouldBounce = false;
	ProjectileMovementComponent->bSweepCollision = true;
	ProjectileMovementComponent->ProjectileGravityScale = GravityScale;

	ExplosionComponent = CreateDefaultSubobject<USimExplosionComponent>(TEXT("ExplosionComponent"));

	// Bomb is explosive, so it can be detonated by another explosion.
	bCanDetonateFromExplosion = true;

	// Bomb defaults.
	Damage = 150.0f;
	Speed = 0.0f;
	LifeTime = 20.0f;

	bExplodeOnHit = true;
	bExplodeAfterFuse = true;
	FuseTime = 8.0f;

	GravityScale = 1.0f;
	ForwardDropSpeed = 0.0f;
	DownwardDropSpeed = 100.0f;
	MaximumFallSpeed = 4000.0f;
}

void ASimBombProjectile::BeginPlay()
{
	Super::BeginPlay();

	// Force this value here because Blueprint instances can keep old default values.
	bCanDetonateFromExplosion = true;

	if (CollisionComponent)
	{
		CollisionComponent->OnComponentHit.AddDynamic(this, &ASimBombProjectile::OnBombHit);

		if (bIgnoreOwnerCollision)
		{
			if (AActor* OwnerActor = GetOwner())
			{
				CollisionComponent->IgnoreActorWhenMoving(OwnerActor, true);
			}
		}
	}

	if (ProjectileMovementComponent)
	{
		ProjectileMovementComponent->InitialSpeed = 0.0f;
		ProjectileMovementComponent->MaxSpeed = MaximumFallSpeed;
		ProjectileMovementComponent->ProjectileGravityScale = GravityScale;

		const FVector InitialVelocity =
			GetActorForwardVector() * ForwardDropSpeed -
			GetActorUpVector() * DownwardDropSpeed;

		ProjectileMovementComponent->Velocity = InitialVelocity;

		ProjectileMovementComponent->OnProjectileStop.AddDynamic(this, &ASimBombProjectile::OnBombStopped);
	}

	if (bExplodeAfterFuse && FuseTime > 0.0f)
	{
		GetWorldTimerManager().SetTimer(
			FuseTimerHandle,
			this,
			&ASimBombProjectile::ExplodeByFuse,
			FuseTime,
			false
		);
	}

	OnBombDropped();
}

void ASimBombProjectile::DetonateFromExplosion_Implementation(const FVector& TriggerLocation)
{
	if (bHasExploded)
	{
		return;
	}

	// Another explosion triggers this bomb, but this bomb explodes at its own location.
	Explode(GetActorLocation());
}

void ASimBombProjectile::OnBombHit(
	UPrimitiveComponent* HitComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	FVector NormalImpulse,
	const FHitResult& Hit
)
{
	if (!bExplodeOnHit)
	{
		return;
	}

	FVector ExplosionLocation = FVector(Hit.ImpactPoint);

	if (ExplosionLocation.IsNearlyZero())
	{
		ExplosionLocation = GetActorLocation();
	}

	Explode(ExplosionLocation);
}

void ASimBombProjectile::OnBombStopped(const FHitResult& ImpactResult)
{
	if (!bExplodeOnHit)
	{
		return;
	}

	FVector ExplosionLocation = FVector(ImpactResult.ImpactPoint);

	if (ExplosionLocation.IsNearlyZero())
	{
		ExplosionLocation = GetActorLocation();
	}

	Explode(ExplosionLocation);
}

void ASimBombProjectile::ExplodeByFuse()
{
	if (bHasExploded)
	{
		return;
	}

	Explode(GetActorLocation());
}

void ASimBombProjectile::Explode(const FVector& ExplosionLocation)
{
	if (bHasExploded)
	{
		return;
	}

	bHasExploded = true;

	GetWorldTimerManager().ClearTimer(FuseTimerHandle);

	if (ExplosionComponent)
	{
		ExplosionComponent->ExplodeAtLocation(
			ExplosionLocation,
			this,
			GetOwner()
		);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Bomb projectile cannot explode: ExplosionComponent is missing."));
	}

	OnExplosionTriggered(ExplosionLocation);

	Destroy();
}