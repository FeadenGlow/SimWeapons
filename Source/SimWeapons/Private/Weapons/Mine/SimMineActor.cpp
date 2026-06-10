// Fill out your copyright notice in the Description page of Project Settings.

#include "Weapons/Mine/SimMineActor.h"

#include "Carriers/SimWeaponCarrierComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SphereComponent.h"
#include "Engine/World.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Weapons/SimExplosionComponent.h"

ASimMineActor::ASimMineActor()
{
	PrimaryActorTick.bCanEverTick = false;

	CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionComponent"));
	SetRootComponent(CollisionComponent);

	CollisionComponent->InitSphereRadius(30.0f);
	CollisionComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	CollisionComponent->SetCollisionObjectType(ECC_WorldDynamic);
	CollisionComponent->SetCollisionResponseToAllChannels(ECR_Block);
	CollisionComponent->SetNotifyRigidBodyCollision(true);

	TriggerSphereComponent = CreateDefaultSubobject<USphereComponent>(TEXT("TriggerSphereComponent"));
	TriggerSphereComponent->SetupAttachment(CollisionComponent);
	TriggerSphereComponent->InitSphereRadius(TriggerRadius);
	TriggerSphereComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	TriggerSphereComponent->SetCollisionObjectType(ECC_WorldDynamic);
	TriggerSphereComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	TriggerSphereComponent->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	TriggerSphereComponent->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Overlap);
	TriggerSphereComponent->SetCollisionResponseToChannel(ECC_PhysicsBody, ECR_Overlap);
	TriggerSphereComponent->SetCollisionResponseToChannel(ECC_Vehicle, ECR_Overlap);

	ProjectileMovementComponent = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovementComponent"));
	ProjectileMovementComponent->UpdatedComponent = CollisionComponent;
	ProjectileMovementComponent->bRotationFollowsVelocity = false;
	ProjectileMovementComponent->bShouldBounce = false;
	ProjectileMovementComponent->bSweepCollision = true;
	ProjectileMovementComponent->ProjectileGravityScale = GravityScale;

	ExplosionComponent = CreateDefaultSubobject<USimExplosionComponent>(TEXT("ExplosionComponent"));

	// Mine is explosive, so it can be detonated by explosions and chain reaction.
	bCanDetonateFromExplosion = true;

	// Mine should stay in the world by default.
	LifeTime = 0.0f;

	Damage = 120.0f;
	Speed = 0.0f;

	bDeployOnFirstHit = true;
	bArmAfterDelay = true;
	ArmDelay = 1.5f;
	TriggerRadius = 400.0f;

	bExplodeOnHitWhenArmed = true;
	bExplodeWhenHitByProjectile = true;

	bTriggerOnCarriers = true;
	bTriggerOnExplosiveProjectiles = true;
	bTriggerOnPhysicsObjects = false;
	bCanTriggerOwner = false;

	bExplodeAfterFuse = false;
	FuseTime = 30.0f;

	GravityScale = 1.0f;
	ForwardDropSpeed = 0.0f;
	DownwardDropSpeed = 100.0f;
	MaximumFallSpeed = 2500.0f;
}

void ASimMineActor::BeginPlay()
{
	Super::BeginPlay();

	// Force this value here because Blueprint instances can keep old default values.
	bCanDetonateFromExplosion = true;

	if (CollisionComponent)
	{
		CollisionComponent->OnComponentHit.AddDynamic(this, &ASimMineActor::OnMineHit);

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

	if (TriggerSphereComponent)
	{
		TriggerSphereComponent->SetSphereRadius(TriggerRadius);
		TriggerSphereComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		TriggerSphereComponent->OnComponentBeginOverlap.AddDynamic(this, &ASimMineActor::OnMineTriggerBeginOverlap);
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
	}

	if (!bDeployOnFirstHit)
	{
		DeployMine(FHitResult());
	}

	if (bExplodeAfterFuse && FuseTime > 0.0f)
	{
		GetWorldTimerManager().SetTimer(
			FuseTimerHandle,
			this,
			&ASimMineActor::ExplodeByFuse,
			FuseTime,
			false
		);
	}
}

void ASimMineActor::DetonateFromExplosion_Implementation(const FVector& TriggerLocation)
{
	if (bHasExploded)
	{
		return;
	}

	Explode(GetActorLocation());
}

bool ASimMineActor::IsArmed() const
{
	return bIsArmed;
}

bool ASimMineActor::IsDeployed() const
{
	return bIsDeployed;
}

void ASimMineActor::OnMineHit(
	UPrimitiveComponent* HitComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	FVector NormalImpulse,
	const FHitResult& Hit
)
{
	if (bHasExploded)
	{
		return;
	}

	if (ShouldExplodeFromHit(OtherActor))
	{
		FVector ExplosionLocation = FVector(Hit.ImpactPoint);

		if (ExplosionLocation.IsNearlyZero())
		{
			ExplosionLocation = GetActorLocation();
		}

		Explode(ExplosionLocation);
		return;
	}

	if (!bIsDeployed && bDeployOnFirstHit)
	{
		DeployMine(Hit);
		return;
	}

	if (bIsArmed && bExplodeOnHitWhenArmed && IsValidTriggerActor(OtherActor, OtherComp))
	{
		FVector ExplosionLocation = FVector(Hit.ImpactPoint);

		if (ExplosionLocation.IsNearlyZero())
		{
			ExplosionLocation = GetActorLocation();
		}

		Explode(ExplosionLocation);
	}
}

void ASimMineActor::OnMineTriggerBeginOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult
)
{
	if (bHasExploded || !bIsArmed)
	{
		return;
	}

	if (!IsValidTriggerActor(OtherActor, OtherComp))
	{
		return;
	}

	Explode(GetActorLocation());
}

void ASimMineActor::DeployMine(const FHitResult& Hit)
{
	if (bIsDeployed || bHasExploded)
	{
		return;
	}

	bIsDeployed = true;

	if (ProjectileMovementComponent)
	{
		ProjectileMovementComponent->StopMovementImmediately();
		ProjectileMovementComponent->Deactivate();
	}

	if (CollisionComponent)
	{
		CollisionComponent->SetPhysicsLinearVelocity(FVector::ZeroVector);
		CollisionComponent->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
	}

	if (bArmAfterDelay && ArmDelay > 0.0f)
	{
		GetWorldTimerManager().SetTimer(
			ArmTimerHandle,
			this,
			&ASimMineActor::ArmMine,
			ArmDelay,
			false
		);
	}
	else
	{
		ArmMine();
	}

	OnMineDeployed();

	UE_LOG(LogTemp, Log, TEXT("Mine deployed: %s"), *GetName());
}

void ASimMineActor::ArmMine()
{
	if (bIsArmed || bHasExploded)
	{
		return;
	}

	bIsArmed = true;

	if (TriggerSphereComponent)
	{
		TriggerSphereComponent->SetSphereRadius(TriggerRadius);
		TriggerSphereComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		TriggerSphereComponent->UpdateOverlaps();
	}

	OnMineArmed();

	UE_LOG(LogTemp, Log, TEXT("Mine armed: %s"), *GetName());

	CheckExistingTriggerOverlaps();
}

void ASimMineActor::CheckExistingTriggerOverlaps()
{
	if (bHasExploded || !bIsArmed || !TriggerSphereComponent)
	{
		return;
	}

	TArray<AActor*> OverlappingActors;
	TriggerSphereComponent->GetOverlappingActors(OverlappingActors);

	for (AActor* OverlappingActor : OverlappingActors)
	{
		if (!IsValidTriggerActor(OverlappingActor, nullptr))
		{
			continue;
		}

		Explode(GetActorLocation());
		return;
	}
}

bool ASimMineActor::ShouldExplodeFromHit(AActor* OtherActor) const
{
	if (!OtherActor || OtherActor == this)
	{
		return false;
	}

	if (!bCanTriggerOwner && OtherActor == GetOwner())
	{
		return false;
	}

	const ASimProjectileBase* OtherProjectile = Cast<ASimProjectileBase>(OtherActor);

	if (bExplodeWhenHitByProjectile && OtherProjectile && OtherProjectile != this)
	{
		return true;
	}

	return false;
}

bool ASimMineActor::IsValidTriggerActor(AActor* OtherActor, UPrimitiveComponent* OtherComp) const
{
	if (!OtherActor || OtherActor == this)
	{
		return false;
	}

	if (!bCanTriggerOwner && OtherActor == GetOwner())
	{
		return false;
	}

	const ASimProjectileBase* Projectile = Cast<ASimProjectileBase>(OtherActor);

	if (bTriggerOnExplosiveProjectiles && Projectile && Projectile != this)
	{
		if (Projectile->CanDetonateFromExplosion())
		{
			return true;
		}
	}

	if (bTriggerOnCarriers)
	{
		const USimWeaponCarrierComponent* CarrierComponent =
			OtherActor->FindComponentByClass<USimWeaponCarrierComponent>();

		if (CarrierComponent && CarrierComponent->IsAlive())
		{
			return true;
		}
	}

	if (bTriggerOnPhysicsObjects && OtherComp && OtherComp->IsSimulatingPhysics())
	{
		return true;
	}

	return false;
}

void ASimMineActor::Explode(const FVector& ExplosionLocation)
{
	if (bHasExploded)
	{
		return;
	}

	bHasExploded = true;

	GetWorldTimerManager().ClearTimer(ArmTimerHandle);
	GetWorldTimerManager().ClearTimer(FuseTimerHandle);

	if (TriggerSphereComponent)
	{
		TriggerSphereComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	if (CollisionComponent)
	{
		CollisionComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	SetActorEnableCollision(false);

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
		UE_LOG(LogTemp, Warning, TEXT("Mine cannot explode: ExplosionComponent is missing."));
	}

	OnMineExploded(ExplosionLocation);

	Destroy();
}

void ASimMineActor::ExplodeByFuse()
{
	if (bHasExploded)
	{
		return;
	}

	Explode(GetActorLocation());
}