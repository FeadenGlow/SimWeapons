// Fill out your copyright notice in the Description page of Project Settings.

#include "Weapons/Defensive/SimDefensiveProjectile.h"

#include "Components/PrimitiveComponent.h"
#include "Components/SphereComponent.h"
#include "Engine/World.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Weapons/SimExplosionComponent.h"

ASimDefensiveProjectile::ASimDefensiveProjectile()
{
	PrimaryActorTick.bCanEverTick = false;

	CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionComponent"));
	SetRootComponent(CollisionComponent);

	CollisionComponent->InitSphereRadius(12.0f);
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

	// Defensive projectile is explosive, so another explosion can detonate it too.
	bCanDetonateFromExplosion = true;

	// Low direct damage. Main purpose is chain reaction, not killing carriers.
	Damage = 20.0f;
	Speed = 2200.0f;
	LifeTime = 5.0f;

	MaximumSpeed = 3000.0f;
	GravityScale = 0.4f;

	bExplodeOnHit = true;
	bUseAirBurst = true;
	AirBurstTime = 1.0f;
}

void ASimDefensiveProjectile::BeginPlay()
{
	Super::BeginPlay();

	// Force this value because Blueprint instances can keep old default values.
	bCanDetonateFromExplosion = true;

	if (CollisionComponent)
	{
		CollisionComponent->OnComponentHit.AddDynamic(
			this,
			&ASimDefensiveProjectile::OnDefensiveProjectileHit
		);

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

		ProjectileMovementComponent->OnProjectileStop.AddDynamic(
			this,
			&ASimDefensiveProjectile::OnDefensiveProjectileStopped
		);
	}

	if (bUseAirBurst && AirBurstTime > 0.0f)
	{
		GetWorldTimerManager().SetTimer(
			AirBurstTimerHandle,
			this,
			&ASimDefensiveProjectile::ExplodeByAirBurst,
			AirBurstTime,
			false
		);
	}

	OnDefensiveProjectileLaunched();
}

void ASimDefensiveProjectile::InitializeDefensiveProjectile(
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

void ASimDefensiveProjectile::AddIgnoredActor(AActor* ActorToIgnore)
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

void ASimDefensiveProjectile::DetonateFromExplosion_Implementation(const FVector& TriggerLocation)
{
	if (bHasExploded)
	{
		return;
	}

	Explode(GetActorLocation());
}

void ASimDefensiveProjectile::OnDefensiveProjectileHit(
	UPrimitiveComponent* HitComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	FVector NormalImpulse,
	const FHitResult& Hit
)
{
	if (bHasExploded || !bExplodeOnHit)
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

void ASimDefensiveProjectile::OnDefensiveProjectileStopped(const FHitResult& ImpactResult)
{
	if (bHasExploded || !bExplodeOnHit)
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

void ASimDefensiveProjectile::ExplodeByAirBurst()
{
	if (bHasExploded)
	{
		return;
	}

	Explode(GetActorLocation());
}

void ASimDefensiveProjectile::Explode(const FVector& ExplosionLocation)
{
	if (bHasExploded)
	{
		return;
	}

	bHasExploded = true;

	GetWorldTimerManager().ClearTimer(AirBurstTimerHandle);

	if (CollisionComponent)
	{
		CollisionComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	SetActorEnableCollision(false);

	if (ProjectileMovementComponent)
	{
		ProjectileMovementComponent->StopMovementImmediately();
		ProjectileMovementComponent->Deactivate();
	}

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
		UE_LOG(LogTemp, Warning, TEXT("Defensive projectile cannot explode: ExplosionComponent is missing."));
	}

	OnDefensiveProjectileExploded(ExplosionLocation);

	Destroy();
}