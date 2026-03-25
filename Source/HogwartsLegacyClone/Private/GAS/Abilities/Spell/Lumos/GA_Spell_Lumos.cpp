// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/Abilities/Spell/Lumos/GA_Spell_Lumos.h"
#include "HOGDebugHelper.h"
#include "Core/HOG_GameplayTags.h"

#include "GameFramework/Character.h"
#include "Character/Player/PlayerCharacterBase.h"
#include "Animation/AnimInstance.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"

#include "GameFramework/HOG_PlayerController.h"
#include "UI/HOG_WidgetController.h"

UGA_Spell_Lumos::UGA_Spell_Lumos()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

void UGA_Spell_Lumos::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	bIsActiveLumos = true;
	APlayerCharacterBase* PlayerCharacter = Cast<APlayerCharacterBase>(ActorInfo->AvatarActor.Get());

	if (PlayerCharacter)
	{
		if (CastVoiceSound)
		{
			UGameplayStatics::PlaySoundAtLocation(this, CastVoiceSound, PlayerCharacter->GetActorLocation());
            
			// =============== [자막 호출] ===============
			if (AHOG_PlayerController* PC = Cast<AHOG_PlayerController>(PlayerCharacter->GetController()))
			{
				if (UHOG_WidgetController* UIController = PC->GetWidgetController())
				{
					UIController->RequestSubtitle(CastSubtitleText, 1.0f);
				}
			}
		}
		if (CastSound)
		{
			UGameplayStatics::PlaySoundAtLocation(this, CastSound, PlayerCharacter->GetActorLocation());
		}
	}
	
	// 시전할 때 마법 지팡이(무기) 꺼내기 판정 유지
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		ASC->AddLooseGameplayTag(HOGGameplayTags::State_Combat_Active);
	}

	// 1. Lumos_Start 애니메이션 플레이
	if (PlayerCharacter && PlayerCharacter->GetMesh() && CastMontage)
	{
		UAnimInstance* AnimInstance = PlayerCharacter->GetMesh()->GetAnimInstance();
		if (AnimInstance)
		{
			const float Duration = AnimInstance->Montage_Play(CastMontage, 2.5f);
			if (Duration > 0.f)
			{
				FOnMontageEnded EndDelegate;
				EndDelegate.BindUObject(this, &UGA_Spell_Lumos::OnStartMontageEnded);
				AnimInstance->Montage_SetEndDelegate(EndDelegate, CastMontage);
			}
			else
			{
				OnStartMontageEnded(CastMontage, false);
			}
		}
	}
	else
	{
		// 몽타주가 없으면 바로 빛나는 상태로 넘어감
		OnStartMontageEnded(nullptr, false);
	}

	// 2. 광원 생성 (머리 위 메인 조명 + 지팡이 끝 보조 조명/VFX)
	if (PlayerCharacter)
	{
		// --- [1] 머리 위 메인 조명 (주변을 밝히고 그림자 생성) ---
		UCapsuleComponent* PlayerCapsule = PlayerCharacter->GetCapsuleComponent();
		
		if (PlayerCapsule)
		{
			// 광원 생성
			SpawnedLight = NewObject<UPointLightComponent>(PlayerCharacter);
			if (SpawnedLight)
			{
				SpawnedLight->RegisterComponent();
				// 캡슐에 부착. (회전할 때 광원이 같이 돌게끔)
				SpawnedLight->AttachToComponent(PlayerCapsule, FAttachmentTransformRules::KeepRelativeTransform);

				// ⭐ 캐릭터 중심(루트) 기준으로 "약간 우측(Y), 약간 전방/위(Z)"에 위치하도록 설정
				SpawnedLight->SetRelativeLocation(FVector(0.f, 60.f, 180.f));

				SpawnedLight->SetLightColor(LightColor);
				SpawnedLight->SetIntensity(LightIntensity);
				SpawnedLight->SetAttenuationRadius(LightAttenuationRadius);
				SpawnedLight->SetCastShadows(true);
			}

			if (LumosVFX)
			{
				SpawnedVFX = UNiagaraFunctionLibrary::SpawnSystemAttached(
					LumosVFX, PlayerCapsule, NAME_None,
					FVector(0.f, 60.f, 180.f), FRotator::ZeroRotator,
					EAttachLocation::KeepRelativeOffset, true 
				);
			}
		}

		// --- [2] 지팡이 끝 보조 조명 & VFX (연출용) ---
		UStaticMeshComponent* WandMesh = Cast<UStaticMeshComponent>(PlayerCharacter->GetDefaultSubobjectByName(TEXT("WandMesh")));

		if (WandMesh)
		{
			FName AttachSocketName = TEXT("TipSocket");

			WandLight = NewObject<UPointLightComponent>(PlayerCharacter);
			if (WandLight)
			{
				WandLight->RegisterComponent();
				WandLight->AttachToComponent(WandMesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, AttachSocketName);

				WandLight->SetLightColor(LightColor);
				// 보조 조명이 너무 밝으면 눈이 부시므로 세기를 살짝 줄임
				WandLight->SetIntensity(LightIntensity * 0.01f); 
				WandLight->SetAttenuationRadius(LightAttenuationRadius * 0.01f);
				// 그림자가 2개 생기면 부자연스럽고 최적화에 안 좋으므로 보조 조명은 그림자 끔
				WandLight->SetCastShadows(false); 
			}

			if (LumosVFX)
			{
				WandVFX = UNiagaraFunctionLibrary::SpawnSystemAttached(
					LumosVFX, WandMesh, AttachSocketName,
					FVector::ZeroVector, FRotator::ZeroRotator,
					EAttachLocation::SnapToTarget, true 
				);
			}
		}
	}
}

// ========= 1. Start 재생 끝 ========
void UGA_Spell_Lumos::OnStartMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	// 시작 모션이 끝났으니, 이제 플레이어는 Hold 자세로 움직일 수 있음
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		// 이 캐스팅 관련 태그를 빼주어야 `Input_Move` 에서 차단을 안당하고 걸을 수 있음.
		ASC->RemoveLooseGameplayTag(HOGGameplayTags::State_Casting_Active);

		// ABP에게 상체를 Lumos_Hold 상태로 만들도록 알려줌
		ASC->AddLooseGameplayTag(FGameplayTag::RequestGameplayTag(FName("State.Spell.Lumos.Active")));
	}
}

// ========= 2. 토글 버튼 다시 누름 (Lumos 끄기) ========
void UGA_Spell_Lumos::InputPressed(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo)
{
	Super::InputPressed(Handle, ActorInfo, ActivationInfo);

	// 토글 버튼을 눌렀을 때
	if (bIsActiveLumos)
	{
		bIsActiveLumos = false;

		// 마법 상태를 끝내므로 먼저 애니메이션 홀드를 품
		if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
		{
			ASC->RemoveLooseGameplayTag(FGameplayTag::RequestGameplayTag(FName("State.Spell.Lumos.Active")));
			
			// 끄는 모션 도중에는 움직이면 안된다면 다시 잠깐 막기 (선택 사항)
			// ASC->AddLooseGameplayTag(HOGGameplayTags::State_Casting_Active); 
		}

		APlayerCharacterBase* PlayerCharacter = Cast<APlayerCharacterBase>(ActorInfo->AvatarActor.Get());
		
		// 3. Lumos_Stop 애니메이션 플레이
		if (PlayerCharacter && PlayerCharacter->GetMesh() && StopMontage)
		{
			UAnimInstance* AnimInstance = PlayerCharacter->GetMesh()->GetAnimInstance();
			if (AnimInstance)
			{
				const float Duration = AnimInstance->Montage_Play(StopMontage, 2.5f);
				if (Duration > 0.f)
				{
					FOnMontageEnded EndDelegate;
					EndDelegate.BindUObject(this, &UGA_Spell_Lumos::OnStopMontageEnded);
					AnimInstance->Montage_SetEndDelegate(EndDelegate, StopMontage);
					return; // Stop 애니메이션이 끝날 때 완전히 종료하도록 리턴
				}
			}
		}

		// Stop 몽타주가 없거나 실패 시 즉시 종료
		OnStopMontageEnded(nullptr, false);
	}
}

// ========= 3. Stop 재생 끝 -> 완전 종료 ========
void UGA_Spell_Lumos::OnStopMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, bInterrupted);
}


void UGA_Spell_Lumos::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	bIsActiveLumos = false;

	APlayerCharacterBase* PlayerCharacter = Cast<APlayerCharacterBase>(ActorInfo->AvatarActor.Get());

	if (PlayerCharacter)
	{
		if (EndVoiceSound)
		{
			UGameplayStatics::PlaySoundAtLocation(this, EndVoiceSound, PlayerCharacter->GetActorLocation());
		}
		if (EndSound)
		{
			UGameplayStatics::PlaySoundAtLocation(this, EndSound, PlayerCharacter->GetActorLocation());
		}
	}

	// 어빌리티 종료 시 추가했던 모든 관련 태그 정리
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		ASC->RemoveLooseGameplayTag(HOGGameplayTags::State_Combat_Active);
		ASC->RemoveLooseGameplayTag(FGameplayTag::RequestGameplayTag(FName("State.Spell.Lumos.Active")));
	}

	// 능력이 꺼질 때 깔끔하게 빛 광원 삭제
	if (SpawnedLight)
	{
		SpawnedLight->DestroyComponent();
		SpawnedLight = nullptr;
	}

	if (SpawnedVFX)
	{
		SpawnedVFX->DestroyComponent();
		SpawnedVFX = nullptr;
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

