// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GameplayTagContainer.h"
#include "GA_Base.generated.h"

class UHOGAbilitySystemComponent;
class APlayerController;
class APawn;
class ACharacter;
class APlayerState;

/**
 * 공용 GameplayAbility 베이스
 * - HOG ASC 캐스팅 헬퍼
 * - Avatar/Controller/PlayerState 접근 헬퍼
 * - (선택) InputTag 저장 슬롯
 *
 * 파생 예정:
 * - UGA_SpellBase
 * - UGA_EnemyBase
 */

UCLASS(Abstract)
class HOGWARTSLEGACYCLONE_API UGA_Base : public UGameplayAbility
{
	GENERATED_BODY()
	
public:
	UGA_Base();
	
	//이 어빌리티에 접근하는 입력 태그
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="HOG|Ability")
	FGameplayTag InputTag;

	//어빌리티 활성, 종료시 확인용으로 디버그 넣어둠
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="HOG|Ability|Debug")
	bool bLogOnActivate = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="HOG|Ability|Debug")
	bool bLogOnEnd = false;
	
	
public:
	//ActorInfoHelper
		
	UFUNCTION(BlueprintCallable, Category="HOG|Ability|ActorInfo")
	UHOGAbilitySystemComponent* GetHOGASC() const;

	UFUNCTION(BlueprintCallable, Category="HOG|Ability|ActorInfo")
	APlayerController* GetPlayerController() const;

	UFUNCTION(BlueprintCallable, Category="HOG|Ability|ActorInfo")
	APawn* GetPawn() const;

	UFUNCTION(BlueprintCallable, Category="HOG|Ability|ActorInfo")
	ACharacter* GetCharacter() const;

	UFUNCTION(BlueprintCallable, Category="HOG|Ability|ActorInfo")
	APlayerState* GetPlayerState() const;
	
protected:
	//Override 들
	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo, 
		const FGameplayAbilityActivationInfo ActivationInfo, 
		const FGameplayEventData* TriggerEventData) override;
	
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, 
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility, bool bWasCancelled) override;
	
protected:
	void LogAbility(const TCHAR* Prefix, bool bWasCancelled = false) const;
	
	
};
