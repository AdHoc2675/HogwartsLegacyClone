// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "DA_SpellDefinition.generated.h"

/**
 * 
 * 스펠의 데이터 값
 * GA_Spell 들은 ID(TAG) 로 이곳에 있는 값들을 조회해서 사용한다.
 */
UCLASS(BlueprintType)
class HOGWARTSLEGACYCLONE_API UDA_SpellDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()
	
public:
	//스펠의 고유 아이디 ID (ex. Sepll.Accio -> tag 로 Attack/lightattack 햇던것 처럼 spell Accio 로  잡으면 됨.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Spell|ID")
	FGameplayTag SpellID;
	
	//UI 표시명 (HUD화면에 띄울 텍스트. 우리 게임 특성상 플레이 중인 화면에서는 없지만 추후 스킬 교체 UI를 제작한다던가, 혹은 보상으로 스펠을 얻을때 등 스킬의 이름을 가져와야 될 때 쓰임
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Spell|UI")
	FText DisplayName;
	
	//아이콘 (UI 같은거에 나올 이미지.)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Spell|UI")
	TObjectPtr<UTexture2D> Icon = nullptr;
	
	//쿨타임
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Spell|Combat", meta=(ClampMin="0.0"))
	float CooldownSeconds = 0.f;
	
	//기본데미지
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Spell|Combat", meta=(ClampMin="0.0"))
	float BaseDamage = 0.f;
	
	//기본사거리
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Spell|Targeting", meta=(ClampMin="0.0"))
	float CastRange = 1500.f;
	
	// 타겟에게 “반드시” 있어야 하는 태그들 (예: Target.Object, Object.Movable, Target.Enemy ...)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Spell|Targeting")
	FGameplayTagContainer TargetRequiredTags;

	// 타겟에게 있으면 “불가” 처리하는 태그들 (예: State.Invulnerable 등)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Spell|Targeting")
	FGameplayTagContainer TargetBlockedTags;
	
public:
	// 유효성 체크(디버그/빌드 검증용)
	UFUNCTION(BlueprintCallable, Category="Spell")
	bool IsValidDefinition() const;
	
};
