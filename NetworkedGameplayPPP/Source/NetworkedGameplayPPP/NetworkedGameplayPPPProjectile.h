// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NetworkedGameplayPPPProjectile.generated.h"

class USphereComponent;
class UProjectileMovementComponent;

UCLASS(config=Game)
class ANetworkedGameplayPPPProjectile : public AActor
{
	GENERATED_BODY()

public:

	/** Sphere collision component */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	class USphereComponent* sphereComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class UStaticMeshComponent* staticMesh;

	/** Projectile movement component */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class UProjectileMovementComponent* projectileMovement;

	UPROPERTY(EditAnywhere, Category = "Effects")
	class UParticleSystem* explosionEffect;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Damage")
	TSubclassOf<class UDamageType> damageType;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Damage")
	float damage;

public:
	ANetworkedGameplayPPPProjectile();

	/** Returns CollisionComp subobject **/
	USphereComponent* GetCollisionComp() const { return sphereComponent; }
	/** Returns ProjectileMovement subobject **/
	UProjectileMovementComponent* GetProjectileMovement() const { return projectileMovement; }

protected:
	virtual void Destroyed() override;

	/** called when projectile hits something */
	UFUNCTION(Category = "Projectile")
	void OnProjectileImpact(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

};

