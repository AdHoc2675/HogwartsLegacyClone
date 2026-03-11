#include "Character/Player/PlayerCharacterBase.h"

#include "Camera/CameraComponent.h"
#include "Component/LockOnComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "UObject/ConstructorHelpers.h"

#include "GameFramework/HOG_PlayerState.h"
#include "GAS/HOGAbilitySystemComponent.h"
#include "Core/HOG_GameplayTags.h"
#include "HOGDebugHelper.h"

APlayerCharacterBase::APlayerCharacterBase()
{
	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;
	bUseControllerRotationYaw = false;

	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->bOrientRotationToMovement = true;
		MoveComp->RotationRate = FRotator(0.0f, 600.0f, 0.0f);
		MoveComp->JumpZVelocity = 700.f;
		MoveComp->AirControl = 0.35f;
		MoveComp->MaxWalkSpeed = 500.f;
		MoveComp->BrakingDecelerationWalking = 2000.f;
	}

	// 카메라 스프링암 셋팅
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(GetCapsuleComponent());
	CameraBoom->TargetArmLength = 400.f;
	CameraBoom->bUsePawnControlRotation = true;

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
	else
	{
		// Debug::Print(
		// 	TEXT("[PlayerCharacterBase] Failed to load WandMesh: /Game/Fab/Ornamental_Wand/OrnamentalWand.OrnamentalWand"),
		// 	FColor::Red
		// );
	}

	LockOnComponent = CreateDefaultSubobject<ULockOnComponent>(TEXT("LockOnComponent"));
}

void APlayerCharacterBase::BeginPlay()
{
	Super::BeginPlay();

	if (UHOGAbilitySystemComponent* HOGASC = GetHOGAbilitySystemComponent())
	{
		HOGASC->RegisterGameplayTagEvent(HOGGameplayTags::State_Combat_Active, EGameplayTagEventType::NewOrRemoved)
			.AddUObject(this, &APlayerCharacterBase::HandleCombatActiveTagChanged);

		HOGASC->RegisterGameplayTagEvent(HOGGameplayTags::State_Combat_Inactive, EGameplayTagEventType::NewOrRemoved)
			.AddUObject(this, &APlayerCharacterBase::HandleCombatInactiveTagChanged);

		HOGASC->RegisterGameplayTagEvent(HOGGameplayTags::State_Casting_Active, EGameplayTagEventType::NewOrRemoved)
			.AddUObject(this, &APlayerCharacterBase::HandleCastingActiveTagChanged);

		HOGASC->RegisterGameplayTagEvent(HOGGameplayTags::State_Casting_Inactive, EGameplayTagEventType::NewOrRemoved)
			.AddUObject(this, &APlayerCharacterBase::HandleCastingInactiveTagChanged);

		bCombatActive = HOGASC->HasMatchingGameplayTag(HOGGameplayTags::State_Combat_Active);
		bCastingActive = HOGASC->HasMatchingGameplayTag(HOGGameplayTags::State_Casting_Active);
	}

	RefreshWandVisibilityFromCombatState();
}

void APlayerCharacterBase::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	TeamTag = HOGGameplayTags::Team_Player;

	InitializeAbilityActorInfo();
}

void APlayerCharacterBase::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	InitializeAbilityActorInfo();
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

	// Debug::Print(FString::Printf(
	// 	TEXT("[PlayerCharacterBase] ASC Init Success | ASC=%s | Owner=%s | Avatar=%s"),
	// 	*GetNameSafe(HOGASC),
	// 	*GetNameSafe(HOGASC->GetOwnerActor()),
	// 	*GetNameSafe(HOGASC->GetAvatarActor())
	// ), FColor::Green);
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

	// Debug::Print(FString::Printf(
	// 	TEXT("[PlayerCharacterBase] Combat Active Tag Changed | NewCount=%d | bCombatActive=%s"),
	// 	NewCount,
	// 	(bCombatActive ? TEXT("true") : TEXT("false"))
	// ), FColor::Cyan);
}

void APlayerCharacterBase::HandleCombatInactiveTagChanged(const FGameplayTag CallbackTag, int32 NewCount)
{
	if (NewCount > 0)
	{
		bCombatActive = false;
		RefreshWandVisibilityFromCombatState();
	}

	// Debug::Print(FString::Printf(
	// 	TEXT("[PlayerCharacterBase] Combat Inactive Tag Changed | NewCount=%d | bCombatActive=%s"),
	// 	NewCount,
	// 	(bCombatActive ? TEXT("true") : TEXT("false"))
	// ), FColor::Silver);
}

void APlayerCharacterBase::HandleCastingActiveTagChanged(const FGameplayTag CallbackTag, int32 NewCount)
{
	bCastingActive = (NewCount > 0);
	RefreshWandVisibilityFromCombatState();

	// Debug::Print(FString::Printf(
	// 	TEXT("[PlayerCharacterBase] Casting Active Tag Changed | NewCount=%d | bCastingActive=%s"),
	// 	NewCount,
	// 	(bCastingActive ? TEXT("true") : TEXT("false"))
	// ), FColor::Orange);
}

void APlayerCharacterBase::HandleCastingInactiveTagChanged(const FGameplayTag CallbackTag, int32 NewCount)
{
	if (NewCount > 0)
	{
		bCastingActive = false;
		RefreshWandVisibilityFromCombatState();
	}

	// Debug::Print(FString::Printf(
	// 	TEXT("[PlayerCharacterBase] Casting Inactive Tag Changed | NewCount=%d | bCastingActive=%s"),
	// 	NewCount,
	// 	(bCastingActive ? TEXT("true") : TEXT("false"))
	// ), FColor::Orange);
}

void APlayerCharacterBase::RefreshWandVisibilityFromCombatState()
{
	const bool bShouldShowWand = (bCombatActive || bCastingActive);

	SetWandVisible(bShouldShowWand);

	// Debug::Print(FString::Printf(
	// 	TEXT("[PlayerCharacterBase] RefreshWandVisibility | Combat=%s | Casting=%s | Show=%s"),
	// 	(bCombatActive ? TEXT("true") : TEXT("false")),
	// 	(bCastingActive ? TEXT("true") : TEXT("false")),
	// 	(bShouldShowWand ? TEXT("true") : TEXT("false"))
	// ), FColor::Green);
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

	// Debug::Print(FString::Printf(
	// 	TEXT("[PlayerCharacterBase] SetCanQueueNextCombo | %s"),
	// 	bCanQueueNextCombo ? TEXT("true") : TEXT("false")
	// ), FColor::Cyan);
}
