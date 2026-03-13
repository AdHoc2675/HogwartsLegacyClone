#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "AN_BasicAttackFire.generated.h"

/**
 * BasicAttack Projectile 발사용 AnimNotify
 * 몽타주에서 호출되면 현재 활성화된 GA_Spell_BasicAttack의
 * SpawnBasicAttackActor()를 실행한다.
 */
class UGA_Spell_BasicAttack;

UCLASS()
class HOGWARTSLEGACYCLONE_API UAN_BasicAttackFire : public UAnimNotify
{
	GENERATED_BODY()

public:

	virtual void Notify(
		USkeletalMeshComponent* MeshComp,
		UAnimSequenceBase* Animation
	) override;

};