#include "GAS/Abilities/GA_SpellBase.h"

#include "HOGDebugHelper.h"
#include "Data/DA_SpellDefinition.h"
#include "GameFramework/HOG_GameInstance.h"

#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"
#include "Character/Player/PlayerCharacterBase.h"
#include "Component/LockOnComponent.h"

UGA_SpellBase::UGA_SpellBase()
{
	// 기본적으로 SpellBase 자체는 “설정(Definition 조회/검증)”만 담당한다.
	// 실제 공격/퍼즐 로직은 파생 Ability에서 구현.
	// 따라서 별도의 정책 변경은 하지 않고 GA_Base의 기본 정책을 사용한다.

	bWarnIfDefinitionMissing = true;
}

UDA_SpellDefinition* UGA_SpellBase::GetSpellDefinition() const
{
	// 1) SpellID 자체가 유효하지 않으면 조회 불가.
	//    (태그가 아직 등록/세팅되지 않았거나, 디폴트 값일 수 있음)
	if (!SpellID.IsValid())
	{
		return nullptr;
	}

	// 2) GAS Ability는 런타임에 CurrentActorInfo가 세팅된다.
	//    (ASC/Owner/Avatar/World 등 컨텍스트를 담고 있음)
	if (!CurrentActorInfo)
	{
		return nullptr;
	}

	// 3) World를 얻고 GameInstance를 얻어온다.
	//    여기서 UHOG_GameInstance는 SpellRegistry를 관리한다.
	
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	UHOG_GameInstance* GI = Cast<UHOG_GameInstance>(World->GetGameInstance());
	if (!GI)
	{
		return nullptr;
	}

	// 4) SpellRegistry에서 SpellID로 Definition 조회
	return GI->GetSpellDefinition(SpellID);
}

UDA_SpellDefinition* UGA_SpellBase::GetSpellDefinitionOrWarn() const
{
	UDA_SpellDefinition* Def = GetSpellDefinition();
	if (Def)
	{
		return Def;
	}

	// 실패했는데, 경고 출력 옵션이 꺼져 있으면 조용히 nullptr 반환
	if (!bWarnIfDefinitionMissing)
	{
		return nullptr;
	}

	// 실패 원인은 다양할 수 있다:
	// - SpellID 미세팅/invalid
	// - GameInstance 캐스팅 실패(프로젝트 설정에서 다른 GI 사용)
	// - GI의 SpellDefinitions 배열에 에셋 등록 안함
	// - BuildSpellRegistry가 호출되지 않음
	const FString Msg = FString::Printf(
		TEXT("[GA_SpellBase] SpellDefinition missing. Ability=%s SpellID=%s"),
		*GetNameSafe(this),
		*SpellID.ToString()
	);

	Debug::Print(Msg, FColor::Yellow);
	return nullptr;
}

float UGA_SpellBase::GetCooldownSeconds() const
{
	// Definition을 조회하고 값 반환. 없으면 0.
	if (UDA_SpellDefinition* Def = GetSpellDefinitionOrWarn())
	{
		return Def->CooldownSeconds;
	}
	return 0.f;
}

float UGA_SpellBase::GetBaseDamage() const
{
	if (UDA_SpellDefinition* Def = GetSpellDefinitionOrWarn())
	{
		return Def->BaseDamage;
	}
	return 0.f;
}

float UGA_SpellBase::GetCastRange() const
{
	if (UDA_SpellDefinition* Def = GetSpellDefinitionOrWarn())
	{
		return Def->CastRange;
	}
	return 0.f;
}

bool UGA_SpellBase::DoesTargetMeetRequirements(AActor* Target) const
{
	// 타겟이 없으면 당연히 실패
	if (!IsValid(Target))
	{
		return false;
	}

	// Definition이 없으면 이 스펠의 조건 자체를 판단할 수 없으므로 실패 처리
	UDA_SpellDefinition* Def = GetSpellDefinitionOrWarn();
	if (!Def)
	{
		return false;
	}

	// 1) BlockedTags: 하나라도 걸리면 실패
	if (IsTargetBlocked(Target, Def->TargetBlockedTags))
	{
		return false;
	}

	// 2) RequiredTags: 모두 만족해야 성공
	if (!HasAllRequiredTags(Target, Def->TargetRequiredTags))
	{
		return false;
	}

	return true;
}

bool UGA_SpellBase::AcquireTargetFromLockOn(AActor*& OutTarget, FGameplayTagContainer& OutTargetTags,
	FVector& OutAimPoint) const
{
	OutTarget = nullptr;
	OutTargetTags.Reset();
	OutAimPoint = FVector::ZeroVector;

	// Definition 필요 (RequiredTags / Range 등)
	UDA_SpellDefinition* Def = GetSpellDefinitionOrWarn();
	if (!Def)
	{
		// Definition 없어도 조준점은 최대한 주기
		GetCenterAimPoint(OutAimPoint, 2000.f);
		return false;
	}

	if (!CurrentActorInfo)
	{
		GetCenterAimPoint(OutAimPoint, Def->CastRange);
		return false;
	}

	AActor* Avatar = CurrentActorInfo->AvatarActor.Get();
	APlayerCharacterBase* PlayerCharacter = Cast<APlayerCharacterBase>(Avatar);
	if (!PlayerCharacter)
	{
		// Avatar가 플레이어 캐릭터가 아닐 수도 있으니 fallback
		GetCenterAimPoint(OutAimPoint, Def->CastRange);
		return false;
	}

	ULockOnComponent* LockOn = PlayerCharacter->GetLockOnComponent();
	if (!LockOn)
	{
		GetCenterAimPoint(OutAimPoint, Def->CastRange);
		return false;
	}

	// LockOn의 탐색 범위는 스펠의 CastRange에 맞춰 동기화(임시 정책)
	LockOn->MaxRange = Def->CastRange;

	FLockOnTargetResult Result;
	const bool bFound = LockOn->FindBestTarget(Def->TargetRequiredTags, Result);

	// 타겟이 없으면 AimPoint만이라도 반환
	if (!bFound || !IsValid(Result.TargetActor))
	{
		// LockOnComponent가 채운 AimPoint가 있으면 그걸 쓰고, 없으면 센터에임 fallback
		if (!Result.AimPoint.IsNearlyZero())
		{
			OutAimPoint = Result.AimPoint;
		}
		else
		{
			GetCenterAimPoint(OutAimPoint, Def->CastRange);
		}
		return false;
	}

	// 2차 검증: 기존 SpellBase 공통 규칙(Blocked/Required)로 최종 판정
	if (!DoesTargetMeetRequirements(Result.TargetActor))
	{
		// 요구조건 불만족이면 타겟 무효 처리(하지만 AimPoint는 타겟 위치로)
		OutAimPoint = Result.TargetActor->GetActorLocation();
		return false;
	}

	OutTarget = Result.TargetActor;
	OutTargetTags = Result.TargetTags;
	OutAimPoint = Result.AimPoint.IsNearlyZero() ? Result.TargetActor->GetActorLocation() : Result.AimPoint;
	return true;
}

bool UGA_SpellBase::GetCenterAimPoint(FVector& OutAimPoint, float RangeOverride) const
{
	if (!CurrentActorInfo)
	{
		return false;
	}

	AActor* Avatar = CurrentActorInfo->AvatarActor.Get();
	if (!Avatar)
	{
		return false;
	}

	APawn* Pawn = Cast<APawn>(Avatar);
	if (!Pawn)
	{
		return false;
	}

	APlayerController* PC = Cast<APlayerController>(Pawn->GetController());
	if (!PC || !PC->PlayerCameraManager)
	{
		return false;
	}

	const float UseRange = (RangeOverride > 0.f) ? RangeOverride : GetCastRange();
	const FVector CamLoc = PC->PlayerCameraManager->GetCameraLocation();
	const FVector CamForward = PC->PlayerCameraManager->GetActorForwardVector().GetSafeNormal();

	OutAimPoint = CamLoc + (CamForward * UseRange);
	return true;
}

bool UGA_SpellBase::IsTargetBlocked(AActor* Target, const FGameplayTagContainer& Blocked) const
{
	// 차단 태그가 비어있으면 차단 없음
	if (Blocked.IsEmpty())
	{
		return false;
	}

	// 1차 구현:
	// - 타겟이 AbilitySystemInterface를 구현하고 있으면,
	//   타겟 ASC의 OwnedGameplayTags를 읽어서 Blocked와 비교한다.
	// - 이는 플레이어/적 같은 “캐릭터 계열”에 적용되는 방식이다.
	if (Target->GetClass()->ImplementsInterface(UAbilitySystemInterface::StaticClass()))
	{
		IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(Target);
		if (ASI)
		{
			if (UAbilitySystemComponent* TargetASC = ASI->GetAbilitySystemComponent())
			{
				FGameplayTagContainer Owned;
				TargetASC->GetOwnedGameplayTags(Owned);
				return Owned.HasAny(Blocked);
			}
		}
	}

	// 퍼즐 오브젝트(ASC 없음)는 현재 버전에서는 “차단 검사 스킵” 처리.
	// 다음 확장 후보:
	// - UHOGTagComponent를 붙여서 GameplayTagContainer를 직접 보유
	// - 또는 ActorTag(FName) -> GameplayTag로 브릿지 비교
	return false;
}

bool UGA_SpellBase::HasAllRequiredTags(AActor* Target, const FGameplayTagContainer& Required) const
{
	// 요구 태그가 비어있으면 항상 통과
	if (Required.IsEmpty())
	{
		return true;
	}

	// 캐릭터/적(ASC 보유) 대상:
	// - ASC의 OwnedTags가 Required를 전부 포함해야 한다.
	if (Target->GetClass()->ImplementsInterface(UAbilitySystemInterface::StaticClass()))
	{
		IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(Target);
		if (ASI)
		{
			if (UAbilitySystemComponent* TargetASC = ASI->GetAbilitySystemComponent())
			{
				FGameplayTagContainer Owned;
				TargetASC->GetOwnedGameplayTags(Owned);
				return Owned.HasAll(Required);
			}
		}
	}

	// 퍼즐 오브젝트(ASC 없음):
	// - 현재는 “요구 태그 만족 불가(false)”로 처리.
	// - 즉, 퍼즐 오브젝트에 RequiredTags 기반 타겟팅을 적용하려면 다음 단계 확장이 필요.
	return false;
}