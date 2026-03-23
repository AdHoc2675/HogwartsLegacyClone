// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Subsystems/WorldSubsystem.h"
#include "Minimap/MinimapData.h"

#include "MinimapSubsystem.generated.h"

/**
 * 
 */

DECLARE_MULTICAST_DELEGATE_OneParam(FOnMinimapMarkerAdded, const FMinimapMarkerData&);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnMinimapMarkerRemoved, const FGuid&);

UCLASS()
class HOGWARTSLEGACYCLONE_API UMinimapSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	
	// 등록 및 제거
	FMinimapMarkerResult RegisterMarker(const FMinimapMarkerData& InData);
	FMinimapMarkerResult UnregisterMarker(const FGuid& MarkerID);
	void ClearAllMarkers();
	
	// 조회
	void GetAllMarkersIDs(TArray<FGuid>& OutIDs);
	TArray<FGuid> GetMarkerIDsByTag(const FGameplayTag& InTag) const;
	bool ContainsMarker(const FGuid& MarkerID) const;
	int32 GetMarkerCount() const;
	
	// 반환
	bool TryGetMarkerLocation(const FGuid& MarkerID, FVector& OutLocation) const;
	bool TryGetMarkerTag(const FGuid& MarkerID, FGameplayTag& OutTag) const;
	bool TryGetMarkerData(const FGuid& MarkerID, FMinimapMarkerData& OutData) const;
	
	// 델리게이트
	FOnMinimapMarkerAdded OnMarkerAdded;
	FOnMinimapMarkerRemoved OnMarkerRemoved;
	
	private:
	bool ValidateMarkerData(const FMinimapMarkerData& InData, FString& OutFailReason);
	FVector ResolveMarkerLocation(const FMinimapMarkerData& Data) const;
	
	TMap<FGuid, FMinimapMarkerData> MarkerMap;
	
	
};
