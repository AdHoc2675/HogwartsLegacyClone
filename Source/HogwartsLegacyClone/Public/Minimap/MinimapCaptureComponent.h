// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"

#include "MinimapCaptureComponent.generated.h"


struct FMinimapScreenPosition;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class HOGWARTSLEGACYCLONE_API UMinimapCaptureComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UMinimapCaptureComponent();

	UTextureRenderTarget2D* GetRenderTarget() const;
	float GetCaptureRadius() const {return CaptureRadius;}
	
	// 월드좌표 -> 미니맵 좌표
	FMinimapScreenPosition WorldToScreenPosition(const FVector& WorldLocation) const;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void InitializeComponent() override;
	
	UPROPERTY(EditAnywhere, Category = "Capture")
	float CaptureHeight = 400.f;
	
	UPROPERTY(EditAnywhere, Category = "Capture")
	float CaptureRadius = 3000.f;
	
	UPROPERTY(EditAnywhere, Category = "Capture")
	int32 RenderTargetResolution = 512;
	
	UPROPERTY(EditAnywhere, Category = "Capture")
	float CaptureUpdateInterval = 0.1f;
	
	UPROPERTY(EditAnywhere, Category = "Capture")
	float RecaptureDistanceThreshold = 50.f;

private:
	void InitializeCaptureCamera();
	void CreateRenderTarget();
	bool TryUpdateCapture();
	
	UPROPERTY()
	TObjectPtr<USceneCaptureComponent2D> CaptureComponent;
	
	UPROPERTY()
	TObjectPtr<UTextureRenderTarget2D> RenderTarget;
	
	float TimeSinceLastCapture = 0.f;
	FVector LastCaptureLocation = FVector::ZeroVector;
};
