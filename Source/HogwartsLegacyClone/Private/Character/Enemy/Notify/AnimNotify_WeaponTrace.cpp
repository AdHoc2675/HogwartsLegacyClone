// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/Enemy/Notify/AnimNotify_WeaponTrace.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "Character/Enemy/EnemyCharacterBase.h"
#include "Component/WeaponComponent.h"
#include "Core/HOG_GameplayTags.h"

void UAnimNotify_WeaponTrace::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
                                          float TotalDuration)
{
	HitActors.Empty();
    
	// 캐싱도 매번 새로 하도록
	Enemy = Cast<AEnemyCharacterBase>(MeshComp->GetOwner());
	Weapon = Enemy.IsValid() 
		? Enemy->FindComponentByClass<UWeaponComponent>() 
		: nullptr;
}

void UAnimNotify_WeaponTrace::NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
                                         float FrameDeltaTime)
{
	DoTrace(MeshComp);
}

void UAnimNotify_WeaponTrace::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	HitActors.Empty();
}

void UAnimNotify_WeaponTrace::DoTrace(USkeletalMeshComponent* MeshComp)
{
	if (!Enemy.IsValid() || !Weapon.IsValid()) return;

	FVector Start = Weapon->GetSocketLocation(Weapon->StartSocketName);
	FVector End = Weapon->GetSocketLocation(Weapon->EndSocketName);

	TArray<FHitResult> HitResult;
	FCollisionQueryParams CollisionParams;
	CollisionParams.AddIgnoredActor(Enemy.Get());

	Enemy->GetWorld()->SweepMultiByChannel(HitResult, Start, End, FQuat::Identity, ECC_Pawn,
	                                       FCollisionShape::MakeSphere(TraceRadius), CollisionParams);
	for (const FHitResult& Hit : HitResult)
	{
		AActor* HitActor = Hit.GetActor();
		if (!HitActor) continue;

		// 적끼리는 무시
		ABaseCharacter* HitCharacter = Cast<ABaseCharacter>(HitActor);
		if (!HitCharacter) continue;
		if (!HitCharacter->HasTeamTag(HOGGameplayTags::Team_Player)) continue;

		// 중복 히트 방지
		if (HitActors.Contains(HitActor)) continue;
		HitActors.Add(HitActor);

		FGameplayEventData Payload;
		Payload.Target = HitActor;

		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(
			Enemy.Get(),
			HOGGameplayTags::Event_Weapon_Hit,
			Payload);
	}
}
