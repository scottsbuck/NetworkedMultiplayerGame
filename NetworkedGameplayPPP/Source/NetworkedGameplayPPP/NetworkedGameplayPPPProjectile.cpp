// Copyright Epic Games, Inc. All Rights Reserved.

#include "NetworkedGameplayPPPProjectile.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/DamageType.h"
#include "Particles/ParticleSystem.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"

ANetworkedGameplayPPPProjectile::ANetworkedGameplayPPPProjectile()
{
	// Use a sphere as a simple collision representation
	sphereComponent = CreateDefaultSubobject<USphereComponent>(TEXT("RootComponent"));
	sphereComponent->InitSphereRadius(37.5f);
	sphereComponent->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	//sphereComponent->OnComponentHit.AddDynamic(this, &ANetworkedGameplayPPPProjectile::OnHit);		// set up a notification for when this component hits something blocking

	// Players can't walk on it
	//sphereComponent->SetWalkableSlopeOverride(FWalkableSlopeOverride(WalkableSlope_Unwalkable, 0.f));
	//sphereComponent->CanCharacterStepUpOn = ECB_No;

	// Set as root component
	RootComponent = sphereComponent;

	if (GetLocalRole() == ROLE_Authority)
	{
		sphereComponent->OnComponentHit.AddDynamic(this, &ANetworkedGameplayPPPProjectile::OnProjectileImpact);
	}

	static ConstructorHelpers::FObjectFinder<UStaticMesh> DefaultMesh(TEXT("/Game/StarterContent/Shapes/Shape_Sphere.Shape_Sphere"));
	staticMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	staticMesh->SetupAttachment(RootComponent);

	if (DefaultMesh.Succeeded())
	{
		staticMesh->SetStaticMesh(DefaultMesh.Object);
		staticMesh->SetRelativeLocation(FVector(0.0f, 0.0f, -37.5));
		staticMesh->SetRelativeScale3D(FVector(0.75f, 0.75f, 0.75f));
	}

	static ConstructorHelpers::FObjectFinder<UParticleSystem> DefaultExplosionEffect(TEXT("/Game/StarterContent/Particles/P_Explosion.P_Explosion"));
	if (DefaultExplosionEffect.Succeeded())
	{
		explosionEffect = DefaultExplosionEffect.Object;
	}

	// Use a ProjectileMovementComponent to govern this projectile's movement
	projectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	projectileMovement->SetUpdatedComponent(sphereComponent);
	projectileMovement->InitialSpeed = 1500.f;
	projectileMovement->MaxSpeed = 1500.f;
	projectileMovement->bRotationFollowsVelocity = true;
	projectileMovement->ProjectileGravityScale = 0.0f;

	// Die after 3 seconds by default
	//InitialLifeSpan = 3.0f;

	damageType = UDamageType::StaticClass();
	damage = 10.0f;

	bReplicates = true;
}

void ANetworkedGameplayPPPProjectile::OnProjectileImpact(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	// Only add impulse and destroy projectile if we hit a physics
	if (OtherActor)
	{
		UGameplayStatics::ApplyPointDamage(OtherActor, damage, NormalImpulse, Hit, GetInstigator()->Controller, this, damageType);
	}

	Destroy();
}

void ANetworkedGameplayPPPProjectile::Destroyed()
{
	FVector spawnLocation = GetActorLocation();
	UGameplayStatics::SpawnEmitterAtLocation(this, explosionEffect, spawnLocation, FRotator::ZeroRotator, true, EPSCPoolMethod::AutoRelease);
}
