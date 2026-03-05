// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Character/BaseCharacter.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"

#include "EnemyCharacterBase.generated.h"

class UDA_EnemyConfig;
class UEnemyAttributeSet;
class AController;
class UBehaviorTree;
/**
 * 
 */
DECLARE_MULTICAST_DELEGATE(FOnEnemyDeath);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnEnemyDamaged, float);

UCLASS()
class HOGWARTSLEGACYCLONE_API AEnemyCharacterBase : public ABaseCharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()
	
public:
	AEnemyCharacterBase();
	
	// ===== Data Asset =====
	FORCEINLINE const UDA_EnemyConfig* GetEnemyData() const { return EnemyConfig; }
	
	// ===== IAbilitySystemInterface =====
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	
	// ===== Getters =====
	float GetHealth() const;
	float GetMaxHealth() const;
	float GetHealthPercent() const;
	
	// ===== BehaviorTree =====
	UBehaviorTree* GetBehaviorTree() const;
	
	// ===== 태그 헬퍼 (GAS용) =====
	bool HasGameplayTag(FGameplayTag Tag) const;
	void AddGameplayTag(FGameplayTag Tag);
	void RemoveGameplayTag(FGameplayTag Tag);
	
	// ===== 델리게이트 =====
	// 적이 죽는경우 호출
	FOnEnemyDeath OnEnemyDeath;
	// 적이 데미지를 받는 경우 호출
	FOnEnemyDamaged OnEnemyDamaged;
	
protected:
	virtual void PossessedBy(AController* NewController) override;
	
	// ===== 초기화 =====
	virtual void InitializeAbilitySystem();
	virtual void InitializeAttributes();
	virtual void GiveStartupAbilities();
	virtual void BindAttributeCallbacks();
	
	// ===== 데이터 에셋 =====
	UPROPERTY(EditAnywhere, Category = "Data")
	UDA_EnemyConfig* EnemyConfig;
	
	// ===== GAS =====
	UPROPERTY(VisibleAnywhere, Category = "GAS")
	UAbilitySystemComponent* AbilitySystemComponent;
	UPROPERTY(VisibleAnywhere, Category = "GAS")
	UEnemyAttributeSet* AttributeSet;
	
	virtual void HandleDeath_Implementation() override;
	virtual void OnHealthChanged(float OldValue, float NewValue);
	
private:
	void OnHealthChangedInternal(const FOnAttributeChangeData& Data);
	
	UPROPERTY(EditAnywhere, Category = "LifeSpan")
	float LifeSpanWhenDead = 3.f;
};
