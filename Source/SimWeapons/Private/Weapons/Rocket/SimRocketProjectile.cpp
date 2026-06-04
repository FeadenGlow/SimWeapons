// Fill out your copyright notice in the Description page of Project Settings.

#include "Weapons/Rocket/SimRocketProjectile.h"

#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"

// Sets default values
ASimRocketProjectile::ASimRocketProjectile()
{
	PrimaryActorTick.bCanEverTick = false;

	CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionComponent"));
	SetRootComponent(CollisionComponent);

	CollisionComponent->InitSphereRadius(15.0f);
	CollisionComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	CollisionComponent->SetCollisionObjectType(ECC_WorldDynamic);
	CollisionComponent->SetCollisionResponseToAllChannels(ECR_Block);
	CollisionComponent->SetNotifyRigidBodyCollision(true);

	ProjectileMovementComponent = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovementComponent"));
	ProjectileMovementComponent->UpdatedComponent = CollisionComponent;
	ProjectileMovementComponent->bRotationFollowsVelocity = true;
	ProjectileMovementComponent->bShouldBounce = false;
	ProjectileMovementComponent->bSweepCollision = true;

	Speed = 3000.0f;
	Damage = 100.0f;
	LifeTime = 5.0f;
}

void ASimRocketProjectile::BeginPlay()
{
	Super::BeginPlay();

	if (CollisionComponent)
	{
		CollisionComponent->OnComponentHit.AddDynamic(this, &ASimRocketProjectile::OnRocketHit);
	}

	if (ProjectileMovementComponent)
	{
		ProjectileMovementComponent->InitialSpeed = Speed;
		ProjectileMovementComponent->MaxSpeed = Speed;
		ProjectileMovementComponent->Velocity = GetActorForwardVector() * Speed;

		ProjectileMovementComponent->OnProjectileStop.AddDynamic(this, &ASimRocketProjectile::OnRocketStopped);
	}
}

void ASimRocketProjectile::OnRocketHit(
	UPrimitiveComponent* HitComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	FVector NormalImpulse,
	const FHitResult& Hit
)
{
	HandleRocketImpact(OtherActor);
}

void ASimRocketProjectile::OnRocketStopped(const FHitResult& ImpactResult)
{
	HandleRocketImpact(ImpactResult.GetActor());
}

void ASimRocketProjectile::HandleRocketImpact(AActor* OtherActor)
{
	if (OtherActor && OtherActor != this && OtherActor != GetOwner())
	{
		UE_LOG(LogTemp, Log, TEXT("Rocket hit actor: %s"), *OtherActor->GetName());

		OtherActor->Destroy();
	}

	Destroy();
}