// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "GameplayTagContainer.h"
#include "InputAction.h"
#include "HOG_PlayerController.generated.h"


class UDA_InputConfig;
class UInputMappingContext;
class UInputAction;
class UEnhancedInputComponent;

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

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

protected:
	/** BP에서 지정할 InputConfig (DA_InputConfig) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="HOG|Input")
	TObjectPtr<UDA_InputConfig> InputConfig;

	/** MappingContext 우선순위(기본 0) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="HOG|Input")
	int32 DefaultMappingPriority = 0;

	/** IMC 적용 여부(중복 적용 방지) */
	UPROPERTY(Transient)
	bool bAppliedMappingContext = false;

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
	
private:
	// Ability Action -> Tag 역조회 캐시
	UPROPERTY(Transient)
	TMap<TObjectPtr<const UInputAction>, FGameplayTag> AbilityActionToTag;

private:
	// EnhancedInput 공용 콜백
	void OnAbilityActionStarted(const FInputActionInstance& Instance);
	void OnAbilityActionCompleted(const FInputActionInstance& Instance);

	
};
