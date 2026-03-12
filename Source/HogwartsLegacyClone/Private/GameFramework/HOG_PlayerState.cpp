#include "GameFramework/HOG_PlayerState.h"

#include "GAS/HOGAbilitySystemComponent.h"
#include "GAS/Attributes/HOGAttributeSet.h"
#include "Data/DA_AbilitySet.h"
#include "Component/SpellComponent.h"
#include "HOGDebugHelper.h"

AHOG_PlayerState::AHOG_PlayerState()
{
	// ASC 생성
	AbilitySystemComponent = CreateDefaultSubobject<UHOGAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	// Attribute 생성
	AttributeSet = CreateDefaultSubobject<UHOGAttributeSet>(TEXT("AttributeSet"));

	// SpellComponent 생성
	SpellComponent = CreateDefaultSubobject<USpellComponent>(TEXT("SpellComponent"));
}

void AHOG_PlayerState::BeginPlay()
{
	Super::BeginPlay();

	// Debug::Print(FString::Printf(
	// 	TEXT("[PlayerState] BeginPlay | HasAuthority=%d | ASC=%s | AbilitySet=%s | SpellComponent=%s"),
	// 	HasAuthority() ? 1 : 0,
	// 	*GetNameSafe(AbilitySystemComponent),
	// 	*GetNameSafe(AbilitySet),
	// 	*GetNameSafe(SpellComponent)
	// ), FColor::Yellow);

	if (HasAuthority() && AbilitySet && AbilitySystemComponent)
	{
		AbilitySet->GiveAbilities(AbilitySystemComponent);
	}
}

UAbilitySystemComponent* AHOG_PlayerState::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

UHOGAbilitySystemComponent* AHOG_PlayerState::GetHOGAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

UHOGAttributeSet* AHOG_PlayerState::GetAttributeSet() const
{
	return AttributeSet;
}

USpellComponent* AHOG_PlayerState::GetSpellComponent() const
{
	return SpellComponent;
}