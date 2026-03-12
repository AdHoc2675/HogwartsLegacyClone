#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "AN_ComboBranch.generated.h"

/**
 * 기본공격 콤보 연결 지점 Notify
 * - 이 시점에 다음 콤보가 예약돼 있으면 즉시 다음 타로 전환 요청
 */
UCLASS(DisplayName="AN_ComboBranch")
class HOGWARTSLEGACYCLONE_API UAN_ComboBranch : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation) override;

#if WITH_EDITOR
	virtual FString GetNotifyName_Implementation() const override;
#endif
};