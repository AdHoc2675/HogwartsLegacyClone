#pragma once

#include "CoreMinimal.h"
#include "Interactable/InteractableBase.h"
#include "InteractableChest.generated.h"

class USkeletalMeshComponent;
class UAnimMontage;
class UGameplayAbility;

UCLASS()
class HOGWARTSLEGACYCLONE_API AInteractableChest : public AInteractableBase
{
	GENERATED_BODY()
	
public:
	AInteractableChest();

public:
	virtual bool CanInteract_Implementation(AActor* Interactor) override;

protected:
	virtual void HandleInteract(AActor* Interactor) override;

public:

protected:
	// 상자 메쉬
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Mesh")
	TObjectPtr<USkeletalMeshComponent> BaseMesh;

	// 상자가 열릴 때 재생할 몽타주
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Animation")
	TObjectPtr<UAnimMontage> OpenMontage;

	// ======== 보상 지급(해금)용 설정 ========

	// 해금할 마법의 Ability 클래스 (BP_GA_Spell_Leviosa 등)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reward")
	TSubclassOf<UGameplayAbility> SpellAbilityClass;

	// 조작 키와 연결할 입력 태그 (예: Input.Spell.Leviosa)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reward")
	FGameplayTag SpellInputTag;

	// UI 자물쇠 아이콘을 풀 스펠 ID (예: Spell.Leviosa)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reward")
	FGameplayTag SpellIDTag;
};