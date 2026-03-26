// Fill out your copyright notice in the Description page of Project Settings.


#include "GameFramework/HOG_PlayerController.h"

#include "Data/DA_InputConfig.h" 
#include "Character/Player/PlayerCharacterBase.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "InputMappingContext.h"
#include "Engine/LocalPlayer.h"
#include "GAS/HOGAbilitySystemComponent.h"
#include "Pool/DamageNumberPool.h"
#include "UI/HOG_WidgetController.h"
#include "Kismet/GameplayStatics.h"
#include "Components/AudioComponent.h"

AHOG_PlayerController::AHOG_PlayerController()
{
}


void AHOG_PlayerController::BeginPlay()
{
	Super::BeginPlay();

	ApplyDefaultMappingContext();
	
	// WidgetController 생성
	WidgetController = NewObject<UHOG_WidgetController>(this);
	if (APlayerCharacterBase* PC = GetPlayerCharacterBase())
	{
		WidgetController->Init(this, PC, PlayerHUDClass, EnemyHUDClass, MinimapWidgetClass);
	}
	
	// 데미지 넘버 풀 생성
	DamageNumberPool = NewObject<UDamageNumberPool>(this);
	DamageNumberPool->InitPool(this, DamageNumberWidgetClass);
	
	for (const FGameplayTag& SpellTag : InitialUnlockedSpells)
	{
		if (SpellTag.IsValid())
		{
			UnlockSpellUI(SpellTag);
		}
	}
}

void AHOG_PlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (WidgetController)
	{
		WidgetController->Shutdown();
		WidgetController = nullptr;
	}

	Super::EndPlay(EndPlayReason);
}

void AHOG_PlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	
	UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(InputComponent);
	if (!EIC)
	{
		//실패 시 로그
		UE_LOG(LogTemp, Error, TEXT("[HOG_PC] InputComponent is not EnhancedInputComponent. Check Project Settings > Input / Enhanced Input setup."));
		return;
	}
	
	ApplyDefaultMappingContext();

	BindDefaultActions();
	BindAbilityActions();
}

void AHOG_PlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);
	
	APlayerCharacterBase* PC = GetPlayerCharacterBase();
	if (!PC)
	{
		return;
	}
	
	UHOGAbilitySystemComponent* HOGASC = PC->GetHOGAbilitySystemComponent();
	if (!HOGASC)
	{
		return;	
	}
	
	HOGASC->ProcessAbilityInput(DeltaTime,false);
}

void AHOG_PlayerController::ApplyDefaultMappingContext()
{
	if (bAppliedMappingContext)
		return;

	if (!InputConfig)
	{//실패시 로그
		UE_LOG(LogTemp, Warning, TEXT("[HOG_PC] InputConfig is null. Set DA_InputConfig in BP (PlayerController defaults)."));
		return;
	}

	if (!InputConfig->DefaultMappingContext)
	{//실패시 로그
		UE_LOG(LogTemp, Warning, TEXT("[HOG_PC] InputConfig->DefaultMappingContext is null."));
		return;
	}

	ULocalPlayer* LP = GetLocalPlayer();
	if (!LP)
	{//실패시 로그
		UE_LOG(LogTemp, Warning, TEXT("[HOG_PC] LocalPlayer is null."));
		return;
	}

	UEnhancedInputLocalPlayerSubsystem* Subsys = LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
	if (!Subsys)
	{//실패시 로그
		UE_LOG(LogTemp, Warning, TEXT("[HOG_PC] EnhancedInputLocalPlayerSubsystem is null."));
		return;
	}

	Subsys->AddMappingContext(InputConfig->DefaultMappingContext, DefaultMappingPriority);
	bAppliedMappingContext = true;

	UE_LOG(LogTemp, Log, TEXT("[HOG_PC] DefaultMappingContext applied. Priority=%d"), DefaultMappingPriority);
}

APlayerCharacterBase* AHOG_PlayerController::GetPlayerCharacterBase() const
{
	return Cast<APlayerCharacterBase>(GetPawn());
}

void AHOG_PlayerController::BindDefaultActions()
{
	if (!InputConfig)
		return;

	UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(InputComponent);
	if (!EIC)
		return;

	// Move
	if (InputConfig->IA_Move)
	{
		EIC->BindAction(InputConfig->IA_Move, ETriggerEvent::Triggered, this, &AHOG_PlayerController::OnMoveTriggered);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[HOG_PC] IA_Move is null."));
	}

	// Look
	if (InputConfig->IA_Look)
	{
		EIC->BindAction(InputConfig->IA_Look, ETriggerEvent::Triggered, this, &AHOG_PlayerController::OnLookTriggered);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[HOG_PC] IA_Look is null."));
	}

	// Jump (Started/Completed)
	if (InputConfig->IA_Jump)
	{
		EIC->BindAction(InputConfig->IA_Jump, ETriggerEvent::Started, this, &AHOG_PlayerController::OnJumpStarted);
		EIC->BindAction(InputConfig->IA_Jump, ETriggerEvent::Completed, this, &AHOG_PlayerController::OnJumpCompleted);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[HOG_PC] IA_Jump is null."));
	}

	// Interact
	if (InputConfig->IA_Interact)
	{
		EIC->BindAction(InputConfig->IA_Interact, ETriggerEvent::Started, this, &AHOG_PlayerController::OnInteractStarted);
	}
}

void AHOG_PlayerController::BindAbilityActions()
{
	if (!InputConfig)
		return;

	UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(InputComponent);
	if (!EIC)
		return;
	
	AbilityActionToTag.Reset();

	// Tag->Action 맵 바인딩
	for (const TPair<FGameplayTag, UInputAction*>& Pair : InputConfig->AbilityInputActions)
	{
		const FGameplayTag Tag = Pair.Key;
		UInputAction* Action = Pair.Value;

		if (!Tag.IsValid() || !Action)
			continue;

		AbilityActionToTag.Add(Action,Tag);
		
		EIC->BindAction(Action,ETriggerEvent::Started,this,&AHOG_PlayerController::OnAbilityActionStarted);
		EIC->BindAction(Action,ETriggerEvent::Completed,this,&AHOG_PlayerController::OnAbilityActionCompleted);
		
	}
}

void AHOG_PlayerController::OnMoveTriggered(const FInputActionValue& Value)
{
	if (APlayerCharacterBase* PC = GetPlayerCharacterBase())
	{
		PC->Input_Move(Value);
	}
}

void AHOG_PlayerController::OnLookTriggered(const FInputActionValue& Value)
{
	if (APlayerCharacterBase* PC = GetPlayerCharacterBase())
	{
		PC->Input_Look(Value);
	}
}

void AHOG_PlayerController::OnJumpStarted()
{
	if (APlayerCharacterBase* PC = GetPlayerCharacterBase())
	{
		PC->Input_JumpStarted();
	}
}

void AHOG_PlayerController::OnJumpCompleted()
{
	if (APlayerCharacterBase* PC = GetPlayerCharacterBase())
	{
		PC->Input_JumpCompleted();
	}
}

void AHOG_PlayerController::OnInteractStarted()
{
	if (APlayerCharacterBase* PC = GetPlayerCharacterBase())
	{
		
		PC->Input_Interact();
	}
}

void AHOG_PlayerController::OnAbilityActionStarted(const FInputActionInstance& Instance)
{
	const UInputAction* SourceAction = Instance.GetSourceAction();
	if (!SourceAction)
		return;

	const FGameplayTag* FoundTag = AbilityActionToTag.Find(SourceAction);
	if (!FoundTag)
		return;

	HandleAbilityPressed(*FoundTag);
}

void AHOG_PlayerController::OnAbilityActionCompleted(const FInputActionInstance& Instance)
{
	const UInputAction* SourceAction = Instance.GetSourceAction();
	if (!SourceAction)
		return;

	const FGameplayTag* FoundTag = AbilityActionToTag.Find(SourceAction);
	if (!FoundTag)
		return;

	HandleAbilityReleased(*FoundTag);
}

void AHOG_PlayerController::HandleAbilityPressed(FGameplayTag InputTag)
{
	if (APlayerCharacterBase* PC = GetPlayerCharacterBase())
	{
		PC->Input_AbilityInputPressed(InputTag);
	}
}

void AHOG_PlayerController::HandleAbilityReleased(FGameplayTag InputTag)
{
	if (APlayerCharacterBase* PC = GetPlayerCharacterBase())
	{
		PC->Input_AbilityInputReleased(InputTag);
	}
}

void AHOG_PlayerController::UnlockSpellUI(FGameplayTag SpellID)
{
	if (WidgetController)
	{
		WidgetController->UnlockSpellSlot(SpellID);
	}
}

// =============== [BGM System] ===============
void AHOG_PlayerController::PlayBGMWithFade(USoundBase* NewBGM, float FadeInTime, float FadeOutTime)
{
	if (!NewBGM) return;

	// 새로 재생할 음악이 현재 재생중인 음악과 같다면 무시
	if (CurrentBGMComponent && CurrentBGMComponent->Sound == NewBGM && CurrentBGMComponent->IsPlaying())
	{
		return;
	}

	// 1. 기존에 재생 중인 음악이 있다면 Fade Out
	if (CurrentBGMComponent && CurrentBGMComponent->IsPlaying())
	{
		// FadeOut이 끝나면 컴포넌트는 자동으로 Destroy 됩니다 (기본 bAutoDestroy=true 속성)
		CurrentBGMComponent->FadeOut(FadeOutTime, 0.0f);
	}

	// 2. 새로운 음악을 생성하고 Fade In
	CurrentBGMComponent = UGameplayStatics::CreateSound2D(this, NewBGM);
	if (CurrentBGMComponent)
	{
		CurrentBGMComponent->FadeIn(FadeInTime);
	}
}

void AHOG_PlayerController::StopBGMWithFade(float FadeOutTime)
{
	// 현재 재생 중인 음악을 서서히 줄여서 끕니다.
	if (CurrentBGMComponent && CurrentBGMComponent->IsPlaying())
	{
		CurrentBGMComponent->FadeOut(FadeOutTime, 0.0f);
		CurrentBGMComponent = nullptr;
	}
}