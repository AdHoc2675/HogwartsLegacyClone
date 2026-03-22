#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "MinimapData.generated.h"

// 마커 데이터
USTRUCT(BlueprintType)
struct FMinimapMarkerData
{
	GENERATED_BODY()
	
	// 고유 아이디
	UPROPERTY(BlueprintReadOnly)
	FGuid MarkerID;
	
	// 마커 종류 태그
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FGameplayTag MarkerTag;
	
	UPROPERTY(BlueprintReadOnly)
	FVector WorldLocation = FVector::ZeroVector;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSoftObjectPtr<UTexture2D> IconTexture;
	
	// 마커 대상
	TWeakObjectPtr<AActor> TrackedActor;
	
	bool IsValid() const
	{
		return MarkerID.IsValid() && MarkerTag.IsValid();
	}
};

// 마커 데이터 결과
USTRUCT(BlueprintType)
struct FMinimapMarkerResult
{
	GENERATED_BODY()
	
	UPROPERTY(BlueprintReadOnly)
	bool bSuccess = false;
	
	UPROPERTY(BlueprintReadOnly)
	FGuid MarkerID;
	
	UPROPERTY(BlueprintReadOnly)
	FString FailReason;
};

USTRUCT(BlueprintType)
struct FMinimapScreenPosition
{
	GENERATED_BODY()
	
	// 정규화 좌표
	UPROPERTY(BlueprintReadOnly)
	FVector2D ScreenPosition = FVector2D::ZeroVector;
	
	// 미니맵 범위 체크
	UPROPERTY(BlueprintReadOnly)
	bool bIsInRange = false;
};
