// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/Player/PlayerCharacterBase.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"


APlayerCharacterBase::APlayerCharacterBase()
{
	bUseControllerRotationPitch=false;
	bUseControllerRotationRoll=false;
	bUseControllerRotationYaw=false;
	
	if (UCharacterMovementComponent* MoveComp=GetCharacterMovement())
	{
		MoveComp->bOrientRotationToMovement=true;
		MoveComp->RotationRate=FRotator(0.0f,600.0f,0.0f);
		MoveComp->JumpZVelocity=700.f; //점프 높이 수정 필요하면 여기 
		MoveComp->AirControl=0.35f;
		MoveComp->MaxWalkSpeed=500.f;
		MoveComp->BrakingDecelerationWalking=2000.f;
		
	}
	
	// 카메라 스프링암 셋팅
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(GetCapsuleComponent());
	CameraBoom->TargetArmLength = 400.f;
	CameraBoom->bUsePawnControlRotation = true; // 컨트롤러 회전에 따라 스프링암이 회전
	
	// 카메라 셋팅
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false; // 카메라는 스프링암 따라서
	
	// Capsule Size
	GetCapsuleComponent()->InitCapsuleSize(CapsuleRadius, CapsuleHalfHeight);

	// Mesh 기본 세팅(캡슐 기준 위치/회전)
	if (USkeletalMeshComponent* MeshComp = GetMesh())
	{
		MeshComp->SetRelativeLocation(MeshRelativeLocation);
		MeshComp->SetRelativeRotation(MeshRelativeRotation);

		// 보통 Character는 Mesh가 PawnCollision은 안 쓰도록
		MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	
}

void APlayerCharacterBase::BeginPlay()
{
	Super::BeginPlay();
}

void APlayerCharacterBase::Input_Move(const FInputActionValue& Value)
{
	FVector2D MoveAxis = Value.Get<FVector2D>();

	if(!Controller)
		return;

	FRotator ControlRotation = Controller->GetControlRotation();
	FRotator YawRotation(0.f, ControlRotation.Yaw, 0.f);

	FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

	AddMovementInput(ForwardDirection, MoveAxis.Y * MoveForwardScale);
	AddMovementInput(RightDirection, MoveAxis.X * MoveRightScale);
}

void APlayerCharacterBase::Input_Look(const FInputActionValue& Value)
{
	FVector2D LookAxis = Value.Get<FVector2D>();

	AddControllerYawInput(LookAxis.X * LookYawScale);
	AddControllerPitchInput(LookAxis.Y * LookPitchScale);
}

void APlayerCharacterBase::Input_JumpStarted()
{
	Jump();
}

void APlayerCharacterBase::Input_JumpCompleted()
{
	StopJumping();
}

void APlayerCharacterBase::Input_AbilityInputPressed(FGameplayTag InputTag)
{
	//나중에 마법 GAS
}

void APlayerCharacterBase::Input_AbilityInputReleased(FGameplayTag InputTag)
{
	//나중에 마법 GAS
}

