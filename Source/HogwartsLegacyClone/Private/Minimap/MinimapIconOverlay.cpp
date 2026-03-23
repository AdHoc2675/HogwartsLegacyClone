// Fill out your copyright notice in the Description page of Project Settings.


#include "Minimap/MinimapIconOverlay.h"
#include "Subsystem/MinimapSubsystem.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Minimap/MinimapCaptureComponent.h"
#include "Components/Image.h"

void UMinimapIconOverlay::Initialize(UCanvasPanel* InCanvas, UMinimapSubsystem* InSubsystem,
                                     UMinimapCaptureComponent* InCapture, const TMap<FGameplayTag, FMinimapIconInfo>& InMinimapIconInfo)
{
	IconCanvas = InCanvas;
	Subsystem = InSubsystem;
	CaptureComponent = InCapture;
	DefaultIconMap = InMinimapIconInfo;
	
	BindDelegates();
	
	if (Subsystem.IsValid())
	{
		TArray<FGuid> ExistingIDs;
		Subsystem->GetAllMarkersIDs(ExistingIDs);
		
		for (const FGuid& ID : ExistingIDs)
		{
			FMinimapMarkerData Data;
			if (Subsystem->TryGetMarkerData(ID, Data))
			{
				CreateMarkerIcon(Data);
			}
		}
	}
}

void UMinimapIconOverlay::ShutDown()
{
	UnbindDelegates();
	
	for (const auto& Pair : IconWidgetMap)
	{
		if (Pair.Value)
		{
			Pair.Value->RemoveFromParent();
		}
	}
	IconWidgetMap.Empty();
}

void UMinimapIconOverlay::UpdatePositions()
{
	if (!CaptureComponent.IsValid() || !Subsystem.IsValid()) return;
	
	TArray<FGuid> MarkersToRemove;
	
	for (const auto& Pair : IconWidgetMap)
	{
		if(!Pair.Value) continue;;
		
		FVector MarkerPosition;
		// 이미 삭제된 마커 제외
		if (!Subsystem->TryGetMarkerLocation(Pair.Key, MarkerPosition))
		{
			MarkersToRemove.Add(Pair.Key);
		}
		
		// 미니맵 좌표 반환
		FMinimapScreenPosition ScreenPosition = CaptureComponent->WorldToScreenPosition(MarkerPosition);
		
		// 범위 체크
		if (ScreenPosition.bIsInRange)
		{
			Pair.Value->SetVisibility(ESlateVisibility::Visible);
			SetIconPosition(Pair.Value, ScreenPosition.ScreenPosition);
		}
		else
		{
			Pair.Value->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
	for (const FGuid& ID : MarkersToRemove)
	{
		RemoveMarkerIcon(ID);
	}
}

void UMinimapIconOverlay::CreateMarkerIcon(const FMinimapMarkerData& MarkerData)
{
	if (!IconCanvas.IsValid()) return;
	if (IconWidgetMap.Contains(MarkerData.MarkerID)) return;
	
	UImage* IconWidget = NewObject<UImage>(IconCanvas.Get());
	if (!IconWidget) return;
	
	FMinimapIconInfo Resolved = ResolveIconInfo(MarkerData.IconInfo, MarkerData.MarkerTag);

	if (!Resolved.IconTexture.IsNull())
	{
		if (UTexture2D* Tex = Resolved.IconTexture.LoadSynchronous())
		{
			IconWidget->SetBrushFromTexture(Tex);
		}
	}

	IconCanvas->AddChild(IconWidget);

	if (UCanvasPanelSlot* Slot = Cast<UCanvasPanelSlot>(IconWidget->Slot))
	{
		Slot->SetSize(Resolved.IconSize);
		Slot->SetAlignment(FVector2D(0.5f, 0.5f));
	}

	IconWidgetMap.Add(MarkerData.MarkerID, IconWidget);
}

void UMinimapIconOverlay::RemoveMarkerIcon(const FGuid& MarkerID)
{
	TObjectPtr<UImage> Found = IconWidgetMap.FindRef(MarkerID);
	if (!Found) return;
	
	Found->RemoveFromParent();
	IconWidgetMap.Remove(MarkerID);
}

void UMinimapIconOverlay::SetIconPosition(UImage* IconWidget, const FVector2D& NormalizedPosition)
{
	if (!IconWidget || !IconCanvas.IsValid()) return;

	UCanvasPanelSlot* Slot = Cast<UCanvasPanelSlot>(IconWidget->Slot);
	if (!Slot) return;
	
	FVector2D CanvasSize = FVector2D::ZeroVector;
    
	FGeometry Geometry = IconCanvas->GetCachedGeometry();
	CanvasSize = FVector2D(Geometry.GetLocalSize());
	
	if (CanvasSize.X <= 0 || CanvasSize.Y <= 0) return;

	const FVector2D Position = FVector2D(
		NormalizedPosition.X * CanvasSize.X,
		NormalizedPosition.Y * CanvasSize.Y);

	Slot->SetPosition(Position);
}

FMinimapIconInfo  UMinimapIconOverlay::ResolveIconInfo(const FMinimapIconInfo& MarkerIconInfo,
	const FGameplayTag& MarkerTag) const
{
	FMinimapIconInfo Result = MarkerIconInfo;
	
	bool bNeedTexture = Result.IconTexture.IsNull();
	bool bNeedSize = Result.IconSize.IsZero();
	
	if (bNeedTexture || bNeedSize)
	{
		for (const auto& Pair : DefaultIconMap)
		{
			if (!MarkerTag.MatchesTag(Pair.Key)) continue;
			
			if (bNeedTexture && !Pair.Value.IconTexture.IsNull())
			{
				Result.IconTexture = Pair.Value.IconTexture;
				bNeedTexture = false;
			}
			
			if (bNeedSize && !Pair.Value.IconSize.IsZero())
			{
				Result.IconSize = Pair.Value.IconSize;
				bNeedSize = false;
			}
			
			if (!bNeedSize && !bNeedTexture) break;
		}
	}
	if (Result.IconSize.IsZero())
	{
		Result.IconSize = FVector2D(30.f ,30.f);
	}
	return Result;
}

void UMinimapIconOverlay::HandleMarkerAdded(const FMinimapMarkerData& MarkerData)
{
	CreateMarkerIcon(MarkerData);
}

void UMinimapIconOverlay::HandleMarkerRemoved(const FGuid& MarkerID)
{
	RemoveMarkerIcon(MarkerID);
}

void UMinimapIconOverlay::BindDelegates()
{
	if (!Subsystem.IsValid()) return;
	
	Subsystem->OnMarkerAdded.AddUObject(this, &UMinimapIconOverlay::HandleMarkerAdded);
	Subsystem->OnMarkerRemoved.AddUObject(this, &UMinimapIconOverlay::HandleMarkerRemoved);
}

void UMinimapIconOverlay::UnbindDelegates()
{
	if (!Subsystem.IsValid()) return;
	
	Subsystem->OnMarkerAdded.RemoveAll(this);	
	Subsystem->OnMarkerRemoved.RemoveAll(this);
}
