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
