#include "Character/BaseCharacter.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Components/SkeletalMeshComponent.h"
#include "Component/CombatComponent.h"
#include "Data/DT_AttributeRow.h"
#include "Engine/DataTable.h"
#include "GAS/Attributes/HOGAttributeSet.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"
#include "HOGDebugHelper.h"

ABaseCharacter::ABaseCharacter()
{
	PrimaryActorTick.bCanEverTick = false;
	PrimaryActorTick.bStartWithTickEnabled = false;

	CombatComponent = CreateDefaultSubobject<UCombatComponent>(TEXT("CombatComponent"));

	if (USkeletalMeshComponent* MeshComp = GetMesh())
	{
		MeshComp->bReceivesDecals = false;
	}
}

void ABaseCharacter::BeginPlay()
{
	Super::BeginPlay();

	// Enemy처럼 자기 자신 ASC를 들고 있는 경우 여기서 바로 초기화될 수 있음
	// Player는 아직 PlayerState ASC가 준비 전일 수 있으므로, 준비 안 되었으면 내부에서 pending 처리됨
	InitializeAttributesIfReady();
}

void ABaseCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	// Player/AI 모두 Possess 이후 ASC 준비가 끝난 시점일 가능성이 높으므로 재시도
	InitializeAttributesIfReady();
}

void ABaseCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	// 클라에서 PlayerState가 복제된 뒤 PlayerState ASC를 찾을 수 있게 되는 지점
	InitializeAttributesIfReady();
}

void ABaseCharacter::SetTeamTag(FGameplayTag NewTeamTag)
{
	TeamTag = NewTeamTag;
}

bool ABaseCharacter::HasTeamTag(FGameplayTag QueryTag) const
{
	return TeamTag.IsValid() && TeamTag.MatchesTag(QueryTag);
}

void ABaseCharacter::Die()
{
	HandleDeath();
}

void ABaseCharacter::HandleDeath_Implementation()
{
	if (bIsDead)
	{
		return;
	}

	bIsDead = true;

	// 일단 정지처리만 구현
	if (UCharacterMovementComponent* MoveComp = Cast<UCharacterMovementComponent>(GetCharacterMovement()))
	{
		MoveComp->DisableMovement();
	}
}

UAbilitySystemComponent* ABaseCharacter::GetCharacterAbilitySystemComponent() const
{
	// 1) 자기 자신에 ASC가 붙은 경우 (주로 Enemy)
	UAbilitySystemComponent* FoundASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(this);
	if (FoundASC)
	{
		return FoundASC;
	}

	// 2) PlayerState에 ASC가 붙은 경우 (주로 Player)
	const APawn* OwnerPawn = Cast<APawn>(this);
	if (!OwnerPawn)
	{
		return nullptr;
	}

	APlayerState* PS = OwnerPawn->GetPlayerState();
	if (!PS)
	{
		return nullptr;
	}

	return UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(PS);
}

void ABaseCharacter::InitializeAttributesIfReady()
{
	if (bAttributesInitialized)
	{
		return;
	}

	if (!AttributeInitDataTable)
	{
		// Debug::Print(
		// 	TEXT("[BaseCharacter] InitializeAttributesIfReady failed | AttributeInitDataTable is null"),
		// 	FColor::Red
		// );
		return;
	}

	if (!UnitTag.IsValid())
	{
		// Debug::Print(FString::Printf(
		// 	TEXT("[BaseCharacter] InitializeAttributesIfReady failed | UnitTag invalid | Owner=%s"),
		// 	*GetNameSafe(this)
		// ), FColor::Red);
		return;
	}

	UAbilitySystemComponent* ASC = GetCharacterAbilitySystemComponent();
	if (!ASC)
	{
		// Debug::Print(FString::Printf(
		// 	TEXT("[BaseCharacter] InitializeAttributesIfReady pending | ASC not ready | Owner=%s"),
		// 	*GetNameSafe(this)
		// ), FColor::Yellow);
		return;
	}

	if (ApplyInitialAttributesFromDataTable())
	{
		bAttributesInitialized = true;

		// Debug::Print(FString::Printf(
		// 	TEXT("[BaseCharacter] Attributes initialized | Owner=%s | UnitTag=%s"),
		// 	*GetNameSafe(this),
		// 	*UnitTag.ToString()
		// ), FColor::Green);
	}
}

bool ABaseCharacter::ApplyInitialAttributesFromDataTable()
{
	if (!AttributeInitDataTable)
	{
		return false;
	}

	UAbilitySystemComponent* ASC = GetCharacterAbilitySystemComponent();
	if (!ASC)
	{
		return false;
	}

	UHOGAttributeSet* AttributeSet = const_cast<UHOGAttributeSet*>(ASC->GetSet<UHOGAttributeSet>());
	if (!AttributeSet)
	{
		// Debug::Print(FString::Printf(
		// 	TEXT("[BaseCharacter] ApplyInitialAttributesFromDataTable failed | AttributeSet null | Owner=%s"),
		// 	*GetNameSafe(this)
		// ), FColor::Red);
		return false;
	}

	static const FString ContextString(TEXT("AttributeInitLookup"));

	const TArray<FName> RowNames = AttributeInitDataTable->GetRowNames();

	for (const FName& RowName : RowNames)
	{
		const FDT_AttributeRow* Row = AttributeInitDataTable->FindRow<FDT_AttributeRow>(RowName, ContextString);
		if (!Row)
		{
			continue;
		}

		if (Row->UnitTag != UnitTag)
		{
			continue;
		}

		ASC->SetNumericAttributeBase(UHOGAttributeSet::GetMaxHealthAttribute(), Row->MaxHealth);
		ASC->SetNumericAttributeBase(UHOGAttributeSet::GetHealthAttribute(), FMath::Clamp(Row->Health, 0.0f, Row->MaxHealth));
		ASC->SetNumericAttributeBase(UHOGAttributeSet::GetAttackPowerAttribute(), Row->AttackPower);

		// Debug::Print(FString::Printf(
		// 	TEXT("[BaseCharacter] ApplyInitialAttributesFromDataTable success | Owner=%s | Row=%s | HP=%.1f | MaxHP=%.1f | AP=%.1f"),
		// 	*GetNameSafe(this),
		// 	*RowName.ToString(),
		// 	Row->Health,
		// 	Row->MaxHealth,
		// 	Row->AttackPower
		// ), FColor::Cyan);

		return true;
	}

	// Debug::Print(FString::Printf(
	// 	TEXT("[BaseCharacter] ApplyInitialAttributesFromDataTable failed | Matching UnitTag row not found | Owner=%s | UnitTag=%s"),
	// 	*GetNameSafe(this),
	// 	*UnitTag.ToString()
	// ), FColor::Red);

	return false;
}