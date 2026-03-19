// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "HpWidget.generated.h"

/**
 * 
 */
class UProgressBar;

UCLASS()
class HOGWARTSLEGACYCLONE_API UHpWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	virtual void NativeDestruct() override;
	
	void SetHP(float NewHp, float MaxHp);
	void SetHP(float HpRatio);
	
protected:
	UPROPERTY(meta= (BindWidget))
	UProgressBar* HPBar;
	
private:
	void UpdateBar();
	 
	UPROPERTY(EditDefaultsOnly, category = "InterpSpeed")
	float InterpSpeed = 5.f;
	
	FTimerHandle TimerHandle;
	float TargetPercentage = 1.f;
	float CurrentPercentage= 1.f;
	float TimerRate = 0.016f;
};
