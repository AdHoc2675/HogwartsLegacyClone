#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Core/HOG_Struct.h"
#include "BasicAttackActor.generated.h"

class USphereComponent;
class UProjectileMovementComponent;
class UNiagaraComponent;
class UNiagaraSystem;

UCLASS()
class HOGWARTSLEGACYCLONE_API ABasicAttackActor : public AActor
{
	GENERATED_BODY()

public:
	ABasicAttackActor();

protected:
	virtual void BeginPlay() override;

protected:

	/* ==============================
	   Components
	================================ */

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="BasicAttack|Component")
	TObjectPtr<USphereComponent> CollisionSphere;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="BasicAttack|Component")
	TObjectPtr<UProjectileMovementComponent> ProjectileMovement;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="BasicAttack|Component")
	TObjectPtr<UNiagaraComponent> TrailNiagara;

	/* ==============================
	   Runtime Data
	================================ */

	UPROPERTY()
	TObjectPtr<AActor> SourceActor;

	UPROPERTY()
	TObjectPtr<AActor> TargetActor;

	bool bHasHit = false;

public:

	/* ==============================
	   Tuning
	================================ */

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="BasicAttack|Damage")
	float Damage = 10.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="BasicAttack|Projectile")
	float InitialSpeed = 4500.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="BasicAttack|Projectile")
	float MaxSpeed = 4500.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="BasicAttack|Projectile")
	bool bRotationFollowsVelocity = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="BasicAttack|Projectile")
	bool bIsHomingProjectile = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="BasicAttack|Projectile")
	float HomingAccelerationMagnitude = 15000.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="BasicAttack|Life")
	float LifeSeconds = 3.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="BasicAttack|VFX")
	TObjectPtr<UNiagaraSystem> ImpactNiagara;

public:

	/* ==============================
	   Setup
	================================ */

	void InitProjectile(
		AActor* InSourceActor,
		AActor* InTargetActor,
		float InDamage
	);

	void FireToDirection(const FVector& InDirection);

protected:

	/* ==============================
	   Hit Handling
	================================ */

	UFUNCTION()
	void OnOverlap(
		UPrimitiveComponent* OverlappedComp,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 BodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult
	);

	void HandleHitActor(AActor* HitActor, const FHitResult& HitResult);

	void DestroyProjectile();
};