// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "MinimapData.h"

#include "MinimapIconOverlay.generated.h"

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
		const TMap<FGameplayTag, FMinimapIconInfo>& InMinimapIconInfo);

	void ShutDown();
	void UpdatePositions();

private:
	void CreateMarkerIcon(const FMinimapMarkerData& MarkerData);
	void RemoveMarkerIcon(const FGuid& MarkerID);
	void SetIconPosition(UImage* IconWidget, const FVector2D& NormalizedPosition);
	
	FMinimapIconInfo ResolveIconInfo(const FMinimapIconInfo& MarkerIconInfo, const FGameplayTag& MarkerTag) const;
	void HandleMarkerAdded(const FMinimapMarkerData& MarkerData);
	void HandleMarkerRemoved(const FGuid& MarkerID);
	
	void BindDelegates();
	void UnbindDelegates();
	
	TWeakObjectPtr<UCanvasPanel> IconCanvas;
	TWeakObjectPtr<UMinimapSubsystem> Subsystem;
	TWeakObjectPtr<UMinimapCaptureComponent> CaptureComponent;
	
	UPROPERTY()
	TMap<FGuid, TObjectPtr<UImage>> IconWidgetMap;
	
	TMap<FGameplayTag, FMinimapIconInfo> DefaultIconMap;
};
