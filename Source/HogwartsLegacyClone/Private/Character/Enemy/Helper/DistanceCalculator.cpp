// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/Enemy/Helper/DistanceCalculator.h"

#include "Character/BaseCharacter.h"
#include "Character/Player/PlayerCharacterBase.h"
#include "Components/CapsuleComponent.h"

float UDistanceCalculator::Calculate(AActor* From, AActor* To)
{
	if (!From || !To) return MAX_FLT;
	
	float Distance = FVector::Distance(From->GetActorLocation(), To->GetActorLocation());
	
	if (const ABaseCharacter* FromCharacter = Cast<ABaseCharacter>(From))
	{
		if (UCapsuleComponent* Capsule = FromCharacter->GetCapsuleComponent())
		{
			Distance -= Capsule->GetScaledCapsuleRadius();
		}
	}
	
	if (const ACharacter* ToChar = Cast<ACharacter>(To))
	{
		if (UCapsuleComponent* Capsule = ToChar->GetCapsuleComponent())
		{
			Distance -= Capsule->GetScaledCapsuleRadius();
		}
	}
	
	return FMath::Max(0.f, Distance);
}
