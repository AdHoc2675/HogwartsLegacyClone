#pragma once

#include "CoreMinimal.h"
#include "GAS/Abilities/GA_SpellBase.h"
#include "GA_Spell_Accio.generated.h"

class UAnimMontage;
class USoundBase;
class UNiagaraSystem;
class UAudioComponent;
class UNiagaraComponent;
class UAbilitySystemComponent;
class USceneComponent;

UCLASS()
class HOGWARTSLEGACYCLONE_API UGA_Spell_Accio : public UGA_SpellBase
{
	GENERATED_BODY()

public:
	UGA_Spell_Accio();

protected:
	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData
	) override;

	virtual void EndAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled
	) override;

	// 토글 오프를 위해 입력 감지 함수 오버라이드
	virtual void InputPressed(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo
	) override;

	UFUNCTION()
	void OnMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	// 마법 시전 핵심 로직 (대상을 성공적으로 당기기 시작하면 true 반환)
	UFUNCTION(BlueprintCallable, Category = "HOG|Spell|Accio")
	bool FireAccio();

	// 타이머 루프: 지속적으로 끌어당기기 연산
	void UpdatePulling();

protected:
	// ====== Accio 설정 ======
	UPROPERTY(EditDefaultsOnly, Category = "HOG|Spell|Accio|Anim")
	TObjectPtr<UAnimMontage> CastMontage = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "HOG|Spell|Accio|Anim")
	TObjectPtr<UAnimMontage> HoldLoopMontage = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "HOG|Spell|Accio|Anim")
	FName StartSectionName = TEXT("Start");

	UPROPERTY(EditDefaultsOnly, Category = "HOG|Spell|Accio|Anim")
	FName HoldSectionName = TEXT("Hold");

	UPROPERTY(EditDefaultsOnly, Category = "HOG|Spell|Accio|Anim")
	FName EndSectionName = TEXT("End");

	UPROPERTY(EditDefaultsOnly, Category = "HOG|Spell|Accio|Sound")
	TObjectPtr<USoundBase> CastVoiceSound = nullptr;

	// 마법 시전 시 나는 마법 이펙트 사운드
	UPROPERTY(EditDefaultsOnly, Category = "HOG|Spell|Accio|Sound")
	TObjectPtr<USoundBase> CastSound = nullptr;

	// 대상을 당기는 동안 나는 루프 사운드
	UPROPERTY(EditDefaultsOnly, Category = "HOG|Spell|Accio|Sound")
	TObjectPtr<USoundBase> PullSound = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "HOG|Spell|Accio|Sound")
	FString CastSubtitleText = TEXT("아씨오!");

	// 진행 중인 당기기 사운드를 끄기 위한 컴포넌트
	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> PullAudioComponent = nullptr;

	// 지속형 빔 Accio VFX
	UPROPERTY(EditDefaultsOnly, Category = "HOG|Spell|Accio|Visual")
	TObjectPtr<UNiagaraSystem> AccioVFX = nullptr;

	// 빔 시작 소켓
	UPROPERTY(EditDefaultsOnly, Category = "HOG|Spell|Accio|Visual")
	FName BeamStartSocketName = TEXT("RightHandWandSocket");

	// Niagara 파라미터 이름
	UPROPERTY(EditDefaultsOnly, Category = "HOG|Spell|Accio|Visual")
	FName BeamStartParamName = TEXT("BeamStart");

	UPROPERTY(EditDefaultsOnly, Category = "HOG|Spell|Accio|Visual")
	FName BeamEndParamName = TEXT("BeamEnd");

	UPROPERTY(EditDefaultsOnly, Category = "HOG|Spell|Accio|Visual")
	FName BeamLengthParamName = TEXT("BeamLength");

	// 대상에 따른 끌어당기기 속도 세분화
	UPROPERTY(EditDefaultsOnly, Category = "HOG|Spell|Accio|Move")
	float EnemyPullSpeed = 3000.f;

	UPROPERTY(EditDefaultsOnly, Category = "HOG|Spell|Accio|Move")
	float InteractablePullSpeed = 500.f;

	UPROPERTY(EditDefaultsOnly, Category = "HOG|Spell|Accio|Move")
	float DefaultPullSpeed = 1000.f;

	UPROPERTY(EditDefaultsOnly, Category = "HOG|Spell|Accio|Move")
	float StopDistance = 150.f;

protected:
	// ====== 런타임 상태 ======
	UPROPERTY(Transient)
	TObjectPtr<AActor> OriginalTarget = nullptr; // 마법을 맞춘 대상(이펙트 기준점용)

	UPROPERTY(Transient)
	TObjectPtr<AActor> TargetToMove = nullptr; // 실제로 이동할 대상 (적재물, 발판 등)

	UPROPERTY(Transient)
	TObjectPtr<AActor> PullDestination = nullptr; // 도착 지점 (보통 플레이어지만, 타겟 지점일 수 있음)

	FTimerHandle PullTimerHandle;

	// 현재 타겟이 상호작용 가능한 물체인지 여부 (토글식 작동 판단용)
	bool bIsPullingInteractable = false;

	// 현재 적용 중인 당기기 속도
	float CurrentPullSpeed = 0.f;

	// Notify 중복 호출 방지
	UPROPERTY(Transient)
	bool bCastNotifyHandled = false;

	// 지속형 빔 컴포넌트
	UPROPERTY(Transient)
	TObjectPtr<UNiagaraComponent> ActiveBeamVFXComponent = nullptr;

	UPROPERTY(Transient)
	bool bPendingMontageEndTransition = false;

	UPROPERTY(Transient)
	bool bPendingEndAbilityReplicate = false;

	UPROPERTY(Transient)
	bool bPendingEndAbilityWasCancelled = false;

	UPROPERTY(Transient)
	bool bIgnoreNextCastMontageInterrupted = false;

protected:
	virtual void OnPreCastFacingFinished(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData
	) override;

	void ExecuteAccioCast(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData
	);

protected:
	virtual void HandleCastNotify() override;

protected:
	bool SpawnPersistentBeamVFX();
	void UpdatePersistentBeamVFX();
	void ClearPersistentBeamVFX();

	bool GetCurrentBeamStartLocation(FVector& OutStartLocation) const;
	bool GetCurrentBeamEndLocation(FVector& OutEndLocation) const;
	
	AActor* ResolveCurrentBeamTargetActor() const;
	UAbilitySystemComponent* GetBeamTargetASC(AActor* TargetActor) const;
	USceneComponent* GetBeamEndAttachComponent(AActor* TargetActor) const;	

	bool TryJumpMontageToSection(FName SectionName) const;
	void BeginMontageEndTransition(bool bReplicateEndAbility, bool bWasCancelled);
	void FinishAccioAbilityEnd(bool bReplicateEndAbility, bool bWasCancelled);
};
