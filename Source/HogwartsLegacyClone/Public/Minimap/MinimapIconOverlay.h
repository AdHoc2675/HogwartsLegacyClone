// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UObject/NoExportTypes.h"
#include "MinimapIconOverlay.generated.h"

struct FMinimapMarkerData;
class UImage;
class UTexture2D;
class UCanvasPanel;
class UMinimapSubsystem;
class UMinimapCaptureComponent;
/**
 * 
 */
UCLASS()
class HOGWARTSLEGACYCLONE_API UMinimapIconOverlay : public UObject
{
	GENERATED_BODY()

public:
	// 종속성 주입
	void Initialize(
		UCanvasPanel* InCanvas,
		UMinimapSubsystem* InSubsystem,
		UMinimapCaptureComponent* InCapture,
		const TMap<FGameplayTag, TSoftObjectPtr<UTexture2D>>& InIconMap,
		FVector2D InIconSize);

	void ShutDown();
	void UpdatePositions();

private:
	void CreateMarkerIcon(const FMinimapMarkerData& MarkerData);
	void RemoveMarkerIcon(const FGuid& MarkerID);
	void SetIconPosition(UImage* IconWidget, const FVector2D& NormalizedPosition);
	
	UTexture2D* ResolveIconTexture(const TSoftObjectPtr<UTexture2D>& MarkerIcon, const FGameplayTag& MarkerTag) const;
	void HandleMarkerAdded(const FMinimapMarkerData& MarkerData);
	void HandleMarkerRemoved(const FGuid& MarkerID);
	
	void BindDelegates();
	void UnbindDelegates();
	
	TWeakObjectPtr<UCanvasPanel> IconCanvas;
	TWeakObjectPtr<UMinimapSubsystem> Subsystem;
	TWeakObjectPtr<UMinimapCaptureComponent> CaptureComponent;
	
	UPROPERTY()
	TMap<FGuid, TObjectPtr<UImage>> IconWidgetMap;
	
	TMap<FGameplayTag, TSoftObjectPtr<UTexture2D>> DefaultIconMap;
	FVector2D IconSize = FVector2D(24.f, 24.f);
	
};
