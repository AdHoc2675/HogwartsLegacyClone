#include "GAS/Abilities/Spell/BasicAttack/BasicAttackActor.h"

#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"

#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"

#include "Character/BaseCharacter.h"
#include "Component/CombatComponent.h"
#include "Components/SkeletalMeshComponent.h"

ABasicAttackActor::ABasicAttackActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

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

	// 투사체 Root에 붙어 있더라도 Niagara 자체는 월드 기준 위치/회전을 직접 세팅해서 쓰기 위함
	TrailNiagara->SetAbsolute(true, true, true);

	InitialLifeSpan = 0.f;
}

void ABasicAttackActor::BeginPlay()
{
	Super::BeginPlay();

	CollisionSphere->OnComponentBeginOverlap.AddDynamic(this, &ABasicAttackActor::OnOverlap);

	ProjectileMovement->InitialSpeed = InitialSpeed;
	ProjectileMovement->MaxSpeed = MaxSpeed;
	ProjectileMovement->bRotationFollowsVelocity = bRotationFollowsVelocity;

	if (TrailNiagara)
	{
		TrailNiagara->Activate(true);
	}

	UpdateBeamVFX();

	SetLifeSpan(LifeSeconds);
}

void ABasicAttackActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bHasHit)
	{
		return;
	}

	// Homing 중이었는데 타겟이 죽었거나 무효가 되면
	// 되돌아가거나 이상하게 꺾이는 현상 방지용으로 유도만 끈다.
	if (ProjectileMovement && ProjectileMovement->bIsHomingProjectile)
	{
		bool bShouldDisableHoming = false;

		// 1) 타겟 자체가 무효
		if (!IsValid(TargetActor))
		{
			bShouldDisableHoming = true;
		}
		else
		{
			// 2) 타겟이 BaseCharacter이고, 이미 죽은 상태
			if (const ABaseCharacter* TargetCharacter = Cast<ABaseCharacter>(TargetActor))
			{
				if (TargetCharacter->IsDead())
				{
					bShouldDisableHoming = true;
				}
			}

			// 3) HomingTargetComponent 자체가 사라졌거나 무효
			if (!ProjectileMovement->HomingTargetComponent.IsValid())
			{
				bShouldDisableHoming = true;
			}
		}

		if (bShouldDisableHoming)
		{
			// 현재 속도 방향은 유지하고, 유도만 끊는다.
			const FVector CurrentVelocity = ProjectileMovement->Velocity;
			const float CurrentSpeed = CurrentVelocity.Length();
			const FVector CurrentDirection = CurrentVelocity.IsNearlyZero()
				? GetActorForwardVector()
				: CurrentVelocity.GetSafeNormal();

			ProjectileMovement->bIsHomingProjectile = false;
			ProjectileMovement->HomingTargetComponent = nullptr;
			TargetActor = nullptr;

			ProjectileMovement->Velocity = CurrentDirection * CurrentSpeed;
			SetActorRotation(CurrentDirection.Rotation());
		}
	}

	// 데미지를 주지 못하고 일정 거리 이상 날아갔으면 삭제
	if (bHasSpawnLocation)
	{
		const float TravelDistance = FVector::Dist(SpawnLocation, GetActorLocation());
		if (TravelDistance >= MaxTravelDistance)
		{
			DestroyProjectile();
			return;
		}
	}

	UpdateBeamVFX();
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
		ProjectileMovement->HomingTargetComponent = nullptr;
	}
}

void ABasicAttackActor::FireToDirection(const FVector& InDirection)
{
	if (!ProjectileMovement)
	{
		return;
	}

	const FVector Dir = InDirection.GetSafeNormal();

	// 실제 발사 시작 위치 저장
	SpawnLocation = GetActorLocation();
	bHasSpawnLocation = true;

	ProjectileMovement->Velocity = Dir * InitialSpeed;
	SetActorRotation(Dir.Rotation());

	UpdateBeamVFX();
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
	{
		return;
	}

	if (!IsValid(OtherActor))
	{
		return;
	}

	if (OtherActor == SourceActor)
	{
		return;
	}

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

FVector ABasicAttackActor::GetBeamStartLocation() const
{
	if (!IsValid(SourceActor))
	{
		return GetActorLocation();
	}

	// SourceActor가 캐릭터일 때 메시 소켓에서 시작
	if (ACharacter* SourceCharacter = Cast<ACharacter>(SourceActor))
	{
		if (USkeletalMeshComponent* MeshComp = SourceCharacter->GetMesh())
		{
			if (MeshComp->DoesSocketExist(WandSocketName))
			{
				return MeshComp->GetSocketLocation(WandSocketName);
			}
		}
	}

	return SourceActor->GetActorLocation();
}

void ABasicAttackActor::UpdateBeamVFX()
{
	if (!TrailNiagara)
	{
		return;
	}

	const FVector BeamStart = GetBeamStartLocation();
	const FVector BeamEnd = GetActorLocation();

	const FVector BeamDelta = BeamEnd - BeamStart;
	const float BeamLength = BeamDelta.Length();

	const FVector BeamDirection = BeamDelta.IsNearlyZero()
		? GetActorForwardVector()
		: BeamDelta.GetSafeNormal();

	const FVector BeamMidPoint = BeamStart + (BeamDirection * (BeamLength * 0.5f));

	// Beam 방식에서는 Start / End를 Niagara 내부에서 직접 사용한다.
	// 따라서 컴포넌트 자체를 MidPoint로 옮기거나 회전시키지 않는다.
	TrailNiagara->SetVectorParameter(BeamStartParameterName, BeamStart);
	TrailNiagara->SetVectorParameter(BeamEndParameterName, BeamEnd);
	TrailNiagara->SetFloatParameter(BeamLengthParameterName, BeamLength);
	TrailNiagara->SetVectorParameter(BeamDirectionParameterName, BeamDirection);
	TrailNiagara->SetVectorParameter(BeamMidPointParameterName, BeamMidPoint);
}