// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SpellWidget.generated.h"

/**
 * 
 */
struct FGameplayTag;
class UProgressBar;
class UImage;
class UOverlay;

UCLASS()
class HOGWARTSLEGACYCLONE_API USpellWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void UseSpell(FGameplayTag SpellID, float InCooldownTime);
	void UnLockSpell();

private:
	// === 애니메이션 ===
	void UpdateUnlockAnimation();

	FTimerHandle UnlockAnimTimerHandle;
	float UnlockAnimStartTime;
	const float UnlockDuration = 0.75f; // 애니메이션 지속 시간 (N초)
	const float MaxScale = 2.0f;       // 최대 스케일 (N배 커짐)

	// ===
	void UpdateSpellCoolDown();

	UPROPERTY(meta = (BindWidget))
	UProgressBar* SpellProgressBar;

	UPROPERTY(meta = (BindWidget))
	UImage* SpellIcon;
	
	UPROPERTY(meta = (BindWidget))
	UOverlay* SpellSlotLockOverlay;
	
	FTimerHandle SpellTimerHandle;

	float StartTime;
	float CooldownTime;
	bool bUseSpell;
};
