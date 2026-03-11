// Fill out your copyright notice in the Description page of Project Settings.


#include "Interactable/InteractableLevitatable.h"
#include "Components/StaticMeshComponent.h"
#include "AbilitySystemComponent.h"
#include "NiagaraComponent.h"
#include "Core/HOG_GameplayTags.h"
#include "HOGDebugHelper.h"

// Sets default values
AInteractableLevitatable::AInteractableLevitatable()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	BaseMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BaseMesh"));
	RootComponent = BaseMesh;
	
	// 물리가 켜져 있어야 엔진 중력/힘 제어가 쉬움
	BaseMesh->SetSimulatePhysics(true);
	BaseMesh->SetCollisionProfileName(TEXT("PhysicsActor"));

	MagicAuraVFXComp = CreateDefaultSubobject<UNiagaraComponent>(TEXT("MagicAuraVFXComp"));
	MagicAuraVFXComp->SetupAttachment(RootComponent);
	MagicAuraVFXComp->SetAutoActivate(false);

	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
}

// Called when the game starts or when spawned
void AInteractableLevitatable::BeginPlay()
{
	Super::BeginPlay();

	// 발판 모드일 경우: 축을 단단히 고정하여 위아래(Z축)로만 움직임
	if (bIsPlatformMode && BaseMesh)
	{
		FBodyInstance* BodyInst = BaseMesh->GetBodyInstance();
		if (BodyInst)
		{
			// 1. 회전 완전 잠금 (플레이어가 모서리쪽에 있어도 기울어지지 않음)
			BodyInst->bLockXRotation = true; // Roll 잠금
			BodyInst->bLockYRotation = true; // Pitch 잠금
			BodyInst->bLockZRotation = true; // Yaw 잠금

			// 2. 수평 이동 잠금 (위아래인 Z축 이동만 허용, 옆으로 밀리지 않음)
			BodyInst->bLockXTranslation = true;
			BodyInst->bLockYTranslation = true;
			
			BodyInst->SetDOFLock(EDOFMode::SixDOF);

			// 3. 질량 증가 (선택 사항: 플레이어 몸무게에 의해 덜덜거리는 것을 방지)
			BaseMesh->SetMassOverrideInKg(NAME_None, 5000.0f, true);
		}
	}

	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->AddLooseGameplayTag(HOGGameplayTags::Team_Object);
		AbilitySystemComponent->AddLooseGameplayTag(HOGGameplayTags::Interactable_Levitatable_Grounded);
	}
}

bool AInteractableLevitatable::CanInteract_Implementation(AActor* Interactor)
{
	return AbilitySystemComponent && AbilitySystemComponent->HasMatchingGameplayTag(HOGGameplayTags::Interactable_Levitatable_Grounded);
}

void AInteractableLevitatable::Interact_Implementation(AActor* Interactor)
{
	if (!IInteractableInterface::Execute_CanInteract(this, Interactor)) return;

	// 상태 태그 변경 로직 (Grounded 분리, Flying 적용 등)
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->AddLooseGameplayTag(HOGGameplayTags::State_Spell_Leviosa_Levitated);
	}

	Debug::Print(TEXT("[Levitatable] 마법 적중, 부유 시작"), FColor::Cyan);

	if (MagicAuraVFXComp)
	{
		MagicAuraVFXComp->Activate(true);
	}

	// 핵심 물리 제어
	if (BaseMesh)
	{
		// 중력 무시
		BaseMesh->SetEnableGravity(false);
		
		// 떠오르는 힘 부여
		BaseMesh->SetPhysicsLinearVelocity(FVector(0.f, 0.f, LevitateForce));
		BaseMesh->SetLinearDamping(5.0f); // 저항을 강하게 줘서 우주로 날아가는걸 방지
		BaseMesh->SetAngularDamping(2.0f); // 회전 저항
	}

	OnLevitated();
}

void AInteractableLevitatable::StopLevitation()
{
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->RemoveLooseGameplayTag(HOGGameplayTags::State_Spell_Leviosa_Levitated);
	}

	Debug::Print(TEXT("[Levitatable] 마법 해제, 부유 종료"), FColor::Red);

	if (MagicAuraVFXComp)
	{
		MagicAuraVFXComp->Deactivate();
	}

	// 중력 및 물리 저항 원상 복구
	if (BaseMesh)
	{
		BaseMesh->SetEnableGravity(true);
		BaseMesh->SetLinearDamping(0.01f); // 기본값 복구
		BaseMesh->SetAngularDamping(0.0f); 
	}

	OnDropped();
}

