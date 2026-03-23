#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BasicAttackActor.generated.h"

class USphereComponent;
class UProjectileMovementComponent;
class UNiagaraComponent;
class UNiagaraSystem;
class UPrimitiveComponent;

UCLASS()
class HOGWARTSLEGACYCLONE_API ABasicAttackActor : public AActor
{
	GENERATED_BODY()

public:
	ABasicAttackActor();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

public:
	void InitProjectile(
		AActor* InSourceActor,
		AActor* InTargetActor,
		float InDamage
	);

	void FireToDirection(const FVector& InDirection);

protected:
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

	FVector GetBeamStartLocation() const;
	void UpdateBeamVFX();

protected:
	/* ==============================
	   Components
	================================ */

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="BasicAttack")
	TObjectPtr<USphereComponent> CollisionSphere;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="BasicAttack")
	TObjectPtr<UProjectileMovementComponent> ProjectileMovement;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="BasicAttack")
	TObjectPtr<UNiagaraComponent> TrailNiagara;

	/* ==============================
	   VFX
	================================ */

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="VFX")
	TObjectPtr<UNiagaraSystem> ImpactNiagara;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="VFX")
	FName BeamStartParameterName = TEXT("BeamStart");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="VFX")
	FName BeamEndParameterName = TEXT("BeamEnd");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="VFX")
	FName BeamLengthParameterName = TEXT("BeamLength");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="VFX")
	FName BeamDirectionParameterName = TEXT("BeamDirection");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="VFX")
	FName BeamMidPointParameterName = TEXT("BeamMidPoint");

	/* ==============================
	   Projectile
	================================ */

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Projectile")
	float InitialSpeed = 3000.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Projectile")
	float MaxSpeed = 3000.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Projectile")
	float LifeSeconds = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Projectile")
	bool bRotationFollowsVelocity = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Projectile")
	bool bIsHomingProjectile = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Projectile", meta=(EditCondition="bIsHomingProjectile"))
	float HomingAccelerationMagnitude = 12000.f;

	// 데미지를 주지 못하고 이 거리 이상 날아가면 삭제
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Projectile")
	float MaxTravelDistance = 2000.f;

	/* ==============================
	   Runtime
	================================ */

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Runtime")
	TObjectPtr<AActor> SourceActor;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Runtime")
	TObjectPtr<AActor> TargetActor;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Runtime")
	float Damage = 0.f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Runtime")
	bool bHasHit = false;

	// 실제 발사 시작 위치
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Runtime")
	FVector SpawnLocation = FVector::ZeroVector;

	// 실제 발사 시작 여부
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Runtime")
	bool bHasSpawnLocation = false;

	/* ==============================
	   Socket
	================================ */

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Socket")
	FName WandSocketName = TEXT("RightHandWandSocket");
};