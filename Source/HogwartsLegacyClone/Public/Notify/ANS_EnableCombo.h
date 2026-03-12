#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "ANS_EnableCombo.generated.h"

/**
 * 기본공격 콤보 입력 가능 구간을 열어주는 NotifyState
 */
UCLASS(DisplayName="ANS_EnableCombo")
class HOGWARTSLEGACYCLONE_API UANS_EnableCombo : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	virtual void NotifyBegin(
		USkeletalMeshComponent* MeshComp,
		UAnimSequenceBase* Animation,
		float TotalDuration,
		const FAnimNotifyEventReference& EventReference
	) override;

	virtual void NotifyEnd(
		USkeletalMeshComponent* MeshComp,
		UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference
	) override;

#if WITH_EDITOR
	virtual FString GetNotifyName_Implementation() const override;
#endif
};