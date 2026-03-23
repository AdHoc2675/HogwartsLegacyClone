#include "Interactable/InteractableChest.h"

#include "AbilitySystemComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"
#include "Core/HOG_GameplayTags.h"

#include "Abilities/GameplayAbility.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/HOG_PlayerController.h"

#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"

AInteractableChest::AInteractableChest()
{
	PrimaryActorTick.bCanEverTick = false;

	BaseMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("BaseMesh"));
	BaseMesh->SetupAttachment(SceneRoot);
}

bool AInteractableChest::CanInteract_Implementation(AActor* Interactor)
{
	return AbilitySystemComponent
		&& AbilitySystemComponent->HasMatchingGameplayTag(HOGGameplayTags::Interactable_Chest_Closed);
}

void AInteractableChest::HandleInteract(AActor* Interactor)
{
	Super::HandleInteract(Interactor);

	GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Green, TEXT("상호작용 진입 성공"));


	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->RemoveLooseGameplayTag(HOGGameplayTags::Interactable_Chest_Closed);
		AbilitySystemComponent->AddLooseGameplayTag(HOGGameplayTags::Interactable_Chest_Opened);
	}

	bIsOpen = true;

	//if (BaseMesh && OpenMontage)
	//{
	//	if (UAnimInstance* AnimInstance = BaseMesh->GetAnimInstance())
	//	{
	//		GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Cyan, TEXT("AnimInstance 찾음! 몽타주 재생 시도"));
	//		AnimInstance->Montage_Play(OpenMontage, 1.0f);
	//	}
	//	else
	//	{
	//		GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red, TEXT("AnimInstance가 없습니다! AnimBP 세팅 필요"));
	//	}
	//}

// -------------------------------------------------------------
// 아이템(스펠) 지급 및 UI 해금 처리
// -------------------------------------------------------------
	if (SpellAbilityClass && SpellInputTag.IsValid() && SpellIDTag.IsValid() && Interactor)
	{
		// 1. 플레이어(상호작용한 대상)에게 마법 능력(GameplayAbility) 장착
		if (IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(Interactor))
		{
			if (UAbilitySystemComponent* TargetASC = ASI->GetAbilitySystemComponent())
			{
				FGameplayAbilitySpec Spec(SpellAbilityClass, 1);
				Spec.DynamicAbilityTags.AddTag(SpellInputTag); // 지정된 키 입력 태그를 매핑해줌
				TargetASC->GiveAbility(Spec);
			}
		}

		// 2. 입력(키보드)을 받아 스킬을 띄우는 플레이어 컨트롤러를 찾아, 화면 UI 해금 반영
		if (APawn* InteractorPawn = Cast<APawn>(Interactor))
		{
			if (AHOG_PlayerController* PC = Cast<AHOG_PlayerController>(InteractorPawn->GetController()))
			{
				PC->UnlockSpellUI(SpellIDTag);

				// 디버그용 출력
				GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, FString::Printf(TEXT("[%s] 마법 획득 및 해금 완료!"), *SpellIDTag.ToString()));
			}
		}
	}

	// 2. === 시청각 피드백 연출 ===
	FVector EffectLocation = GetActorLocation() + FVector(0.f, 0.f, 50.f); // 상자 살짝 위

	// 파티클 터스리기
	if (RewardVFX)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, RewardVFX, EffectLocation);
	}

	// 사운드 재생
	if (RewardSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, RewardSound, EffectLocation);
	}
}