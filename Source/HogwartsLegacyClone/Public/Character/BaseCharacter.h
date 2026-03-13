// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GameplayTagContainer.h"

#include "BaseCharacter.generated.h"

class UCombatComponent;
class UDataTable;
class UHOGAttributeSet;
class UAbilitySystemComponent;


/**
 * 공통 캐릭터 베이스
 * - TeamTag (GameplayTag) : 값은 자식(Player/Enemy)에서 기본값으로 세팅
 * - 최소 사망 상태
 * - Possess/PlayerState 훅 자리 (GAS 초기화 훅으로 사용)
 *
 * 죽음 판정은 AttributeSet(PostGameplayEffectExecute 등)에서 수행하고,
 * 여기서는 "사망 처리(HandleDeath)"만 담당한다.
 */


UCLASS()
class HOGWARTSLEGACYCLONE_API ABaseCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ABaseCharacter();

	// -------------------------
	// Tag
	// -------------------------
	UFUNCTION(BlueprintCallable, Category="Tag|Team")
	FGameplayTag GetTeamTag() const { return TeamTag; }

	UFUNCTION(BlueprintCallable, Category="Tag|Team")
	void SetTeamTag(FGameplayTag NewTeamTag);

	UFUNCTION(BlueprintCallable, Category="Tag|Team")
	bool HasTeamTag(FGameplayTag QueryTag) const;

	// -------------------------
	// State
	// -------------------------
	UFUNCTION(BlueprintCallable, Category="State")
	bool IsDead() const { return bIsDead; }

	UFUNCTION(BlueprintCallable, Category="State")
	virtual void Die();

	// -------------------------
	// Component
	// -------------------------
	UFUNCTION(BlueprintPure, Category="Component")
	UCombatComponent* GetCombatComponent() const { return CombatComponent; }

protected:
	virtual void BeginPlay() override;

	// (나중에) PlayerState 기반 GAS 초기화 훅 자리
	virtual void PossessedBy(AController* NewController) override;
	virtual void OnRep_PlayerState() override;

	/**
	 * 공통 최소 사망 처리 훅
	 * - 연출/드랍/리스폰은 하위(Player/Enemy)에서 override
	 */
	UFUNCTION(BlueprintNativeEvent, Category="State")
	void HandleDeath();
	virtual void HandleDeath_Implementation();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Component", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UCombatComponent> CombatComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Tag|Team")
	FGameplayTag TeamTag;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="State")
	bool bIsDead = false;

protected:
	/* Attribute 초기값 DT */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="HOG|Attributes")
	TObjectPtr<UDataTable> AttributeInitDataTable = nullptr;

	/* 이 캐릭터를 식별하는 유닛 태그 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="HOG|Attributes")
	FGameplayTag UnitTag;

	/* 초기 Attribute 적용 완료 여부 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="HOG|Attributes")
	bool bAttributesInitialized = false;

protected:
	/* ASC / DT / UnitTag 준비가 되었을 때 초기 Attribute 적용 시도 */
	UFUNCTION(BlueprintCallable, Category="HOG|Attributes")
	void InitializeAttributesIfReady();

	/* DT에서 UnitTag에 맞는 Row를 찾아 AttributeSet에 실제 값 반영 */
	UFUNCTION(BlueprintCallable, Category="HOG|Attributes")
	bool ApplyInitialAttributesFromDataTable();

	/* Player는 PlayerState, Enemy는 자기 자신 등 ASC 위치 차이를 흡수 */
	UFUNCTION(BlueprintPure, Category="HOG|Attributes")
	UAbilitySystemComponent* GetCharacterAbilitySystemComponent() const;
};
