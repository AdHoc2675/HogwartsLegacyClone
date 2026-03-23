// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "MinimapData.h"
#include "Blueprint/UserWidget.h"
#include "MinimapWidget.generated.h"

class UMinimapIconOverlay;
class UCanvasPanel;
class UImage;
class UMinimapCaptureComponent;
/**
 * 
 */

UCLASS()
class HOGWARTSLEGACYCLONE_API UMinimapWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void InitializeMiniMap(UMinimapCaptureComponent* InCaptureComponent);
	void ShutdownMiniMap();

protected:
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> MinimapImage;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCanvasPanel> MinimapCanvas;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> PlayerIcon;

	UPROPERTY(EditDefaultsOnly, Category ="Minimap|Icons")
	TMap<FGameplayTag, FMinimapIconInfo> DefaultIconMap;
	
	UPROPERTY(EditDefaultsOnly, Category = "Minimap")
	TObjectPtr<UMaterialInterface> MinimapMaterial;
	
	UPROPERTY(EditDefaultsOnly, Category ="Minimap")
	float UIUpdateInterval = 0.1f;

private:
	void BindRenderTarget();
	
	TWeakObjectPtr<UMinimapCaptureComponent> CaptureComponent;
	
	UPROPERTY()
	TObjectPtr<UMinimapIconOverlay> MinimapIconOverlay;
	
	float TimeSinceLastUpdate = 0.f;
};
