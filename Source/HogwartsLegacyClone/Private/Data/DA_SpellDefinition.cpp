// Fill out your copyright notice in the Description page of Project Settings.


#include "Data/DA_SpellDefinition.h"

bool UDA_SpellDefinition::IsValidDefinition() const
{
	// 최소 조건: SpellID는 있어야 함
	return SpellID.IsValid();
}