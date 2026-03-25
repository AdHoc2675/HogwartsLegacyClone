// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/Enemy/Handler/EnemyDamageHandler.h"

#include "Core/HOG_GameplayTags.h"
#include "AbilitySystemComponent.h"
#include "Character/Enemy/EnemyCharacterBase.h"
#include "GameFramework/HOG_PlayerController.h"
#include "Pool/DamageNumberPool.h"

void UEnemyDamageHandler::Initialize(AEnemyCharacterBase* InOwner)
{
    OwnerEnemy = InOwner;
    
    if (OwnerEnemy.IsValid())
    {
        OwnerEnemy->OnEnemyDamaged.AddUObject(this, &UEnemyDamageHandler::HandleHitReact);
        OwnerEnemy->OnEnemyDamaged.AddUObject(this, &UEnemyDamageHandler::SpawnDamageNumber);
    }
}

void UEnemyDamageHandler::Shutdown()
{
    if (OwnerEnemy.IsValid())
    {
        OwnerEnemy->OnEnemyDamaged.RemoveAll(this);
    }
    
    OwnerEnemy.Reset();
    DamageNumberPool.Reset();
}

void UEnemyDamageHandler::HandleHitReact(float Damage)
{
    if (!OwnerEnemy.IsValid()) return;
    
    UAbilitySystemComponent* ASC = OwnerEnemy->GetAbilitySystemComponent();
    if (!ASC) return;
    
    FGameplayTagContainer HitReactTag;
    HitReactTag.AddTag(HOGGameplayTags::State_Hit);
    ASC->TryActivateAbilitiesByTag(HitReactTag);
}

void UEnemyDamageHandler::SpawnDamageNumber(float Damage)
{
    if (!OwnerEnemy.IsValid()) return;
    
    if (!DamageNumberPool.IsValid())
    {
        UWorld* World = OwnerEnemy->GetWorld();
        if (!World) return;
        
        AHOG_PlayerController* PC = Cast<AHOG_PlayerController>(World->GetFirstPlayerController());
        if (!PC) return;
        
        DamageNumberPool = PC->GetDamageNumberPool();
    }
    
    if (!DamageNumberPool.IsValid()) return;
    
    USkeletalMeshComponent* Mesh = OwnerEnemy->GetMesh();
    if (!Mesh) return;
    
    float MeshHeight = Mesh->Bounds.BoxExtent.Z;
    float BaseHeight = MeshHeight * 0.5f;
    
    double CurrentTime = OwnerEnemy->GetWorld()->GetTimeSeconds();
    
    if (CurrentTime - LastDamageNumberTime < 1.0f)
    {
        LastDamageNumberZ += DamageNumberSpacing;
        
        if (LastDamageNumberZ > DamageNumberSpacing * 3.f)
        {
            LastDamageNumberZ = 0.f;
        }
    }
    else
    {
        LastDamageNumberZ = 0.f;
    }
    
    LastDamageNumberTime = CurrentTime;
    
    FVector Offset = FVector(FMath::RandRange(-30.f, 30.f), 0.f, BaseHeight + LastDamageNumberZ);
    FVector WorldLocation = Mesh->Bounds.Origin + Offset;
    
    DamageNumberPool->ShowDamage(Damage, WorldLocation);
}