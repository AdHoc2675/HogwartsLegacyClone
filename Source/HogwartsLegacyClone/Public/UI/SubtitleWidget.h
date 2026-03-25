// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SubtitleWidget.generated.h"

class UTextBlock;

/**
 * 
 */
UCLASS()
class HOGWARTSLEGACYCLONE_API USubtitleWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	// 외부(혹은 WidgetController)에서 텍스트와 유지 시간을 전달받아 호출
	void ShowSubtitle(const FString& Text, float Duration = 2.0f);

protected:
	UPROPERTY(meta = (BindWidget))
	UTextBlock* SubtitleText;

private:
	void HideSubtitle();

	FTimerHandle SubtitleTimerHandle;
};
