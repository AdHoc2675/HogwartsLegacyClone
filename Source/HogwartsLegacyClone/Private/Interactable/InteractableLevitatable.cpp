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

	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->AddLooseGameplayTag(HOGGameplayTags::Team_Object);
		AbilitySystemComponent->AddLooseGameplayTag(HOGGameplayTags::Interactable_Levitatable_Grounded);
	}
}

// Called every frame
void AInteractableLevitatable::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

bool AInteractableLevitatable::CanInteract_Implementation(AActor* Interactor)
{
	// 땅에 있을 때만 들어올릴 수 있게 할 경우 태그 검사 로직 추가 가능
	return true; 
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

