// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Character/BaseCharacter.h"
#include "GameplayTagContainer.h"
#include "InputActionValue.h"
#include "PlayerCharacterBase.generated.h"


class USpringArmComponent;
class UCameraComponent;
class ULockOnComponent;
class UHOGAbilitySystemComponent;

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
	
	//INPUT 	
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void Input_Move(const FInputActionValue& Value);

	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void Input_Look(const FInputActionValue& Value);

	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void Input_JumpStarted();

	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void Input_JumpCompleted();

	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void Input_AbilityInputPressed(FGameplayTag InputTag);

	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void Input_AbilityInputReleased(FGameplayTag InputTag);
	
public:
	UFUNCTION(BlueprintPure, Category="Components")
	ULockOnComponent* GetLockOnComponent() const { return LockOnComponent; }
	
	UFUNCTION(BlueprintPure,Category="GAS")
	UHOGAbilitySystemComponent* GetHOGAbilitySystemComponent() const;

protected:
	virtual void BeginPlay() override;
	
	virtual void PossessedBy(AController* NewController) override;
	virtual void OnRep_PlayerState() override;
	
	void InitializeAbilityActorInfo();


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
	

};
