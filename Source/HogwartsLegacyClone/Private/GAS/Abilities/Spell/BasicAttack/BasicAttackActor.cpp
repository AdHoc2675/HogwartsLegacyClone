#include "GAS/Abilities/Spell/BasicAttack/BasicAttackActor.h"

#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"

#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"

#include "Character/BaseCharacter.h"
#include "Component/CombatComponent.h"

#include "HOGDebugHelper.h"

ABasicAttackActor::ABasicAttackActor()
{
	PrimaryActorTick.bCanEverTick = false;

	CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
	SetRootComponent(CollisionSphere);

	CollisionSphere->InitSphereRadius(12.f);
	CollisionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CollisionSphere->SetCollisionObjectType(ECC_WorldDynamic);
	CollisionSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	CollisionSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	CollisionSphere->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Overlap);
	CollisionSphere->SetGenerateOverlapEvents(true);

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->UpdatedComponent = CollisionSphere;

	ProjectileMovement->ProjectileGravityScale = 0.f;
	ProjectileMovement->bRotationFollowsVelocity = bRotationFollowsVelocity;
	ProjectileMovement->bShouldBounce = false;

	TrailNiagara = CreateDefaultSubobject<UNiagaraComponent>(TEXT("TrailNiagara"));
	TrailNiagara->SetupAttachment(RootComponent);
	TrailNiagara->SetAutoActivate(true);

	InitialLifeSpan = 0.f;
}

void ABasicAttackActor::BeginPlay()
{
	Super::BeginPlay();

	CollisionSphere->OnComponentBeginOverlap.AddDynamic(this, &ABasicAttackActor::OnOverlap);

	ProjectileMovement->InitialSpeed = InitialSpeed;
	ProjectileMovement->MaxSpeed = MaxSpeed;

	SetLifeSpan(LifeSeconds);
}

void ABasicAttackActor::InitProjectile(
	AActor* InSourceActor,
	AActor* InTargetActor,
	float InDamage
)
{
	SourceActor = InSourceActor;
	TargetActor = InTargetActor;
	Damage = InDamage;

	if (bIsHomingProjectile && IsValid(TargetActor))
	{
		ProjectileMovement->bIsHomingProjectile = true;
		ProjectileMovement->HomingTargetComponent = TargetActor->GetRootComponent();
		ProjectileMovement->HomingAccelerationMagnitude = HomingAccelerationMagnitude;
	}
	else
	{
		ProjectileMovement->bIsHomingProjectile = false;
	}
}

void ABasicAttackActor::FireToDirection(const FVector& InDirection)
{
	if (!ProjectileMovement)
		return;

	const FVector Dir = InDirection.GetSafeNormal();

	ProjectileMovement->Velocity = Dir * InitialSpeed;

	SetActorRotation(Dir.Rotation());
}

void ABasicAttackActor::OnOverlap(
	UPrimitiveComponent* OverlappedComp,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 BodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult
)
{
	if (bHasHit)
		return;

	if (!IsValid(OtherActor))
		return;

	if (OtherActor == SourceActor)
		return;

	HandleHitActor(OtherActor, SweepResult);
}

void ABasicAttackActor::HandleHitActor(AActor* HitActor, const FHitResult& HitResult)
{
	bHasHit = true;

	if (ImpactNiagara)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			this,
			ImpactNiagara,
			HitResult.ImpactPoint,
			HitResult.ImpactNormal.Rotation()
		);
	}

	ABaseCharacter* SourceCharacter = Cast<ABaseCharacter>(SourceActor);
	ABaseCharacter* HitCharacter = Cast<ABaseCharacter>(HitActor);

	if (SourceCharacter && HitCharacter)
	{
		if (UCombatComponent* CombatComp = HitCharacter->GetCombatComponent())
		{
			FDamageRequest DamageRequest;

			DamageRequest.SourceActor = SourceActor;
			DamageRequest.TargetActor = HitActor;
			DamageRequest.InstigatorActor = SourceActor;
			DamageRequest.DamageCauser = this;
			DamageRequest.BaseDamage = Damage;
			DamageRequest.HitResult = HitResult;

			CombatComp->ApplyDamageRequest(DamageRequest);
		}
	}

	DestroyProjectile();
}

void ABasicAttackActor::DestroyProjectile()
{
	SetActorEnableCollision(false);

	if (CollisionSphere)
	{
		CollisionSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	if (TrailNiagara)
	{
		TrailNiagara->Deactivate();
	}

	Destroy();
}