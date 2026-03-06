#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GAS/Abilities/GA_Base.h"
#include "GA_SpellBase.generated.h"

class UDA_SpellDefinition;
class APlayerCharacterBase;
class ULockOnComponent;

/**
 * UGA_SpellBase
 *
 * [역할]
 * - “모든 스펠(마법) Ability”의 공통 베이스 클래스.
 * - 이 클래스는 스펠의 실제 효과(Accio 당기기, Stupefy 충격 등)를 구현하지 않는다.
 * - 대신 “스펠 데이터(쿨타임/데미지/사거리/타겟 조건)”를 DataAsset로부터 가져오고,
 *   공통 검증(타겟 태그 조건 등)을 제공한다.
 *
 * [데이터 파이프라인]0
 *   (1) UDA_SpellDefinition (PrimaryDataAsset)
 *       - SpellID(게임플레이태그)를 키로, 쿨타임/데미지/사거리/타겟 요구태그 등을 보관
 *   (2) UHOG_GameInstance 의 SpellRegistry
 *       - TMap<SpellID, Definition> 형태로 런타임 조회 가능
 *   (3) UGA_SpellBase (이 클래스)
 *       - SpellID를 가지고 있다가, 실행 시 GameInstance에서 Definition을 조회하여 사용
 *
 * [중요 설계 의도]
 * - 스펠의 “진실의 원천”은 Definition(DataAsset)이다.
 * - 스펠 Ability는 가능한 한 하드코딩을 줄이고, Definition을 읽어서 동작하도록 한다.
 * - 이후 “스펠 매핑(SpellID -> Primary/Alt GAClass)”은 별도 Mapping DataAsset에서 관리 예정.
 *
 * [상속 구조]
 *   UGA_Base
 *     └ UGA_SpellBase (여기)
 *         └ GA_Accio / GA_Stupefy / GA_Leviosa / GA_Incendio ...
 */
UCLASS(Abstract)
class HOGWARTSLEGACYCLONE_API UGA_SpellBase : public UGA_Base
{
	GENERATED_BODY()

public:
	UGA_SpellBase();

	/**
	 * SpellID
	 * - 이 Ability가 담당하는 스펠의 고유 ID.
	 * - 예: Spell.Accio, Spell.Stupefy ...
	 *
	 * 사용 방법:
	 * - 파생 Ability(예: GA_Accio)에서 이 값을 세팅하고,
	 * - 런타임에 GameInstance 레지스트리에서 Definition을 조회한다.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="HOG|Spell")
	FGameplayTag SpellID;

	/**
	 * bWarnIfDefinitionMissing
	 * - Definition 조회 실패(레지스트리에 없거나 SpellID invalid 등) 시,
	 *   경고 메시지를 출력할지 여부.
	 *
	 * 개발 중에는 true 권장.
	 * 릴리즈 빌드에서 로그 노이즈가 크면 false로 바꾸면 됨.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="HOG|Spell|Debug")
	bool bWarnIfDefinitionMissing = true;

public:
	/**
	 * GetSpellDefinition()
	 * - SpellID를 키로 GameInstance SpellRegistry에서 Definition을 조회한다.
	 * - 실패 시 nullptr.
	 *
	 * 주의:
	 * - 이 함수는 로그 출력 X
	 * - 로그까지 원하면 GetSpellDefinitionOrWarn() 사용.
	 */
	UFUNCTION(BlueprintCallable, Category="HOG|Spell")
	UDA_SpellDefinition* GetSpellDefinition() const;

	/**
	 * GetSpellDefinitionOrWarn()
	 * - GetSpellDefinition()과 동일하지만,
	 *   실패 시 Debug::Print로 경고를 출력한다.
	 *
	 * 용도:
	 * - 파생 Ability에서 “반드시 Definition이 있어야 하는” 지점에서 사용.
	 */
	UFUNCTION(BlueprintCallable, Category="HOG|Spell")
	UDA_SpellDefinition* GetSpellDefinitionOrWarn() const;

	/**
	 * 아래 3개는 “Definition의 특정 필드”를 빠르게 꺼내오기 위한 편의 함수.
	 * - Definition이 없으면 0을 반환한다.
	 * - 파생 Ability에서는 직접 Def->CooldownSeconds 이런 식으로 써도 되지만,
	 *   블루프린트 노출이나 공통 사용성을 위해 제공.
	 */
	UFUNCTION(BlueprintCallable, Category="HOG|Spell")
	float GetCooldownSeconds() const;

	UFUNCTION(BlueprintCallable, Category="HOG|Spell")
	float GetBaseDamage() const;

	UFUNCTION(BlueprintCallable, Category="HOG|Spell")
	float GetCastRange() const;

	/**
	 * DoesTargetMeetRequirements()
	 * - “이 스펠이 이 Target에 적용 가능한가?”를 공통으로 판정한다.
	 *
	 * 판정 기준(Definition 기반):
	 *  1) TargetBlockedTags: 하나라도 포함하면 실패
	 *  2) TargetRequiredTags: 모두 포함해야 성공
	 *
	 * 현재 버전의 한계:
	 * - ASC(AbilitySystemComponent)가 있는 Actor만 GameplayTagContainer를 읽어 검사 가능.
	 * - 퍼즐 오브젝트(ASC 없음)는 다음 단계에서 TagComponent/브릿지로 확장 예정.
	 */
	UFUNCTION(BlueprintCallable, Category="HOG|Spell|Targeting")
	bool DoesTargetMeetRequirements(AActor* Target) const;
	
	/**
 * AcquireTargetFromLockOn()
 * - PlayerCharacterBase에 부착된 LockOnComponent를 통해 타겟을 선정한다.
 * - RequiredTargetTags는 SpellDefinition의 TargetRequiredTags를 그대로 사용한다(현재 단계).
 * - 타겟이 없거나 LockOnComponent가 없으면 OutTarget=nullptr, OutAimPoint는 Fallback으로 채운다.
 */
	UFUNCTION(BlueprintCallable, Category="HOG|Spell|Targeting")
	bool AcquireTargetFromLockOn(AActor*& OutTarget, FGameplayTagContainer& OutTargetTags, FVector& OutAimPoint) const;
	

protected:
	/**
	 * IsTargetBlocked()
	 * - Target이 BlockedTags 중 “하나라도” 보유하면 true(=차단)
	 *
	 * 구현상:
	 * - Target이 AbilitySystemInterface를 구현하면 ASC 태그로 검사한다.
	 * - ASC가 없는 경우: 현재 버전에서는 false로 처리(차단 검사 스킵)
	 */
	bool IsTargetBlocked(AActor* Target, const FGameplayTagContainer& Blocked) const;

	/**
	 * HasAllRequiredTags()
	 * - Target이 RequiredTags를 “전부” 보유하면 true
	 *
	 * 구현상:
	 * - Target이 AbilitySystemInterface를 구현하면 ASC 태그로 검사한다.
	 * - ASC가 없는 경우: 현재 버전에서는 false(=요구태그 만족 불가)로 처리.
	 */
	bool HasAllRequiredTags(AActor* Target, const FGameplayTagContainer& Required) const;
	
	bool GetCenterAimPoint(FVector& OutAimPoint, float RangeOverride = -1.f) const;
};