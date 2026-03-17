#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayEffectTypes.h"
#include "GameplayTagContainer.h"
#include "Core/HOG_Struct.h"
#include "CombatComponent.generated.h"

class ABaseCharacter;
class UAbilitySystemComponent;
class UGameplayEffect;
class UHOGAttributeSet;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDamageAppliedSignature, const FDamageResult&, DamageResult);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnParrySuccessSignature, AActor*, AttackerActor);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDeathSignature);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class HOGWARTSLEGACYCLONE_API UCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCombatComponent();

protected:
	virtual void BeginPlay() override;

protected:
	UPROPERTY(Transient, BlueprintReadOnly, Category="Combat")
	TWeakObjectPtr<ABaseCharacter> OwnerCharacter;

	UPROPERTY(Transient, BlueprintReadOnly, Category="Combat")
	TWeakObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Combat|Protego")
	float ProtegoParryWindowEndTime = -1.0f;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Combat|Protego")
	float ProtegoBlockWindowEndTime = -1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Combat|Protego")
	bool bConsumeParryWindowOnSuccess = true;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Combat|State")
	bool bCanBeDamaged = true;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Combat|State")
	bool bIsDead = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Combat|State")
	float LastDamageTime = -1.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Combat|State")
	TWeakObjectPtr<AActor> LastHitInstigator;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Combat|State")
	TWeakObjectPtr<AActor> LastHitCauser;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Combat|Settings")
	bool bAllowFriendlyFire = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Combat|Settings")
	TSubclassOf<UGameplayEffect> DefaultDamageEffectClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Combat|Settings")
	bool bEnableDebug = true;

public:
	UPROPERTY(BlueprintAssignable, Category="Combat|Event")
	FOnDamageAppliedSignature OnDamageApplied;

	UPROPERTY(BlueprintAssignable, Category="Combat|Event")
	FOnDeathSignature OnDeath;
	
	UPROPERTY(BlueprintAssignable, Category="Combat|Event")
	FOnParrySuccessSignature OnParrySuccess;

public:
	UFUNCTION(BlueprintCallable, Category="Combat")
	void InitializeCombatComponent();

	UFUNCTION(BlueprintPure, Category="Combat")
	UAbilitySystemComponent* GetAbilitySystemComponent() const;

	UFUNCTION(BlueprintPure, Category="Combat")
	const UHOGAttributeSet* GetAttributeSet() const;

	UFUNCTION(BlueprintPure, Category="Combat")
	bool CanReceiveDamage() const;

	UFUNCTION(BlueprintPure, Category="Combat")
	bool IsDead() const;

	UFUNCTION(BlueprintCallable, Category="Combat")
	bool IsFriendlyTo(AActor* OtherActor) const;
	
	UFUNCTION(BlueprintCallable, Category="Combat")
	FDamageResult ApplyDamageRequest(const FDamageRequest& InRequest);

	

protected:
	bool ValidateDamageRequest(const FDamageRequest& InRequest) const;
	bool ShouldIgnoreDamage(const FDamageRequest& InRequest) const;

	FGameplayEffectContextHandle BuildEffectContext(
		const FDamageRequest& InRequest,
		UAbilitySystemComponent* EffectSourceASC
	) const;
	bool ApplyDamageEffect(const FDamageRequest& InRequest, FDamageResult& OutResult);

	void HandleDamageResult(const FDamageRequest& InRequest, FDamageResult& OutResult);
	void HandleDeath();

	void DebugPrint(const FString& Message) const;

	/* 현재 공식 기준으로 CombatComponent 쪽에서도 최종 피해량을 계산 */
	float CalculateExpectedFinalDamage(const FDamageRequest& InRequest) const;

public:
	UFUNCTION(BlueprintCallable, Category="Combat|Protego")
	void OpenProtegoDefenseWindow(float ParryDurationSeconds, float BlockDurationSeconds);

	UFUNCTION(BlueprintCallable, Category="Combat|Protego")
	void ClearProtegoDefenseWindow();

	UFUNCTION(BlueprintPure, Category="Combat|Protego")
	bool IsProtegoParryWindowActive() const;
	
	UFUNCTION(BlueprintPure, Category="Combat|Protego")
	bool IsProtegoBlockWindowActive() const;

protected:
	bool HasOwnerGameplayTag(const FGameplayTag& Tag) const;
	bool TryHandleProtegoDefense(const FDamageRequest& InRequest, FDamageResult& OutResult);
};
