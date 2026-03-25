#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HOG_DialogTrigger.generated.h"

class UBoxComponent;
class USoundBase;

UCLASS()
class HOGWARTSLEGACYCLONE_API AHOG_DialogTrigger : public AActor
{
	GENERATED_BODY()

public:
	AHOG_DialogTrigger();

protected:
	virtual void BeginPlay() override;

	// 플레이어가 영역에 닿았을 때 호출될 오버랩 함수
	UFUNCTION()
	void OnOverlapBegin(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

protected:
	// 트리거 영역 (에디터에서 크기 조절 가능)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UBoxComponent> TriggerCollision2;

	// 재생할 대사(음성) 파일
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HOG|Dialog")
	TObjectPtr<USoundBase> DialogSound;

	// 자막으로 띄울 텍스트 내용
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HOG|Dialog")
	FString DialogText;

	// 자막이 화면에 유지되는 시간 (초)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HOG|Dialog")
	float DialogDuration = 3.0f;

	// 한 번만 실행되게 할지 여부
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HOG|Dialog")
	bool bTriggerOnce = true;

private:
	// 이미 실행되었는지 확인하는 내부 플래그
	bool bHasTriggered = false;
};