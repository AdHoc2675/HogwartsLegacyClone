#include "Character/Player/PlayerCharacterBase.h"

#include "Camera/CameraComponent.h"

#include "Component/LockOnComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GAS/HOGAbilitySystemComponent.h"

#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/HOG_PlayerState.h"

#include "Interactable/InteractableInterface.h"
#include "Kismet/KismetSystemLibrary.h"

#include "UObject/ConstructorHelpers.h"
#include "Core/HOG_GameplayTags.h"
#include "HOGDebugHelper.h"

#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"

#include "Components/SkeletalMeshComponent.h"
#include "Core/HOG_Struct.h"
#include "GAS/Abilities/GA_SpellBase.h"




APlayerCharacterBase::APlayerCharacterBase()
{
	PrimaryActorTick.bCanEverTick = true;

	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;
	bUseControllerRotationYaw = false;

	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		// 평소엔 자유시점 / 자유이동
		MoveComp->bOrientRotationToMovement = true;
		MoveComp->RotationRate = FRotator(0.0f, 720.0f, 0.0f);
		MoveComp->JumpZVelocity = 700.f;
		MoveComp->AirControl = 0.35f;
		MoveComp->MaxWalkSpeed = 500.f;
		MoveComp->BrakingDecelerationWalking = 2000.f;
	}

	// 카메라 스프링암 셋팅
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(GetCapsuleComponent());
	CameraBoom->TargetArmLength = 300.f;
	CameraBoom->SocketOffset = FVector(0.f, 65.f, 20.f);
	CameraBoom->TargetOffset = FVector(0.f, 0.f, 55.f);
	CameraBoom->bUsePawnControlRotation = true;
	CameraBoom->bDoCollisionTest = true;
	CameraBoom->bEnableCameraLag = false;
	CameraBoom->bEnableCameraRotationLag = false;

	// 카메라 셋팅
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	// Capsule Size
	GetCapsuleComponent()->InitCapsuleSize(CapsuleRadius, CapsuleHalfHeight);

	// Mesh 기본 세팅(캡슐 기준 위치/회전)
	if (USkeletalMeshComponent* MeshComp = GetMesh())
	{
		MeshComp->SetRelativeLocation(MeshRelativeLocation);
		MeshComp->SetRelativeRotation(MeshRelativeRotation);
		MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	// Wand Mesh
	WandMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WandMesh"));
	WandMesh->SetupAttachment(GetMesh(), TEXT("RightHandWandSocket"));
	WandMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WandMesh->SetGenerateOverlapEvents(false);
	WandMesh->SetHiddenInGame(true);
	WandMesh->SetVisibility(false, true);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> WandMeshFinder(
		TEXT("/Game/Fab/Ornamental_Wand/OrnamentalWand.OrnamentalWand")
	);
	if (WandMeshFinder.Succeeded())
	{
		WandMesh->SetStaticMesh(WandMeshFinder.Object);
	}

	LockOnComponent = CreateDefaultSubobject<ULockOnComponent>(TEXT("LockOnComponent"));
}

void APlayerCharacterBase::BeginPlay()
{
	Super::BeginPlay();

	BindASCGameplayTagCallbacks();
	RefreshWandVisibilityFromCombatState();
}

void APlayerCharacterBase::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	UpdateForcedFacing(DeltaSeconds);
}

void APlayerCharacterBase::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	TeamTag = HOGGameplayTags::Team_Player;

	InitializeAbilityActorInfo();
	BindASCGameplayTagCallbacks();
}

void APlayerCharacterBase::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	InitializeAbilityActorInfo();
	BindASCGameplayTagCallbacks();
}

void APlayerCharacterBase::InitializeAbilityActorInfo()
{
	AHOG_PlayerState* HOGPlayerState = GetPlayerState<AHOG_PlayerState>();
	if (!HOGPlayerState)
	{
		return;
	}

	UHOGAbilitySystemComponent* HOGASC = Cast<UHOGAbilitySystemComponent>(HOGPlayerState->GetAbilitySystemComponent());
	if (!HOGASC)
	{
		return;
	}

	// OwnerActor = PlayerState, AvatarActor = Character
	HOGASC->InitAbilityActorInfo(HOGPlayerState, this);
}

void APlayerCharacterBase::BindASCGameplayTagCallbacks()
{
	UHOGAbilitySystemComponent* HOGASC = GetHOGAbilitySystemComponent();
	if (!HOGASC)
	{
		return;
	}

	HOGASC->RegisterGameplayTagEvent(HOGGameplayTags::State_Combat_Active, EGameplayTagEventType::NewOrRemoved)
		.RemoveAll(this);
	HOGASC->RegisterGameplayTagEvent(HOGGameplayTags::State_Combat_Active, EGameplayTagEventType::NewOrRemoved)
		.AddUObject(this, &APlayerCharacterBase::HandleCombatActiveTagChanged);

	HOGASC->RegisterGameplayTagEvent(HOGGameplayTags::State_Combat_Inactive, EGameplayTagEventType::NewOrRemoved)
		.RemoveAll(this);
	HOGASC->RegisterGameplayTagEvent(HOGGameplayTags::State_Combat_Inactive, EGameplayTagEventType::NewOrRemoved)
		.AddUObject(this, &APlayerCharacterBase::HandleCombatInactiveTagChanged);

	HOGASC->RegisterGameplayTagEvent(HOGGameplayTags::State_Casting_Active, EGameplayTagEventType::NewOrRemoved)
		.RemoveAll(this);
	HOGASC->RegisterGameplayTagEvent(HOGGameplayTags::State_Casting_Active, EGameplayTagEventType::NewOrRemoved)
		.AddUObject(this, &APlayerCharacterBase::HandleCastingActiveTagChanged);

	HOGASC->RegisterGameplayTagEvent(HOGGameplayTags::State_Casting_Inactive, EGameplayTagEventType::NewOrRemoved)
		.RemoveAll(this);
	HOGASC->RegisterGameplayTagEvent(HOGGameplayTags::State_Casting_Inactive, EGameplayTagEventType::NewOrRemoved)
		.AddUObject(this, &APlayerCharacterBase::HandleCastingInactiveTagChanged);

	

	bCombatActive = HOGASC->HasMatchingGameplayTag(HOGGameplayTags::State_Combat_Active);
	bCastingActive = HOGASC->HasMatchingGameplayTag(HOGGameplayTags::State_Casting_Active);
}

void APlayerCharacterBase::BeginForcedFacingToLocation(const FVector& TargetLocation)
{
	FVector ToTarget = TargetLocation - GetActorLocation();
	ToTarget.Z = 0.f;

	if (ToTarget.IsNearlyZero())
	{
		return;
	}

	const FRotator TargetRot = ToTarget.Rotation();
	BeginForcedFacingToRotation(TargetRot);
}

void APlayerCharacterBase::BeginForcedFacingToRotation(const FRotator& TargetRotation)
{
	ForcedFacingTargetRotation = FRotator(0.f, TargetRotation.Yaw, 0.f);
	bForcedFacingActive = true;
	
	// 강제 회전중에는 이동방향 일시 중지
	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->bOrientRotationToMovement = false;
	}
}

void APlayerCharacterBase::StopForcedFacing()
{
	bForcedFacingActive = false;

	// 평상시 회전 복구
	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->bOrientRotationToMovement = true;
	}
}

bool APlayerCharacterBase::IsForcedFacingFinished() const
{
	const float DeltaYaw = FMath::Abs(FMath::FindDeltaAngleDegrees(
		GetActorRotation().Yaw,
		ForcedFacingTargetRotation.Yaw
	));

	return DeltaYaw <= ForcedFacingAcceptAngle;
}

void APlayerCharacterBase::UpdateForcedFacing(float DeltaSeconds)
{
	if (!bForcedFacingActive)
	{
		return;
	}
	
	const FRotator CurrentRot = GetActorRotation();
	const FRotator NewRot = FMath::RInterpTo(CurrentRot, ForcedFacingTargetRotation, DeltaSeconds, ForcedFacingInterpSpeed);
	
	SetActorRotation(FRotator(0.f, NewRot.Yaw, 0.f));

	if (IsForcedFacingFinished())
	{
		SetActorRotation(ForcedFacingTargetRotation);
		StopForcedFacing();
	}
}

UHOGAbilitySystemComponent* APlayerCharacterBase::GetHOGAbilitySystemComponent() const
{
	const AHOG_PlayerState* HOGPlayerState = GetPlayerState<AHOG_PlayerState>();
	if (!HOGPlayerState)
	{
		return nullptr;
	}

	return Cast<UHOGAbilitySystemComponent>(HOGPlayerState->GetAbilitySystemComponent());
}

void APlayerCharacterBase::Input_Move(const FInputActionValue& Value)
{
	if (bCastingActive && !bForcedFacingActive)
	{
		return;
	}
	
	FVector2D MoveAxis = Value.Get<FVector2D>();

	if (!Controller)
	{
		return;
	}

	FRotator ControlRotation = Controller->GetControlRotation();
	FRotator YawRotation(0.f, ControlRotation.Yaw, 0.f);

	FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

	AddMovementInput(ForwardDirection, MoveAxis.Y * MoveForwardScale);
	AddMovementInput(RightDirection, MoveAxis.X * MoveRightScale);
}

void APlayerCharacterBase::Input_Look(const FInputActionValue& Value)
{
	FVector2D LookAxis = Value.Get<FVector2D>();

	AddControllerYawInput(LookAxis.X * LookYawScale);
	AddControllerPitchInput(LookAxis.Y * LookPitchScale);
}

void APlayerCharacterBase::Input_JumpStarted()
{
	Jump();
}

void APlayerCharacterBase::Input_JumpCompleted()
{
	StopJumping();
}

void APlayerCharacterBase::Input_Interact()
{
	// 캐릭터 기준 전방으로 탐색
	FVector StartLoc = GetActorLocation();
	FVector EndLoc = StartLoc + (GetActorForwardVector() * 300.0f); // 전방 300 유닛(3M) 앞까지 탐색

	TArray<AActor*> ActorsToIgnore;
	ActorsToIgnore.Add(this);

	FHitResult HitResult;

	bool bHit = UKismetSystemLibrary::SphereTraceSingle(
		this, StartLoc, EndLoc,
		50.0f,
		TraceTypeQuery1,
		false,
		ActorsToIgnore,
		EDrawDebugTrace::ForDuration,
		HitResult, true
	);

	if (bHit && HitResult.GetActor())
	{
		AActor* HitActor = HitResult.GetActor();

		if (HitActor->Implements<UInteractableInterface>())
		{
			if (IInteractableInterface::Execute_CanInteract(HitActor, this))
			{
				IInteractableInterface::Execute_Interact(HitActor, this);
			}
		}
	}
}

void APlayerCharacterBase::Input_AbilityInputPressed(FGameplayTag InputTag)
{
	UHOGAbilitySystemComponent* HOGASC = GetHOGAbilitySystemComponent();
	if (!HOGASC || !InputTag.IsValid())
	{
		return;
	}

	HOGASC->AbilityInputTagPressed(InputTag);
}

void APlayerCharacterBase::Input_AbilityInputReleased(FGameplayTag InputTag)
{
	UHOGAbilitySystemComponent* HOGASC = GetHOGAbilitySystemComponent();
	if (!HOGASC || !InputTag.IsValid())
	{
		return;
	}

	HOGASC->AbilityInputTagReleased(InputTag);
}

void APlayerCharacterBase::HandleCombatActiveTagChanged(const FGameplayTag CallbackTag, int32 NewCount)
{
	bCombatActive = (NewCount > 0);
	RefreshWandVisibilityFromCombatState();
}

void APlayerCharacterBase::HandleCombatInactiveTagChanged(const FGameplayTag CallbackTag, int32 NewCount)
{
	if (NewCount > 0)
	{
		bCombatActive = false;
		RefreshWandVisibilityFromCombatState();
	}
}

void APlayerCharacterBase::HandleCastingActiveTagChanged(const FGameplayTag CallbackTag, int32 NewCount)
{
	bCastingActive = (NewCount > 0);
	RefreshWandVisibilityFromCombatState();
}

void APlayerCharacterBase::HandleCastingInactiveTagChanged(const FGameplayTag CallbackTag, int32 NewCount)
{
	if (NewCount > 0)
	{
		bCastingActive = false;
		RefreshWandVisibilityFromCombatState();
	}
}




void APlayerCharacterBase::RefreshWandVisibilityFromCombatState()
{
	const bool bShouldShowWand = (bCombatActive || bCastingActive);
	SetWandVisible(bShouldShowWand);
}

void APlayerCharacterBase::SetWandVisible(bool bVisible)
{
	if (!WandMesh)
	{
		return;
	}

	WandMesh->SetHiddenInGame(!bVisible);
	WandMesh->SetVisibility(bVisible, true);
}

void APlayerCharacterBase::SetCanQueueNextCombo(bool bInCanQueue)
{
	bCanQueueNextCombo = bInCanQueue;
}

void APlayerCharacterBase::QueueSpellVFX(const FQueuedSpellVFXData& InVFXData)
{
	QueuedSpellVFXData = InVFXData;
	QueuedSpellVFXData.bPending = true;
}

bool APlayerCharacterBase::ConsumeAndSpawnQueuedSpellVFX()
{
	if (!QueuedSpellVFXData.bPending)
	{
		return false;
	}

	if (!QueuedSpellVFXData.NiagaraSystem)
	{
		QueuedSpellVFXData = FQueuedSpellVFXData();
		return false;
	}

	FVector StartLocation = GetActorLocation();
	FRotator SpawnRotation = GetActorRotation();

	// 1) WandMesh(static mesh) 소켓 우선
	if (QueuedSpellVFXData.StartSocketName != NAME_None &&
		WandMesh &&
		WandMesh->DoesSocketExist(QueuedSpellVFXData.StartSocketName))
	{
		StartLocation = WandMesh->GetSocketLocation(QueuedSpellVFXData.StartSocketName);
	}
	// 2) fallback : 캐릭터 SkeletalMesh 소켓
	else if (USkeletalMeshComponent* MeshComp = GetMesh())
	{
		if (MeshComp->DoesSocketExist(QueuedSpellVFXData.StartSocketName))
		{
			StartLocation = MeshComp->GetSocketLocation(QueuedSpellVFXData.StartSocketName);
		}
	}

	SpawnRotation = (QueuedSpellVFXData.TargetLocation - StartLocation).Rotation();

	UNiagaraComponent* NiagaraComp = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
		GetWorld(),
		QueuedSpellVFXData.NiagaraSystem,
		StartLocation,
		SpawnRotation,
		FVector(1.f),
		true,
		true,
		ENCPoolMethod::None,
		true
	);

	if (NiagaraComp)
	{
		NiagaraComp->SetVectorParameter(
			QueuedSpellVFXData.BeamStartParameterName,
			StartLocation
		);

		NiagaraComp->SetVectorParameter(
			QueuedSpellVFXData.BeamEndParameterName,
			QueuedSpellVFXData.TargetLocation
		);

		const float BeamLength = FVector::Distance(StartLocation, QueuedSpellVFXData.TargetLocation);

		NiagaraComp->SetFloatParameter(
			QueuedSpellVFXData.BeamLengthParameterName,
			BeamLength
		);
	}

	QueuedSpellVFXData = FQueuedSpellVFXData();
	return NiagaraComp != nullptr;
}

void APlayerCharacterBase::QueueCastNotifyAbility(UGA_SpellBase* InAbility)
{
	QueuedCastNotifyAbility = InAbility;
}

void APlayerCharacterBase::ConsumeCastNotifyAbility()
{
	if (!QueuedCastNotifyAbility)
	{
		return;
	}

	UGA_SpellBase* Ability = QueuedCastNotifyAbility;
	QueuedCastNotifyAbility = nullptr;

	if (IsValid(Ability))
	{
		Ability->HandleCastNotify();
	}
}