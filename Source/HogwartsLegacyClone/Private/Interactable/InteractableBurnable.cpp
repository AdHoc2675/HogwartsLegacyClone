#include "Interactable/InteractableBurnable.h"
#include "Components/StaticMeshComponent.h"
#include "NiagaraComponent.h"
#include "AbilitySystemComponent.h"
#include "Core/HOG_GameplayTags.h"
#include "HOGDebugHelper.h"

AInteractableBurnable::AInteractableBurnable()
{
	PrimaryActorTick.bCanEverTick = false;

	BaseMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BaseMesh"));
	RootComponent = BaseMesh;

	FireVFXComp = CreateDefaultSubobject<UNiagaraComponent>(TEXT("FireVFXComp"));
	FireVFXComp->SetupAttachment(RootComponent);
	FireVFXComp->SetAutoActivate(false); // 초기엔 불꽃 꺼짐 상태

	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
}

void AInteractableBurnable::BeginPlay()
{
	Super::BeginPlay();

	if (AbilitySystemComponent)
	{
		// 이 오브젝트도 상호작용 가능한 객체임을 명시
		AbilitySystemComponent->AddLooseGameplayTag(HOGGameplayTags::Team_Object);

		// 초기 상태를 '불 꺼짐(Unlit)'으로 설정
		AbilitySystemComponent->AddLooseGameplayTag(HOGGameplayTags::Interactable_Burnable_Unlit);
	}
}

bool AInteractableBurnable::CanInteract_Implementation(AActor* Interactor)
{
	// ASC가 유효하며, 현재 "불이 꺼진" 상태일 때만 상호작용(점화) 가능
	return AbilitySystemComponent && AbilitySystemComponent->HasMatchingGameplayTag(HOGGameplayTags::Interactable_Burnable_Unlit);
}

void AInteractableBurnable::Interact_Implementation(AActor* Interactor)
{
	if (!CanInteract(Interactor))
	{
		return;
	}

	if (AbilitySystemComponent)
	{
		// 상태 전환: 불 꺼짐 제거, 불 켜짐 추가
		AbilitySystemComponent->RemoveLooseGameplayTag(HOGGameplayTags::Interactable_Burnable_Unlit);
		AbilitySystemComponent->AddLooseGameplayTag(HOGGameplayTags::Interactable_Burnable_Lit);
	}

	FString InteractorName = Interactor ? Interactor->GetName() : TEXT("Unknown");
	Debug::Print(FString::Printf(TEXT("[Burnable] %s에 마법이 적중하여 점화 (By %s)"), *GetName(), *InteractorName), FColor::Orange);


	// 불꽃 시각 효과 켜기
	if (FireVFXComp)
	{
		FireVFXComp->Activate(true);
	}

	// 블루프린트 측 이벤트 호출 (사운드 재생 등에 활용)
	PlayIgniteEffects();
}