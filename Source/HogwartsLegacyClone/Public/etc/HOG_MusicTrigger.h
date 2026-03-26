// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HOG_MusicTrigger.generated.h"

class UBoxComponent;
class USoundBase;

UCLASS()
class HOGWARTSLEGACYCLONE_API AHOG_MusicTrigger : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AHOG_MusicTrigger();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnOverlapBegin(
		UPrimitiveComponent* OverlappedComponent, 
		AActor* OtherActor, 
		UPrimitiveComponent* OtherComp, 
		int32 OtherBodyIndex, 
		bool bFromSweep, 
		const FHitResult& SweepResult);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UBoxComponent> TriggerBox;

	// 이 값을 비워두면(None) 기존에 재생중인 배경음악이 페이드아웃 되며 완전히 꺼짐
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HOG|BGM")
	TObjectPtr<USoundBase> TargetBGM;

	// 새로 재생될 음악이 서서히 커지는 시간 (초)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HOG|BGM", meta=(ClampMin="0.0"))
	float FadeInTime = 2.0f;

	// 기존 음악이 서서히 작아지는 시간 (초)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HOG|BGM", meta=(ClampMin="0.0"))
	float FadeOutTime = 2.0f;
};
