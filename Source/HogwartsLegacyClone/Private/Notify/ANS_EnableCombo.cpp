#include "Notify/ANS_EnableCombo.h"

#include "Character/Player/PlayerCharacterBase.h"
#include "HOGDebugHelper.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Actor.h"

void UANS_EnableCombo::NotifyBegin(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	float TotalDuration,
	const FAnimNotifyEventReference& EventReference)
{
	if (!MeshComp)
	{
		return;
	}

	AActor* OwnerActor = MeshComp->GetOwner();
	if (!OwnerActor)
	{
		return;
	}

	APlayerCharacterBase* PlayerCharacter = Cast<APlayerCharacterBase>(OwnerActor);
	if (!PlayerCharacter)
	{
		return;
	}

	// 여기서 캐릭터에 "다음 콤보 입력 가능" ON
	PlayerCharacter->SetCanQueueNextCombo(true);

	// Debug::Print(FString::Printf(
	// 	TEXT("[ANS_EnableCombo] NotifyBegin | Owner=%s | ComboWindow=OPEN"),
	// 	*GetNameSafe(PlayerCharacter)
	// ), FColor::Green);
}

void UANS_EnableCombo::NotifyEnd(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	if (!MeshComp)
	{
		return;
	}

	AActor* OwnerActor = MeshComp->GetOwner();
	if (!OwnerActor)
	{
		return;
	}

	APlayerCharacterBase* PlayerCharacter = Cast<APlayerCharacterBase>(OwnerActor);
	if (!PlayerCharacter)
	{
		return;
	}

	// 여기서 캐릭터에 "다음 콤보 입력 가능" OFF
	PlayerCharacter->SetCanQueueNextCombo(false);

	// Debug::Print(FString::Printf(
	// 	TEXT("[ANS_EnableCombo] NotifyEnd | Owner=%s | ComboWindow=CLOSE"),
	// 	*GetNameSafe(PlayerCharacter)
	// ), FColor::Orange);
}


#if WITH_EDITOR
FString UANS_EnableCombo::GetNotifyName_Implementation() const
{
	return TEXT("EnableCombo");
}
#endif