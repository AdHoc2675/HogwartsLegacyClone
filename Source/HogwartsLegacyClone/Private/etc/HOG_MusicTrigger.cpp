// Fill out your copyright notice in the Description page of Project Settings.


#include "etc/HOG_MusicTrigger.h"
#include "Components/BoxComponent.h"
#include "Character/Player/PlayerCharacterBase.h"
#include "GameFramework/HOG_PlayerController.h"

// Sets default values
AHOG_MusicTrigger::AHOG_MusicTrigger()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	TriggerBox = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerBox"));
	RootComponent = TriggerBox;

	// 플레이어만 감지하도록 셋업 (기존 트리거와 동일)
	TriggerBox->SetCollisionProfileName(TEXT("Trigger"));
}

// Called when the game starts or when spawned
void AHOG_MusicTrigger::BeginPlay()
{
	Super::BeginPlay();

	if (TriggerBox)
	{
		TriggerBox->OnComponentBeginOverlap.AddDynamic(this, &AHOG_MusicTrigger::OnOverlapBegin);
	}
}

void AHOG_MusicTrigger::OnOverlapBegin(
	UPrimitiveComponent* OverlappedComponent, 
	AActor* OtherActor, 
	UPrimitiveComponent* OtherComp, 
	int32 OtherBodyIndex, 
	bool bFromSweep, 
	const FHitResult& SweepResult)
{
	// 진입한 대상이 플레이어인지 확실히 검사
	APlayerCharacterBase* PlayerCharacter = Cast<APlayerCharacterBase>(OtherActor);
	if (!PlayerCharacter) return;

	// 커스텀 된 PlayerController를 가동
	AHOG_PlayerController* PC = Cast<AHOG_PlayerController>(PlayerCharacter->GetController());
	if (!PC) return;

	// TargetBGM 이 설정되어 있으면 새로운 곡으로 교체 (Fade Out -> Fade In)
	if (TargetBGM != nullptr)
	{
		PC->PlayBGMWithFade(TargetBGM, FadeInTime, FadeOutTime);
	}
	// TargetBGM 을 비워두었으면 기존 곡을 종료 (Fade Out)
	else
	{
		PC->StopBGMWithFade(FadeOutTime);
	}
}

