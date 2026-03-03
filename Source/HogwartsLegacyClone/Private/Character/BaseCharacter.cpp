// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/BaseCharacter.h"

// Sets default values
ABaseCharacter::ABaseCharacter()
{
	// disable tick by default, enable it in child classes if needed
    PrimaryActorTick.bCanEverTick = false;
    PrimaryActorTick.bStartWithTickEnabled = false;

	// disable decals by default, enable it in child classes if needed
    GetMesh()->bReceivesDecals = false;
}


