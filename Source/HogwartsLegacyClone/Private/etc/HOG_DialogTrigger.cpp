#include "etc/HOG_DialogTrigger.h"
#include "Components/BoxComponent.h"
#include "Character/Player/PlayerCharacterBase.h" // 플레이어 확인용
#include "Kismet/GameplayStatics.h"
#include "HOGDebugHelper.h"

#include "GameFramework/HOG_PlayerController.h"
#include "UI/HOG_WidgetController.h"

AHOG_DialogTrigger::AHOG_DialogTrigger()
{
	PrimaryActorTick.bCanEverTick = false;

	// 구체 콜리전 생성 및 초기 세팅
	TriggerCollision2 = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerCollision"));
	RootComponent = TriggerCollision2;

	TriggerCollision2->SetCollisionProfileName(TEXT("Trigger")); // Overlap만 감지하도록 설정
}

void AHOG_DialogTrigger::BeginPlay()
{
	Super::BeginPlay();

	// 콜리전에 Overlap 이벤트 바인딩
	if (TriggerCollision2)
	{
		TriggerCollision2->OnComponentBeginOverlap.AddDynamic(this, &AHOG_DialogTrigger::OnOverlapBegin);
	}
}

void AHOG_DialogTrigger::OnOverlapBegin(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)

{
	//Debug::Print(TEXT("DialogTrigger: OnOverlapBegin called"), FColor::Green);
	
	// 1회성인데 이미 실행되었다면 무시
	if (bTriggerOnce && bHasTriggered)
	{
		return;
	}

	// 영역에 닿은 타겟이 '플레이어'인지 확실히 검사
	APlayerCharacterBase* PlayerCharacter = Cast<APlayerCharacterBase>(OtherActor);
	if (PlayerCharacter)
	{
		// 한 번 실행되었음을 체크
		bHasTriggered = true;

		// 1. 대사 음성 재생 (플레이어의 위치에서 재생)
		if (DialogSound)
		{
			UGameplayStatics::PlaySoundAtLocation(this, DialogSound, PlayerCharacter->GetActorLocation());
		}

		// 2. 대사 자막 출력
		if (!DialogText.IsEmpty())
		{
			if (AHOG_PlayerController* PC = Cast<AHOG_PlayerController>(PlayerCharacter->GetController()))
			{
				if (UHOG_WidgetController* UIController = PC->GetWidgetController())
				{
					// 추가한 Duration 변수를 통해 에디터에서 설정한 시간만큼 표시
					UIController->RequestSubtitle(DialogText, DialogDuration); 
				}
			}
		}
	}
}