// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "GameplayTagContainer.h"
#include "InputAction.h"
#include "HOG_PlayerController.generated.h"


class UMinimapWidget;
class UDA_InputConfig;
class UInputMappingContext;
class UInputAction;
class UEnhancedInputComponent;
class UHOG_WidgetController;
class UHOGPlayerHUDBase;
class UHOGEnemyHUDBase;

class UUserWidget;
class UDamageNumberPool;

class UAudioComponent;
class USoundBase;

/**
 * DataAsset 기반 PlayerController
 * - UDA_InputConfig를 읽어 EnhancedInput 바인딩/IMC 추가
 * - 입력 발생 시 APlayerCharacterBase의 Input_* 호출
 */


UCLASS()

class HOGWARTSLEGACYCLONE_API AHOG_PlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AHOG_PlayerController();

	// 상호작용이나 보상으로 마법을 해금할 때 UI를 업데이트
	UFUNCTION(BlueprintCallable, Category = "HOG|UI")
	void UnlockSpellUI(FGameplayTag SpellID);

	// WidgetController Getter
	UFUNCTION(BlueprintCallable, Category = "HOG|UI")
	class UHOG_WidgetController* GetWidgetController() const { return WidgetController; }

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<class USubtitleWidget> SubtitleWidgetClass;
protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void SetupInputComponent() override;
	virtual void PlayerTick(float DeltaTime) override;

protected:
	/** BP에서 지정할 InputConfig (DA_InputConfig) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="HOG|Input")
	TObjectPtr<UDA_InputConfig> InputConfig;

	/** 시작 시 기본으로 해금되어 있을 마법 목록 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HOG|Spell")
	TArray<FGameplayTag> InitialUnlockedSpells;

	/** MappingContext 우선순위(기본 0) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="HOG|Input")
	int32 DefaultMappingPriority = 0;

	/** IMC 적용 여부(중복 적용 방지) */
	UPROPERTY(Transient)
	bool bAppliedMappingContext = false;

	/** 위젯 컨트롤러 */
	UPROPERTY()
	UHOG_WidgetController* WidgetController;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UHOGPlayerHUDBase> PlayerHUDClass;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UHOGEnemyHUDBase> EnemyHUDClass;

protected:
	/** LocalPlayer Subsystem에 DefaultMappingContext 추가 */
	void ApplyDefaultMappingContext();

	/** 캐릭터(플레이어 폰) 얻기 */
	class APlayerCharacterBase* GetPlayerCharacterBase() const;

	/** 기본 액션 바인딩 */
	void BindDefaultActions();

	/** Ability(Tag->Action) 바인딩 */
	void BindAbilityActions();

	/** Ability Press/Release 라우팅 (EnhancedInput에서 Tag를 전달할 수 없어서 함수 분리) */
	void HandleAbilityPressed(FGameplayTag InputTag);
	void HandleAbilityReleased(FGameplayTag InputTag);

private:
	/** Move */
	void OnMoveTriggered(const struct FInputActionValue& Value);

	/** Look */
	void OnLookTriggered(const struct FInputActionValue& Value);

	/** Jump */
	void OnJumpStarted();
	void OnJumpCompleted();

	/** Interact */
	void OnInteractStarted();

private:
	// Ability Action -> Tag 역조회 캐시
	UPROPERTY(Transient)
	TMap<TObjectPtr<const UInputAction>, FGameplayTag> AbilityActionToTag;

private:
	// EnhancedInput 공용 콜백
	void OnAbilityActionStarted(const FInputActionInstance& Instance);
	void OnAbilityActionCompleted(const FInputActionInstance& Instance);

	// FloatDamage
public:
	UDamageNumberPool* GetDamageNumberPool() const { return DamageNumberPool; }

protected:
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UMinimapWidget> MinimapWidgetClass;
	
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UUserWidget> DamageNumberWidgetClass;

	UPROPERTY()
	UDamageNumberPool* DamageNumberPool;

public:
	// =============== [BGM System] ===============
	/** 새로운 배경음악을 페이드인 하며 재생. 기존 음악은 페이드아웃 */
	UFUNCTION(BlueprintCallable, Category = "HOG|BGM")
	void PlayBGMWithFade(USoundBase* NewBGM, float FadeInTime = 1.0f, float FadeOutTime = 1.0f);
		
	/** 현재 재생중인 배경음악을 페이드아웃 하며 정지 */
	UFUNCTION(BlueprintCallable, Category = "HOG|BGM")
	void StopBGMWithFade(float FadeOutTime = 1.0f);

protected:
	/** 현재 재생중인 BGM 컴포넌트 추적용 */
	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> CurrentBGMComponent;
};
