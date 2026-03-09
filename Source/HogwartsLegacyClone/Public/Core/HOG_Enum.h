#pragma once

#include "CoreMinimal.h"
#include "HOG_Enum.generated.h"

/**
 * 스펠 발동 맥락
 * - Normal: 일반 입력 발동
 * - ParryCounter: 패링 성공 후 반격 발동
 * - SpecialFreeCast: 이벤트/특수 조건 무료 발동
 */
UENUM(BlueprintType)
enum class ESpellCastContext : uint8
{
	Normal UMETA(DisplayName = "Normal"),
	ParryCounter UMETA(DisplayName = "Parry Counter"),
	SpecialFreeCast UMETA(DisplayName = "Special Free Cast")
};

/**
 * 스펠 발동 실패 사유를 코드상으로 분기하기 위한 결과값
 * UI/디버그/로그에서 문자열 대신 enum으로 다루고 싶을 때 사용
 */
UENUM(BlueprintType)
enum class ESpellCastFailReason : uint8
{
	None UMETA(DisplayName = "None"),
	InvalidSpellID UMETA(DisplayName = "Invalid Spell ID"),
	InvalidOwner UMETA(DisplayName = "Invalid Owner"),
	SpellCastingLocked UMETA(DisplayName = "Spell Casting Locked"),
	BlockedByStateTag UMETA(DisplayName = "Blocked By State Tag"),
	SpellOnCooldown UMETA(DisplayName = "Spell On Cooldown")
};
