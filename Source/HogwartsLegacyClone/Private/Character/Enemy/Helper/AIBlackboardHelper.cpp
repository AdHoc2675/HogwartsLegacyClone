// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/Enemy/Helper/AIBlackboardHelper.h"
#include "BehaviorTree/BlackboardComponent.h"

AActor* UAIBlackboardHelper::GetTargetActor(UBlackboardComponent* BB)
{
	if (!BB) return nullptr;
	return Cast<AActor>(BB->GetValueAsObject(KEY_TargetActor));
}

void UAIBlackboardHelper::SetTargetActor(UBlackboardComponent* BB, AActor* Actor)
{
	if (BB)
	{
		BB->SetValueAsObject(KEY_TargetActor, Actor);
	}
}

void UAIBlackboardHelper::ClearTargetActor(UBlackboardComponent* BB)
{
	if (BB) 
	{
		BB->ClearValue(KEY_TargetActor);
	}
}

float UAIBlackboardHelper::GetTargetDistance(UBlackboardComponent* BB)
{
	if (!BB) return MAX_FLT;
	return BB->GetValueAsFloat(KEY_TargetDistance);
}

void UAIBlackboardHelper::SetTargetDistance(UBlackboardComponent* BB, float Distance)
{
	if (BB)
	{
		BB->SetValueAsFloat(KEY_TargetDistance, Distance);
	}
}

FName UAIBlackboardHelper::GetAbilityTagName(UBlackboardComponent* BB)
{
	if (!BB) return NAME_None;
	return BB->GetValueAsName(KEY_AbilityTag);
}

void UAIBlackboardHelper::SetAbilityTagName(UBlackboardComponent* BB, FName TagName)
{
	if (BB)
	{
		BB->SetValueAsName(KEY_AbilityTag, TagName);
	}
}

bool UAIBlackboardHelper::GetChaseDelay(UBlackboardComponent* BB)
{
	if (!BB) return false;
	return BB->GetValueAsBool(KEY_ChaseDelay);
}

void UAIBlackboardHelper::SetChaseDelay(UBlackboardComponent* BB, bool bInDelay)
{
	if (BB)
	{
		BB->SetValueAsBool(KEY_ChaseDelay, bInDelay);
	}
}
