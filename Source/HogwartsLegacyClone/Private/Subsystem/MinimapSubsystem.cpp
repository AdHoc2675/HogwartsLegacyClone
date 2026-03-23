// Fill out your copyright notice in the Description page of Project Settings.


#include "Subsystem/MinimapSubsystem.h"

#include "Minimap/MinimapData.h"

void UMinimapSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	MarkerMap.Empty();
}

void UMinimapSubsystem::Deinitialize()
{
	ClearAllMarkers();
	Super::Deinitialize();
}

FMinimapMarkerResult UMinimapSubsystem::RegisterMarker(const FMinimapMarkerData& InData)
{
	FMinimapMarkerResult Result;
	Result.MarkerID = InData.MarkerID;

	FString FailReason;

	if (!ValidateMarkerData(InData, FailReason))
	{
		Result.bSuccess = false;
		Result.FailReason = FailReason;
		return Result;
	}

	// 이미 있으면 리턴
	if (MarkerMap.Contains(Result.MarkerID))
	{
		Result.bSuccess = false;
		Result.FailReason = TEXT("마커 아이디 등록 안됨");
	}

	MarkerMap.Add(InData.MarkerID, InData);
	OnMarkerAdded.Broadcast(InData);

	Result.bSuccess = true;
	return Result;
}

FMinimapMarkerResult UMinimapSubsystem::UnregisterMarker(const FGuid& MarkerID)
{
	FMinimapMarkerResult Result;
	Result.MarkerID = MarkerID;

	if (!MarkerID.IsValid())
	{
		Result.bSuccess = false;
		Result.FailReason = TEXT("유효한 마커 아이디 없음");
		return Result;
	}
	if (!MarkerMap.Contains(MarkerID))
	{
		Result.bSuccess = false;
		Result.FailReason = TEXT("저장된 마커 아이디 없음");
		return Result;
	}

	MarkerMap.Remove(MarkerID);
	OnMarkerRemoved.Broadcast(MarkerID);

	Result.bSuccess = true;
	return Result;
}

void UMinimapSubsystem::ClearAllMarkers()
{
	TArray<FGuid> MarkerIDs;
	MarkerMap.GetKeys(MarkerIDs);

	for (const FGuid& MarkerID : MarkerIDs)
	{
		OnMarkerRemoved.Broadcast(MarkerID);
	}
	MarkerMap.Empty();
}

void UMinimapSubsystem::GetAllMarkersIDs(TArray<FGuid>& OutIDs)
{
	MarkerMap.GetKeys(OutIDs);
}

TArray<FGuid> UMinimapSubsystem::GetMarkerIDsByTag(const FGameplayTag& InTag) const
{
	TArray<FGuid> Results;
	if (!InTag.IsValid()) return Results;

	for (const TPair<FGuid, FMinimapMarkerData>&
		Pair : MarkerMap)
	{
		if (Pair.Value.MarkerTag.MatchesTag(InTag))
		{
			Results.Add(Pair.Key);
		}
	}
	return Results;
}

bool UMinimapSubsystem::ContainsMarker(const FGuid& MarkerID) const
{
	return MarkerMap.Contains(MarkerID);
}

int32 UMinimapSubsystem::GetMarkerCount() const
{
	return MarkerMap.Num();
}

bool UMinimapSubsystem::TryGetMarkerLocation(const FGuid& MarkerID, FVector& OutLocation) const
{
	const FMinimapMarkerData& Found = MarkerMap.FindRef(MarkerID);

	if (!Found.IsValid())
	{
		OutLocation = FVector::ZeroVector;
		return false;
	}
	OutLocation = ResolveMarkerLocation(Found);
	return true;
}

bool UMinimapSubsystem::TryGetMarkerTag(const FGuid& MarkerID, FGameplayTag& OutTag) const
{
	const FMinimapMarkerData& Found = MarkerMap.FindRef(MarkerID);
	if (!Found.IsValid())
	{
		OutTag = FGameplayTag();
		return false;
	}
	OutTag = Found.MarkerTag;
	return true;
}


bool UMinimapSubsystem::TryGetMarkerData(const FGuid& MarkerID, FMinimapMarkerData& OutData) const
{
	const FMinimapMarkerData Found = MarkerMap.FindRef(MarkerID);

	if (!Found.IsValid()) return false;

	OutData = Found;
	return true;
}

bool UMinimapSubsystem::ValidateMarkerData(const FMinimapMarkerData& InData, FString& OutFailReason)
{
	if (!InData.MarkerID.IsValid())
	{
		OutFailReason = TEXT("유효하지 않은 마커 ID");
		return false;
	}

	if (!InData.MarkerTag.IsValid())
	{
		OutFailReason = TEXT("마커 태그가 없음");
		return false;
	}

	return true;
}

FVector UMinimapSubsystem::ResolveMarkerLocation(const FMinimapMarkerData& Data) const
{
	if (Data.TrackedActor.IsValid())
	{
		return Data.TrackedActor->GetActorLocation();
	}
	return Data.WorldLocation;
}
