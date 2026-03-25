// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Character/BaseCharacter.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"

#include "EnemyCharacterBase.generated.h"

UENUM(BlueprintType)
enum class EAIActivationMode : uint8
{
	Immediate,     
	WaitForSignal   
};

class UDA_EnemyConfigBase;
class UHOGAttributeSet;
class AController;
class UBehaviorTree;
class UDamageNumberPool;
class UEnemyDamageHandler;
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
	// 자식에서 오버라이드
	virtual UDA_EnemyConfigBase* GetEnemyConfig() const { return nullptr; }
	
	// ===== IAbilitySystemInterface =====
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	
	// ===== AI Activation Mode =====
	EAIActivationMode GetActivationMode() const { return ActivationMode; }
	
	// ===== Getters =====
	float GetHealth() const;
	float GetMaxHealth() const;
	float GetHealthPercent() const;
	float GetMinAttackRange() const;
	
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
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void PossessedBy(AController* NewController) override;
	
	// ===== 초기화 =====
	virtual void InitializeAbilitySystem();
	//virtual void InitializeAttributes();
	virtual void GiveStartupAbilities();
	virtual void BindAttributeCallbacks();
	
	//팀태그 ->ASC 동기화
	virtual void SyncTeamTagToAbilitySystem();
	
	// ===== GAS =====
	UPROPERTY(VisibleAnywhere, Category = "GAS")
	UAbilitySystemComponent* AbilitySystemComponent;
	
	UPROPERTY(VisibleAnywhere, Category = "GAS")
	UHOGAttributeSet* AttributeSet;
	
	virtual void HandleDeath_Implementation() override;
	virtual void OnHealthChanged(float OldValue, float NewValue);
	
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Activation")
	EAIActivationMode ActivationMode = EAIActivationMode::Immediate;
	
private:
	void OnHealthChangedInternal(const FOnAttributeChangeData& Data);
	
	UPROPERTY()
	TObjectPtr<UEnemyDamageHandler> DamageHandler;
	
};
