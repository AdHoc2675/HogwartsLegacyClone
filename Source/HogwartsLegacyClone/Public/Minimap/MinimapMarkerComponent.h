// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "MinimapData.h"
#include "Components/ActorComponent.h"
#include "MinimapMarkerComponent.generated.h"

class USphereComponent;
struct FMinimapMarkerData;
DECLARE_MULTICAST_DELEGATE_OneParam(FOnMinimapPlayerArrived, AActor*)

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class HOGWARTSLEGACYCLONE_API UMinimapMarkerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UMinimapMarkerComponent();

	void RemoveMarker();

	FGuid GetMarkerId() const { return MarkerId; }

	// 마커 제거가 로직으로 필요한 경우
	FOnMinimapPlayerArrived OnPlayerArrived;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// 마커 태그 설정
	UPROPERTY(EditAnywhere, Category = "Minimap")
	FGameplayTag MarkerTag;
	
	UPROPERTY(EditAnywhere, Category = "Minimap")
	FMinimapIconInfo IconInfo;

	// 도착 감지 트리거가 필요시
	UPROPERTY(EditAnywhere, Category = "Minimap")
	float ArrivalRadius = 0.f;

	// 도착시 바로 마커 제가 필요한지 체크
	UPROPERTY(EditAnywhere, Category = "Minimap")
	bool bRemoveOnArrival = false;

private:
	UFUNCTION()
	void OnArrivalOverlapBegin(
		UPrimitiveComponent* OverlappedComp,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

	void RegisterToSubSystem();
	void UnRegisterFromSubSystem();
	FMinimapMarkerData BuildMakerData() const;
	void CreateArrivalTrigger();

	UPROPERTY()
	TObjectPtr<USphereComponent> ArrivalTrigger;

	FGuid MarkerId;
};
