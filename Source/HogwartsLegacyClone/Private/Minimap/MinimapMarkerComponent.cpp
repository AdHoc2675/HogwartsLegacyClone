// Fill out your copyright notice in the Description page of Project Settings.


#include "Minimap/MinimapMarkerComponent.h"

#include "Character/Player/PlayerCharacterBase.h"
#include "Components/SphereComponent.h"
#include "Minimap/MinimapData.h"
#include "Subsystem/MinimapSubsystem.h"

// Sets default values for this component's properties
UMinimapMarkerComponent::UMinimapMarkerComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}


void UMinimapMarkerComponent::BeginPlay()
{
	Super::BeginPlay();

	// 고유 아이디 생성
	MarkerId = FGuid::NewGuid();
	
	RegisterToSubSystem();

	// 트리거가 필요한 경우, 도착 후 스펠을 배우고 나서 마커가 사라지게 해야하는 경우 등
	if (ArrivalRadius > 0.f)
	{
		CreateArrivalTrigger();
	}
}

void UMinimapMarkerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnRegisterFromSubSystem();

	if (ArrivalTrigger)
	{
		ArrivalTrigger->DestroyComponent();
		ArrivalTrigger = nullptr;
	}

	Super::EndPlay(EndPlayReason);
}

void UMinimapMarkerComponent::RemoveMarker()
{
	UnRegisterFromSubSystem();
}

void UMinimapMarkerComponent::OnArrivalOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
                                                    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
                                                    bool bFromSweep, const FHitResult& SweepResult)
{
	if (!OtherActor) return;
	
	APlayerCharacterBase* PlayerCharacter = Cast<APlayerCharacterBase>(OtherActor);
	if (!PlayerCharacter) return;
	
	// 플레이거 도착했음을 알림
	// 마커 제거를 이벤트 후 제거하는 경우 등..
	OnPlayerArrived.Broadcast(GetOwner());
	
	// 바로 마커를 제가하는 경우
	if (bRemoveOnArrival)
	{
		RemoveMarker();
	}
}

void UMinimapMarkerComponent::RegisterToSubSystem()
{
	UWorld* World = GetWorld();
	if (!World) return;

	UMinimapSubsystem* Subsystem = World->GetSubsystem<UMinimapSubsystem>();
	if (!Subsystem) return;

	FMinimapMarkerData Data = BuildMakerData();
	FMinimapMarkerResult Result;
	Result = Subsystem->RegisterMarker(Data);

	if (!Result.bSuccess)
	{
		UE_LOG(LogTemp, Warning, TEXT("MinimapMarker: 등록 실패 - %s"), *Result.FailReason);
	}
}

void UMinimapMarkerComponent::UnRegisterFromSubSystem()
{
	UWorld* World = GetWorld();
	if (!World) return;

	UMinimapSubsystem* Subsystem = World->GetSubsystem<UMinimapSubsystem>();
	if (!Subsystem) return;
	
	Subsystem->UnregisterMarker(MarkerId);
}

FMinimapMarkerData UMinimapMarkerComponent::BuildMakerData() const
{
	AActor* Owner = GetOwner();

	FMinimapMarkerData Data;
	Data.MarkerID = MarkerId;
	Data.MarkerTag = MarkerTag;
	Data.WorldLocation = Owner ? Owner->GetActorLocation() : FVector::ZeroVector;
	Data.IconInfo = IconInfo;
	Data.TrackedActor = Owner;

	return Data;
}

void UMinimapMarkerComponent::CreateArrivalTrigger()
{
	AActor* Owner = GetOwner();
	if (!Owner) return;

	ArrivalTrigger = NewObject<USphereComponent>(Owner);
	if (!ArrivalTrigger) return;

	ArrivalTrigger->SetSphereRadius(ArrivalRadius);
	ArrivalTrigger->SetCollisionProfileName("OverlapAllDynamic");
	ArrivalTrigger->SetGenerateOverlapEvents(true);

	ArrivalTrigger->AttachToComponent(
		Owner->GetRootComponent(),
		FAttachmentTransformRules::KeepRelativeTransform);
	
	ArrivalTrigger->RegisterComponent();
	
	ArrivalTrigger->OnComponentBeginOverlap.AddDynamic(this, &UMinimapMarkerComponent::OnArrivalOverlapBegin);
}
