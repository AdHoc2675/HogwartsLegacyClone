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

	if (CastVoiceSound && PlayerCharacter)
	{
		UGameplayStatics::PlaySoundAtLocation(this, CastVoiceSound, PlayerCharacter->GetActorLocation());
	}

	// 1. 애니메이션 플레이
	if (PlayerCharacter && PlayerCharacter->GetMesh() && CastMontage)
	{
		UAnimInstance* AnimInstance = PlayerCharacter->GetMesh()->GetAnimInstance();
		if (AnimInstance)
		{
			const float Duration = AnimInstance->Montage_Play(CastMontage, 1.0f);
			if (Duration > 0.f)
			{
				FOnMontageEnded EndDelegate;
				EndDelegate.BindUObject(this, &UGA_Spell_Lumos::OnMontageEnded);
				AnimInstance->Montage_SetEndDelegate(EndDelegate, CastMontage);
			}
		}
	}

	// 2. PlayerCharacter에서 지팡이(WandMesh)를 찾아 빛 부착
	if (PlayerCharacter)
	{
		// 외부 의존성을 최소화하기 위해 객체 이름으로 기본 컴포넌트를 찾아옴
		UStaticMeshComponent* WandMesh = Cast<UStaticMeshComponent>(PlayerCharacter->GetDefaultSubobjectByName(TEXT("WandMesh")));

		if (WandMesh)
		{
			// 설정해 둔 소켓(TipSocket)이 있다면 사용하고, 없다면 메시 중심에 부착
			FName AttachSocketName = TEXT("TipSocket");

			SpawnedLight = NewObject<UPointLightComponent>(PlayerCharacter);
			if (SpawnedLight)
			{
				SpawnedLight->RegisterComponent();
				SpawnedLight->AttachToComponent(WandMesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, AttachSocketName);

				SpawnedLight->SetLightColor(LightColor);
				SpawnedLight->SetIntensity(LightIntensity);
				SpawnedLight->SetAttenuationRadius(LightAttenuationRadius);
				SpawnedLight->SetCastShadows(true);
			}

			if (LumosVFX)
			{
				SpawnedVFX = UNiagaraFunctionLibrary::SpawnSystemAttached(
					LumosVFX,
					WandMesh,
					AttachSocketName,
					FVector::ZeroVector,
					FRotator::ZeroRotator,
					EAttachLocation::SnapToTarget,
					true 
				);
			}
		}
	}
}

void UGA_Spell_Lumos::InputPressed(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo)
{
	Super::InputPressed(Handle, ActorInfo, ActivationInfo);

	// 다시 동일한 입력이 발생하면 강제로 불을 끄고 어빌리티 종료
	if (bIsActiveLumos)
	{
		Debug::Print(TEXT("[Lumos] Canceled by Toggle Input."), FColor::Yellow);
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
	}
}

void UGA_Spell_Lumos::OnMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	// 몽타주 종료 여부와 상관없이 마법(빛)은 유지되어야 하므로 아무 처리도 하지 않고 어빌리티를 계속 돌려둠
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

	if (EndVoiceSound && PlayerCharacter)
	{
		UGameplayStatics::PlaySoundAtLocation(this, EndVoiceSound, PlayerCharacter->GetActorLocation());
	}

	// 능력이 꺼질 때마다 깔끔하게 빛 광원 삭제
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

