#pragma once

#include "CoreMinimal.h"
#include "Character/BaseCharacter.h"
#include "GameplayTagContainer.h"
#include "InputActionValue.h"
#include "Core/HOG_Struct.h"
#include "PlayerCharacterBase.generated.h"

class USpringArmComponent;
class UCameraComponent;
class ULockOnComponent;
class UHOGAbilitySystemComponent;
class UStaticMeshComponent;
class USkeletalMeshComponent;
class UAbilitySystemComponent;
class UNiagaraSystem;
class UGA_SpellBase;


/**
 * 플레이어 캐릭터 베이스
 * - 카메라(SpringArm + Camera)
 * - 기본 이동/시점/점프 입력 처리 함수 (PlayerController가 호출)
 *
 * EnhancedInput 바인딩은 PlayerController에서 처리.
 */
UCLASS()
class HOGWARTSLEGACYCLONE_API APlayerCharacterBase : public ABaseCharacter
{
	GENERATED_BODY()

public:
	APlayerCharacterBase();

public:
	// INPUT
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void Input_Move(const FInputActionValue& Value);

	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void Input_Look(const FInputActionValue& Value);

	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void Input_JumpStarted();

	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void Input_JumpCompleted();

	UFUNCTION(BlueprintCallable, Category = "Input")
	virtual void Input_Interact();

	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void Input_AbilityInputPressed(FGameplayTag InputTag);

	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void Input_AbilityInputReleased(FGameplayTag InputTag);

public:
	UFUNCTION(BlueprintPure, Category="Components")
	ULockOnComponent* GetLockOnComponent() const { return LockOnComponent; }

	UFUNCTION(BlueprintPure, Category="GAS")
	UHOGAbilitySystemComponent* GetHOGAbilitySystemComponent() const;

	UFUNCTION(BlueprintCallable, Category="HOG|Combat")
	UStaticMeshComponent* GetWandMesh() const { return WandMesh; }

public:
	// =========================
	// Spell Facing
	// =========================
	UFUNCTION(BlueprintCallable, Category="HOG|Look")
	void BeginForcedFacingToLocation(const FVector& TargetLocation);

	UFUNCTION(BlueprintCallable, Category="HOG|Look")
	void BeginForcedFacingToRotation(const FRotator& TargetRotation);

	UFUNCTION(BlueprintCallable, Category="HOG|Look")
	void StopForcedFacing();

	UFUNCTION(BlueprintPure, Category="HOG|Look")
	bool IsForcedFacingActive() const { return bForcedFacingActive; }

	UFUNCTION(BlueprintPure, Category="HOG|Look")
	bool IsForcedFacingFinished() const;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void OnRep_PlayerState() override;

	void InitializeAbilityActorInfo();
	void UpdateForcedFacing(float DeltaSeconds);
	void BindASCGameplayTagCallbacks();

#pragma region Camera

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Camera", meta=(AllowPrivateAccess="true"))
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Camera", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UCameraComponent> FollowCamera;
#pragma endregion

#pragma region Tuning

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="HOG|Move")
	float MoveForwardScale = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="HOG|Move")
	float MoveRightScale = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="HOG|Look")
	float LookYawScale = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="HOG|Look")
	float LookPitchScale = 1.0f;

	// 시전 직전 강제 회전 속도
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="HOG|Look")
	float ForcedFacingInterpSpeed = 25.0f;

	// 이 각도 안으로 들어오면 회전 완료로 판정
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="HOG|Look")
	float ForcedFacingAcceptAngle = 5.0f;
#pragma endregion

protected:
	// Capsule
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="HOG|Capsule")
	float CapsuleRadius = 34.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="HOG|Capsule")
	float CapsuleHalfHeight = 88.f;

	// Mesh Offset (캡슐 기준)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="HOG|Mesh")
	FVector MeshRelativeLocation = FVector(0.f, 0.f, -90.f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="HOG|Mesh")
	FRotator MeshRelativeRotation = FRotator(0.f, -90.f, 0.f);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Component", meta=(AllowPrivateAccess="true"))
	TObjectPtr<ULockOnComponent> LockOnComponent;

protected:
	// =========================
	// Forced Facing Runtime
	// =========================
	UPROPERTY(Transient)
	bool bForcedFacingActive = false;

	UPROPERTY(Transient)
	FRotator ForcedFacingTargetRotation = FRotator::ZeroRotator;

#pragma region Wand

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="HOG|Combat", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UStaticMeshComponent> WandMesh;

protected:
	bool bCombatActive = false;
	bool bCastingActive = false;

	UFUNCTION()
	void HandleCombatActiveTagChanged(const FGameplayTag CallbackTag, int32 NewCount);

	UFUNCTION()
	void HandleCombatInactiveTagChanged(const FGameplayTag CallbackTag, int32 NewCount);

	UFUNCTION()
	void HandleCastingActiveTagChanged(const FGameplayTag CallbackTag, int32 NewCount);

	UFUNCTION()
	void HandleCastingInactiveTagChanged(const FGameplayTag CallbackTag, int32 NewCount);

	void RefreshWandVisibilityFromCombatState();
	void SetWandVisible(bool bVisible);
#pragma endregion

#pragma region Combo

protected:
	UPROPERTY(Transient, BlueprintReadOnly, Category="HOG|Combat")
	bool bCanQueueNextCombo = false;

public:
	UFUNCTION(BlueprintCallable, Category="HOG|Combat")
	void SetCanQueueNextCombo(bool bInCanQueue);

	UFUNCTION(BlueprintPure, Category="HOG|Combat")
	bool CanQueueNextCombo() const { return bCanQueueNextCombo; }

#pragma endregion

public:
	UFUNCTION(BlueprintCallable, Category="Spell|VFX")
	void QueueSpellVFX(const FQueuedSpellVFXData& InVFXData);

	UFUNCTION(BlueprintCallable, Category="Spell|VFX")
	bool ConsumeAndSpawnQueuedSpellVFX();

protected:
	UPROPERTY(Transient)
	FQueuedSpellVFXData QueuedSpellVFXData;

protected:
	UPROPERTY(Transient)
	TObjectPtr<UGA_SpellBase> QueuedCastNotifyAbility = nullptr;

public:
	UFUNCTION(BlueprintCallable, Category="Spell|VFX")
	void QueueCastNotifyAbility(UGA_SpellBase* InAbility);

	UFUNCTION(BlueprintCallable, Category="Spell|VFX")
	void ConsumeCastNotifyAbility();
};
