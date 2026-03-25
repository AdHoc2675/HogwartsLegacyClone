// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/HOG_WidgetController.h"
#include "UI/Player/HOGPlayerHUDBase.h"
#include "GameplayTagContainer.h"
#include "HOGDebugHelper.h"
#include "Character/Enemy/EnemyCharacterBase.h"
#include "Character/Player/PlayerCharacterBase.h"
#include "Component/LockOnComponent.h"
#include "Component/SpellComponent.h"
#include "Core/HOG_GameplayTags.h"
#include "Data/Enemy/DA_EnemyConfigBase.h"
#include "GameFramework/HOG_PlayerController.h"
#include "GameFramework/HOG_PlayerState.h"
#include "GAS/Attributes/HOGAttributeSet.h"
#include "Minimap/MinimapCaptureComponent.h"
#include "Minimap/MinimapWidget.h"
#include "UI/Enemy/HOGEnemyHUDBase.h"
#include "UI/SubtitleWidget.h"

void UHOG_WidgetController::Init(AHOG_PlayerController* InPlayerController, APlayerCharacterBase* InPlayerCharacter,
                                 TSubclassOf<UHOGPlayerHUDBase> InPlayerHUDClass,
                                 TSubclassOf<UHOGEnemyHUDBase> InEnemyHUDClass,
                                 TSubclassOf<UMinimapWidget> InMinimapClass)
{
	if (!InPlayerController || !InPlayerCharacter) return;

	// PlayerWidget 생성
	PlayerHUDClass = InPlayerHUDClass;
	CreatePlayerWidget(InPlayerController);

	AHOG_PlayerState* PlayerState = GetPlayerState(InPlayerCharacter);

	if (!PlayerState)
	{
		Debug::Print("No Exist Player State In UHOG_WidgetController");
		return;
	}

	BindSpellComponent(PlayerState);
	BindPlayerHP(PlayerState);

	// EnemyWidget 생성
	EnemyHUDClass = InEnemyHUDClass;
	CreateEnemyWidget(InPlayerController);

	BindLockOnComponent(InPlayerCharacter);
	
	MinimapWidgetClass = InMinimapClass;
	CreateMiniMapWidget(InPlayerController, InPlayerCharacter);

	// SubtitleWidget 생성
	CreateSubtitleWidget(InPlayerController);
}

void UHOG_WidgetController::UnlockSpellSlot(FGameplayTag SpellID)
{
	if (PlayerHUD)
	{
		PlayerHUD->UnlockSpellSlot(SpellID);
	}
}

void UHOG_WidgetController::CreatePlayerWidget(AHOG_PlayerController* InPlayerController)
{
	if (!InPlayerController || !PlayerHUDClass) return;

	PlayerHUD = CreateWidget<UHOGPlayerHUDBase>(InPlayerController, PlayerHUDClass);
	if (PlayerHUD)
	{
		PlayerHUD->AddToViewport();
	}
}

AHOG_PlayerState* UHOG_WidgetController::GetPlayerState(APlayerCharacterBase* PlayerCharacter) const
{
	APawn* Pawn = Cast<APawn>(PlayerCharacter);
	if (!Pawn) return nullptr;

	APlayerController* PC = Cast<APlayerController>(Pawn->GetController());
	if (!PC) return nullptr;

	return PC->GetPlayerState<AHOG_PlayerState>();
}

void UHOG_WidgetController::BindSpellComponent(AHOG_PlayerState* PlayerState)
{
	if (!PlayerState) return;

	SpellComponent = PlayerState->GetSpellComponent();
	if (!SpellComponent) return;

	SpellComponent->OnSpellCooldownStarted.AddDynamic(this, &UHOG_WidgetController::OnSpellCooldownStarted);
	SpellComponent->OnSpellCooldownEnded.AddDynamic(this, &UHOG_WidgetController::OnSpellCooldownEnded);
}

void UHOG_WidgetController::UnBindSpellComponent()
{
	if (!SpellComponent) return;

	SpellComponent->OnSpellCooldownStarted.RemoveAll(this);
	SpellComponent->OnSpellCooldownEnded.RemoveAll(this);
	SpellComponent = nullptr;
}

void UHOG_WidgetController::BindPlayerHP(AHOG_PlayerState* PlayerState)
{
	if (!PlayerState) return;

	IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(PlayerState);
	if (!ASI) return;

	PlayerASC = ASI->GetAbilitySystemComponent();
	if (!PlayerASC) return;

	PlayerHPDelegateHandle = PlayerASC->GetGameplayAttributeValueChangeDelegate(
		UHOGAttributeSet::GetHealthAttribute()
	).AddUObject(this, &UHOG_WidgetController::OnPlayerHpChanged);

	bool bFound = false;
	float CurHP = PlayerASC->GetGameplayAttributeValue(UHOGAttributeSet::GetHealthAttribute(), bFound);
	float MaxHP = PlayerASC->GetGameplayAttributeValue(UHOGAttributeSet::GetMaxHealthAttribute(), bFound);

	if (PlayerHUD && bFound)
	{
		PlayerHUD->SetPlayerHP(CurHP, MaxHP);
	}
}

void UHOG_WidgetController::UnbindPlayerHP()
{
	if (PlayerASC && PlayerHPDelegateHandle.IsValid())
	{
		PlayerASC->GetGameplayAttributeValueChangeDelegate(
			UHOGAttributeSet::GetHealthAttribute()
		).Remove(PlayerHPDelegateHandle);
		PlayerHPDelegateHandle.Reset();
		PlayerASC = nullptr;
	}
}

void UHOG_WidgetController::OnPlayerHpChanged(const FOnAttributeChangeData& Data)
{
	if (!PlayerHUD || !PlayerASC) return;

	bool bFound = false;
	float MaxHP = PlayerASC->GetGameplayAttributeValue(UHOGAttributeSet::GetMaxHealthAttribute(), bFound);

	PlayerHUD->SetPlayerHP(Data.NewValue, MaxHP);
}

void UHOG_WidgetController::OnSpellCooldownStarted(FGameplayTag SpellID, float CooldownSeconds)
{
	if (PlayerHUD)
	{
		PlayerHUD->SetCooldownActive(SpellID, CooldownSeconds);
	}
}

void UHOG_WidgetController::OnSpellCooldownEnded(FGameplayTag SpellID)
{
}

void UHOG_WidgetController::CreateEnemyWidget(AHOG_PlayerController* InPlayerController)
{
	if (!EnemyHUDClass || !InPlayerController) return;

	EnemyHUD = CreateWidget<UHOGEnemyHUDBase>(InPlayerController, EnemyHUDClass);
	if (EnemyHUD)
	{
		EnemyHUD->AddToViewport();
		EnemyHUD->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UHOG_WidgetController::BindLockOnComponent(APlayerCharacterBase* PlayerCharacter)
{
	LockOnComponent = PlayerCharacter->FindComponentByClass<ULockOnComponent>();
	if (!LockOnComponent) return;

	LockOnComponent->OnLockOnTarget.AddUObject(this, &UHOG_WidgetController::HandleLockOn);
	LockOnComponent->OnLockOnReleased.AddUObject(this, &UHOG_WidgetController::HandleLockOnReleased);
}

void UHOG_WidgetController::UnBindLockOnComponent()
{
	if (!LockOnComponent) return;
	LockOnComponent->OnLockOnTarget.RemoveAll(this);
	LockOnComponent->OnLockOnReleased.RemoveAll(this);

	LockOnComponent = nullptr;
}

void UHOG_WidgetController::HandleLockOn(const FLockOnTargetResult& TargetResult)
{
	if (!EnemyHUD || !TargetResult.TargetActor) return;
	if (!TargetResult.TargetTags.HasTag(HOGGameplayTags::Team_Enemy)) {
		HandleLockOnReleased(TargetResult);
		return;
	}
	
	ClearEnemyBinding();

	IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(TargetResult.TargetActor);
	if (!ASI) return;

	UAbilitySystemComponent* ASC = ASI->GetAbilitySystemComponent();
	if (!ASC) return;

	BindEnemyASC(ASC);

	bool bFound = false;
	float CurHp = ASC->GetGameplayAttributeValue(UHOGAttributeSet::GetHealthAttribute(), bFound);
	float MaxHp = ASC->GetGameplayAttributeValue(UHOGAttributeSet::GetMaxHealthAttribute(), bFound);

	EnemyHUD->SetEnemyHp(CurHp, MaxHp);
	
	AEnemyCharacterBase* Enemy = Cast<AEnemyCharacterBase>(TargetResult.TargetActor);
	if (Enemy && Enemy->GetEnemyConfig())
	{
		EnemyHUD->SetEnemyName(Enemy->GetEnemyConfig()->EnemyName);
	}
	EnemyHUD->SetVisibility(ESlateVisibility::Visible);
}

void UHOG_WidgetController::HandleLockOnReleased(const FLockOnTargetResult& TargetResult)
{
	ClearEnemyBinding();

	if (EnemyHUD)
	{
		EnemyHUD->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UHOG_WidgetController::BindEnemyASC(UAbilitySystemComponent* TargetASC)
{
	BoundEnemyASC = TargetASC;

	EnemyHPDelegateHandle = TargetASC->GetGameplayAttributeValueChangeDelegate(UHOGAttributeSet::GetHealthAttribute())
	                                 .AddUObject(this, &UHOG_WidgetController::OnEnemyHpChanged);
}

void UHOG_WidgetController::ClearEnemyBinding()
{
	if (BoundEnemyASC && EnemyHPDelegateHandle.IsValid())
	{
		BoundEnemyASC->GetGameplayAttributeValueChangeDelegate(UHOGAttributeSet::GetHealthAttribute())
		             .Remove(EnemyHPDelegateHandle);
		
		EnemyHPDelegateHandle.Reset();
		BoundEnemyASC = nullptr;
	}
}

void UHOG_WidgetController::OnEnemyHpChanged(const FOnAttributeChangeData& Data)
{
	if (!EnemyHUD || !BoundEnemyASC) return;
	
	bool bFound = false;
	float MaxHp = BoundEnemyASC->GetGameplayAttributeValue(UHOGAttributeSet::GetMaxHealthAttribute(), bFound);
	
	EnemyHUD->SetEnemyHp(Data.NewValue, MaxHp);
}

void UHOG_WidgetController::CreateMiniMapWidget( AHOG_PlayerController* InPlayerController, 
	APlayerCharacterBase* InPlayerCharacter)
{
	if (!InPlayerController || !InPlayerCharacter || !MinimapWidgetClass) return;

	UMinimapCaptureComponent* Capture = 
		InPlayerCharacter->FindComponentByClass<UMinimapCaptureComponent>();
	if (!Capture) return;

	MinimapWidget = CreateWidget<UMinimapWidget>(InPlayerController, MinimapWidgetClass);
	if (!MinimapWidget) return;

	MinimapWidget->AddToViewport();
	MinimapWidget->InitializeMiniMap(Capture);
}

void UHOG_WidgetController::ShutdownMiniMapWidget()
{
	if (MinimapWidget)
	{
		MinimapWidget->ShutdownMiniMap();
		MinimapWidget = nullptr;
	}
}

void UHOG_WidgetController::Shutdown()
{
	UnBindSpellComponent();
	UnbindPlayerHP();
	UnBindLockOnComponent();
	ClearEnemyBinding();
	ShutdownMiniMapWidget();
}

void UHOG_WidgetController::CreateSubtitleWidget(AHOG_PlayerController* InPlayerController)
{
	if (!InPlayerController) return;

	// PlayerController에 선언될 SubtitleWidgetClass를 가져옴
	if (InPlayerController->SubtitleWidgetClass)
	{
		SubtitleWidget = CreateWidget<USubtitleWidget>(InPlayerController, InPlayerController->SubtitleWidgetClass);
		if (SubtitleWidget)
		{
			SubtitleWidget->AddToViewport(50); // 다른 UI보다 위에
			SubtitleWidget->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

void UHOG_WidgetController::RequestSubtitle(const FString& Text, float Duration)
{
	if (SubtitleWidget)
	{
		SubtitleWidget->ShowSubtitle(Text, Duration);
	}
}
