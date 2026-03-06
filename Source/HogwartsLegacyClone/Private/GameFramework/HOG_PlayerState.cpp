// Fill out your copyright notice in the Description page of Project Settings.


#include "GameFramework/HOG_PlayerState.h"

#include "GAS/HOGAbilitySystemComponent.h"
#include "GAS/Attributes/HOGAttributeSet.h"
#include "Data/DA_AbilitySet.h"
#include "HOGDebugHelper.h"

AHOG_PlayerState::AHOG_PlayerState()
{
	//ASC 생성하기
	AbilitySystemComponent=CreateDefaultSubobject<UHOGAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);
	
	//Attribute 생성
	AttributeSet=CreateDefaultSubobject<UHOGAttributeSet>(TEXT("AttributeSet"));
	
}

void AHOG_PlayerState::BeginPlay()
{
	Super::BeginPlay();
	
	Debug::Print(FString::Printf(
		TEXT("[PlayerState] BeginPlay | HasAuthority=%d | ASC=%s | AbilitySet=%s"),
		HasAuthority() ? 1 : 0,
		*GetNameSafe(AbilitySystemComponent),
		*GetNameSafe(AbilitySet)
	), FColor::Yellow);

	
	if (HasAuthority()&&AbilitySet&&AbilitySystemComponent)
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


