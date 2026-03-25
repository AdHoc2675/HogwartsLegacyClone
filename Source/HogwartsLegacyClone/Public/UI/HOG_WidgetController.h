// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "AbilitySystemComponent.h"

#include "HOG_WidgetController.generated.h"

class ULockOnComponent;
class UHOGEnemyHUDBase;
class UHOGPlayerHUDBase;
class APlayerCharacterBase;
class AHOG_PlayerController;
class USpellComponent;
class AHOG_PlayerState;
class UMinimapWidget;
class USubtitleWidget;
struct FLockOnTargetResult;

/**
 * 
 */
UCLASS()
class HOGWARTSLEGACYCLONE_API UHOG_WidgetController : public UObject
{
	GENERATED_BODY()

public:
	void Init(AHOG_PlayerController* InPlayerController, APlayerCharacterBase* InPlayerCharacter, TSubclassOf<UHOGPlayerHUDBase> InPlayerHUDClass, TSubclassOf<UHOGEnemyHUDBase> InEnemyHUDClass, TSubclassOf<UMinimapWidget> InMinimapClass);
	void UnlockSpellSlot(FGameplayTag SpellID);
	void Shutdown();
	
private:
	void CreatePlayerWidget(AHOG_PlayerController* InPlayerController);
	
	// ===== Player =====
	AHOG_PlayerState* GetPlayerState(APlayerCharacterBase* PlayerCharacter) const;
	
	void BindSpellComponent(AHOG_PlayerState* PlayerCharacter);
	void UnBindSpellComponent();
	
	void BindPlayerHP(AHOG_PlayerState* PlayerCharacter);
	void UnbindPlayerHP();
	void OnPlayerHpChanged(const FOnAttributeChangeData& Data);
	
	UFUNCTION()
	void OnSpellCooldownStarted(FGameplayTag SpellID, float CooldownSeconds);
	UFUNCTION()
	void OnSpellCooldownEnded(FGameplayTag SpellID);
	
	UPROPERTY()
	UHOGPlayerHUDBase* PlayerHUD;
	
	UPROPERTY()
	USpellComponent* SpellComponent;
	
	UPROPERTY()
	TSubclassOf<UHOGPlayerHUDBase> PlayerHUDClass;
	
	UPROPERTY()
	UAbilitySystemComponent* PlayerASC;
	
	FDelegateHandle PlayerHPDelegateHandle;
	
	// ===== Enemy =====
	void CreateEnemyWidget(AHOG_PlayerController* InPlayerController);
	
	void BindLockOnComponent(APlayerCharacterBase* PlayerCharacter);
	void UnBindLockOnComponent();
	void HandleLockOn(const FLockOnTargetResult& TargetResult);
	void HandleLockOnReleased(const FLockOnTargetResult& Target);
	void BindEnemyASC(UAbilitySystemComponent* TargetASC);
	void ClearEnemyBinding();
	void OnEnemyHpChanged(const FOnAttributeChangeData& Data);
	
	UPROPERTY()
	UHOGEnemyHUDBase* EnemyHUD;
	 
	UPROPERTY()
	UAbilitySystemComponent* BoundEnemyASC;
	
	UPROPERTY()
	ULockOnComponent* LockOnComponent;
	
	UPROPERTY()
	TSubclassOf<UHOGEnemyHUDBase> EnemyHUDClass;
	
	FDelegateHandle EnemyHPDelegateHandle;
	
	// ===== MiniMap =====
	void CreateMiniMapWidget( AHOG_PlayerController* InPlayerController, 
	APlayerCharacterBase* InPlayerCharacter);
	
	void ShutdownMiniMapWidget();
	
	UPROPERTY()
	TSubclassOf<UMinimapWidget> MinimapWidgetClass;

	UPROPERTY()
	TObjectPtr<UMinimapWidget> MinimapWidget;
	
public:
	// ===== Subtitle =====
	
	// 자막을 띄워 달라고 요청받을 외부 인터페이스
	void RequestSubtitle(const FString& Text, float Duration = 2.0f);

	// 자막 위젯용 Tsubclass
	UPROPERTY()
	TSubclassOf<USubtitleWidget> SubtitleWidgetClass;

	void CreateSubtitleWidget(AHOG_PlayerController* InPlayerController);

	UPROPERTY()
	TObjectPtr<USubtitleWidget> SubtitleWidget;
};
