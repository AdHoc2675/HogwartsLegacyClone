// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/Enemy/Anim/EnemyAnimInstanceBase.h"
#include "KismetAnimationLibrary.h"
#include "Core/HOG_GameplayTags.h"
#include "Character/Enemy/EnemyCharacterBase.h"

void UEnemyAnimInstanceBase::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();
	
	EnemyCharacter = Cast<AEnemyCharacterBase>(TryGetPawnOwner());
	
	if (EnemyCharacter)
	{
		AbilitySystemComponent = EnemyCharacter->GetAbilitySystemComponent();
	}
}

void UEnemyAnimInstanceBase::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);
	
	if (!EnemyCharacter) return;
	
	FVector Velocity = EnemyCharacter->GetVelocity();
	Speed = Velocity.Size2D();
	
	if (Speed > 0.f)
	{
		Direction = UKismetAnimationLibrary::CalculateDirection(Velocity, EnemyCharacter->GetActorRotation());
	}
	
	if (AbilitySystemComponent)
	{
		bIsDead = AbilitySystemComponent->HasMatchingGameplayTag(HOGGameplayTags::State_Dead);
	}
		
}
