// Fill out your copyright notice in the Description page of Project Settings.


#include "Minimap/MinimapCaptureComponent.h"

#include "Components/SceneCaptureComponent2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Minimap/MinimapData.h"

// Sets default values for this component's properties
UMinimapCaptureComponent::UMinimapCaptureComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
	bWantsInitializeComponent = true; 
	
}

UTextureRenderTarget2D* UMinimapCaptureComponent::GetRenderTarget() const
{
	return RenderTarget;
}

// 현재 미니맵에서의 마커의 위치 반환
FMinimapScreenPosition UMinimapCaptureComponent::WorldToScreenPosition(const FVector& WorldLocation) const
{
	FMinimapScreenPosition Result;

	AActor* Owner = GetOwner();
	if (!Owner) return Result;

	if (CaptureRadius <= KINDA_SMALL_NUMBER) return Result;

	const FVector Offset = WorldLocation - Owner->GetActorLocation();

	const float NormalizedX = Offset.X / CaptureRadius;
	const float NormalizedY = Offset.Y / CaptureRadius;

	const float DistSq = NormalizedX * NormalizedX + NormalizedY * NormalizedY;
	Result.bIsInRange = (DistSq <= 1.f);
	
	Result.ScreenPosition.X = (NormalizedY * 0.5f) + 0.5f;
	Result.ScreenPosition.Y = (-NormalizedX * 0.5f) + 0.5f;

	return Result;
}

void UMinimapCaptureComponent::BeginPlay()
{
	Super::BeginPlay();
	
	InitializeCaptureCamera();

	if (AActor* Owner = GetOwner())
	{
		LastCaptureLocation = Owner->GetActorLocation();
	}
}

void UMinimapCaptureComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (CaptureComponent)
	{
		CaptureComponent->DestroyComponent();
		CaptureComponent = nullptr;
	}

	Super::EndPlay(EndPlayReason);
}

void UMinimapCaptureComponent::TickComponent(float DeltaTime, enum ELevelTick TickType,
                                             FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	TimeSinceLastCapture += DeltaTime;
	// 캡쳐 간격 Threshold 
	if (TimeSinceLastCapture < CaptureUpdateInterval) return;

	TimeSinceLastCapture = 0.f;
	TryUpdateCapture();
}

void UMinimapCaptureComponent::InitializeComponent()
{
	Super::InitializeComponent();
	CreateRenderTarget();
}

void UMinimapCaptureComponent::InitializeCaptureCamera()
{
	AActor* Owner = GetOwner();
	if (!Owner) return;

	CaptureComponent = NewObject<USceneCaptureComponent2D>(Owner);
	if (!CaptureComponent) return;

	CaptureComponent->RegisterComponent();
	CaptureComponent->AttachToComponent(
		Owner->GetRootComponent(),
		FAttachmentTransformRules::KeepRelativeTransform);

	// 직교 카메라 생성
	CaptureComponent->ProjectionType = ECameraProjectionMode::Orthographic;
	CaptureComponent->OrthoWidth = CaptureRadius * 2.f;
	
	// 타겟 설정
	CaptureComponent->TextureTarget = RenderTarget;
	CaptureComponent->SetRelativeLocation(FVector(0.f, 0.f, CaptureHeight));
	CaptureComponent->SetRelativeRotation(FRotator(-90.f, 0.f, 0.f));

	CaptureComponent->CaptureSource = SCS_FinalColorLDR;
	CaptureComponent->bCaptureEveryFrame = false;
	CaptureComponent->bCaptureOnMovement = false;

	CaptureComponent->CaptureScene();
}

void UMinimapCaptureComponent::CreateRenderTarget()
{
	RenderTarget = NewObject<UTextureRenderTarget2D>(this);
	if (!RenderTarget) return;

	// 미니맵 크기 설정
	RenderTarget->InitAutoFormat(RenderTargetResolution, RenderTargetResolution);
	RenderTarget->UpdateResourceImmediate(true);
}

bool UMinimapCaptureComponent::TryUpdateCapture()
{
	if (!CaptureComponent) return false;

	AActor* Owner = GetOwner();
	if (!Owner) return false;

	const FVector CurrentLocation = Owner->GetActorLocation();

	const float DistanceMoved = FVector::Dist2D(CurrentLocation, LastCaptureLocation);
	// 이동이 threshold 미만이면 리턴
	if (DistanceMoved < RecaptureDistanceThreshold)
	{
		return false;
	}

	CaptureComponent->SetWorldLocation(
		FVector(CurrentLocation.X, CurrentLocation.Y, CurrentLocation.Z + CaptureHeight));

	// 항상 아래를 내려다보도록 월드 회전 고정
	CaptureComponent->SetWorldRotation(FRotator(-90.f, 0.f, 0.f));

	CaptureComponent->CaptureScene();
	LastCaptureLocation = CurrentLocation;

	return true;
}
