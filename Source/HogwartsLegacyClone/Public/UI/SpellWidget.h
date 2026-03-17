// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SpellWidget.generated.h"

/**
 * 
 */
class UProgressBar;
class UImage;
UCLASS()
class HOGWARTSLEGACYCLONE_API USpellWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void UseSpell(const float Time);

private:
	void UpdateSpellCoolDown();

	UPROPERTY(meta = (BindWidget))
	UProgressBar* SpellProgressBar;

	UPROPERTY(meta = (BindWidget))
	UImage* SpellIcon;

	FTimerHandle SpellTimerHandle;

	float StartTime;
	float CooldownTime;
	bool bUseSpell;
};
