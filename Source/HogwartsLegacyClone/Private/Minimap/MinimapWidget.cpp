// Fill out your copyright notice in the Description page of Project Settings.


#include "Minimap/MinimapWidget.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Components/Image.h"
#include "Minimap/MinimapCaptureComponent.h"
#include "Subsystem/MinimapSubsystem.h"
#include "Minimap/MinimapIconOverlay.h"

void UMinimapWidget::InitializeMiniMap(UMinimapCaptureComponent* InCaptureComponent)
{
	if (!InCaptureComponent) return;

	CaptureComponent = InCaptureComponent;
	BindRenderTarget();

	UWorld* World = GetWorld();
	if (!World) return;

	UMinimapSubsystem* Subsystem = World->GetSubsystem<UMinimapSubsystem>();
	if (!Subsystem) return;

	MinimapIconOverlay = NewObject<UMinimapIconOverlay>(this);
	
	MinimapIconOverlay->Initialize(
	MinimapCanvas,
	Subsystem,
	InCaptureComponent,
	DefaultIconMap);
}

void UMinimapWidget::ShutdownMiniMap()
{
	if (MinimapIconOverlay)
	{
		MinimapIconOverlay->ShutDown();
		MinimapIconOverlay = nullptr;
	}
	CaptureComponent.Reset();
}

void UMinimapWidget::NativeDestruct()
{
	ShutdownMiniMap();
	Super::NativeDestruct();
}

void UMinimapWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!CaptureComponent.IsValid()) return;
	
	AActor* Owner = CaptureComponent->GetOwner();
	if (Owner && PlayerIcon)
	{
		FVector Velocity = Owner->GetVelocity();
		if (Velocity.SizeSquared2D() > KINDA_SMALL_NUMBER)
		{
			float MoveYaw = Velocity.ToOrientationRotator().Yaw;
			PlayerIcon->SetRenderTransformAngle(MoveYaw);
		}
	}
	
	TimeSinceLastUpdate += InDeltaTime;
	if (TimeSinceLastUpdate < UIUpdateInterval) return;
	TimeSinceLastUpdate = 0.f;

	if (MinimapIconOverlay)
	{
		MinimapIconOverlay->UpdatePositions();
	}
}

void UMinimapWidget::BindRenderTarget()
{
	if (!MinimapImage || !CaptureComponent.IsValid() || !MinimapMaterial) return;

	UTextureRenderTarget2D* RenderTarget = CaptureComponent->GetRenderTarget();
	if (!RenderTarget) return;

	UMaterialInstanceDynamic* MatInst = UMaterialInstanceDynamic::Create(MinimapMaterial, this);
	if (!MatInst) return;

	MatInst->SetTextureParameterValue("MinimapTexture", RenderTarget);
	MinimapImage->SetBrushFromMaterial(MatInst);
	
}
