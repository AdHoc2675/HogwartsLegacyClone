#include "Notify/AN_ComboBranch.h"

#include "Character/Player/PlayerCharacterBase.h"
#include "GAS/HOGAbilitySystemComponent.h"
#include "GAS/Abilities/Spell/BasicAttack/GA_Spell_BasicAttack.h"
#include "AbilitySystemComponent.h"
#include "GameplayAbilitySpec.h"
#include "Components/SkeletalMeshComponent.h"
#include "HOGDebugHelper.h"

void UAN_ComboBranch::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	if (!MeshComp)
	{
		return;
	}

	APlayerCharacterBase* PlayerCharacter = Cast<APlayerCharacterBase>(MeshComp->GetOwner());
	if (!PlayerCharacter)
	{
		return;
	}

	UHOGAbilitySystemComponent* HOGASC = PlayerCharacter->GetHOGAbilitySystemComponent();
	if (!HOGASC)
	{
		// Debug::Print(TEXT("[AN_ComboBranch] Failed: HOGASC is null"), FColor::Red);
		return;
	}

	// 현재 활성 중인 BasicAttack Ability 인스턴스를 찾아서 브랜치 요청
	for (FGameplayAbilitySpec& Spec : HOGASC->GetActivatableAbilities())
	{
		if (!Spec.IsActive())
		{
			continue;
		}

		for (UGameplayAbility* AbilityInstance : Spec.GetAbilityInstances())
		{
			UGA_Spell_BasicAttack* BasicAttackAbility = Cast<UGA_Spell_BasicAttack>(AbilityInstance);
			if (!BasicAttackAbility)
			{
				continue;
			}

			// Debug::Print(FString::Printf(
			// 	TEXT("[AN_ComboBranch] Notify | Found Active BasicAttack Ability=%s"),
			// 	*GetNameSafe(BasicAttackAbility)
			// ), FColor::Green);

			// 다음 단계에서 이 함수 구현 예정
			BasicAttackAbility->TryAdvanceComboFromBranchPoint();
			return;
		}
	}

	//Debug::Print(TEXT("[AN_ComboBranch] No Active BasicAttack Ability Found"), FColor::Orange);
}

#if WITH_EDITOR
FString UAN_ComboBranch::GetNotifyName_Implementation() const
{
	return TEXT("ComboBranch");
}
#endif