#include "etc/HOG_DialogTrigger.h"
#include "Components/SphereComponent.h"
#include "Character/Player/PlayerCharacterBase.h" // 플레이어 확인용
#include "Kismet/GameplayStatics.h"
#include "HOGDebugHelper.h"

AHOG_DialogTrigger::AHOG_DialogTrigger()
{
	PrimaryActorTick.bCanEverTick = false;

	// 구체 콜리전 생성 및 초기 세팅
	TriggerCollision = CreateDefaultSubobject<USphereComponent>(TEXT("TriggerCollision"));
	RootComponent = TriggerCollision;

	// 기본반경 300 (언리얼 단위 = 3미터)
	TriggerCollision->SetSphereRadius(300.f);
	TriggerCollision->SetCollisionProfileName(TEXT("Trigger")); // Overlap만 감지하도록 설정
}

void AHOG_DialogTrigger::BeginPlay()
{
	Super::BeginPlay();

	// 콜리전에 Overlap 이벤트 바인딩
	if (TriggerCollision)
	{
		TriggerCollision->OnComponentBeginOverlap.AddDynamic(this, &AHOG_DialogTrigger::OnOverlapBegin);
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

		// 2. 대사 자막 출력 (현재는 디버그 텍스트로 대체)
		if (!DialogText.IsEmpty())
		{
			// TODO: 나중에 화면 하단 UI 위젯을 호출하는 코드로 변경
			Debug::Print(FString::Printf(TEXT("[플레이어] : %s"), *DialogText), FColor::Cyan);
		}
	}
}