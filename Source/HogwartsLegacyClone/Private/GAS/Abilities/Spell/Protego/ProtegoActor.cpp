// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/Abilities/Spell/Protego/ProtegoActor.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "UObject/ConstructorHelpers.h"


// Sets default values
AProtegoActor::AProtegoActor()
{
	PrimaryActorTick.bCanEverTick = false;

	// Root
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	// Shield Mesh
	ShieldMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ShieldMesh"));
	ShieldMesh->SetupAttachment(Root);

	ShieldMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ShieldMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
	ShieldMesh->SetCastShadow(false);
	ShieldMesh->SetReceivesDecals(false);

	// 기본 Sphere 메쉬
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMeshRef(
		TEXT("/Engine/BasicShapes/Sphere.Sphere")
	);
	if (SphereMeshRef.Succeeded())
	{
		ShieldMesh->SetStaticMesh(SphereMeshRef.Object);
	}

	// Niagara
	ShieldNiagara = CreateDefaultSubobject<UNiagaraComponent>(TEXT("ShieldNiagara"));
	ShieldNiagara->SetupAttachment(Root);

	ShieldNiagara->SetAutoActivate(false);
	ShieldNiagara->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ShieldNiagara->SetGenerateOverlapEvents(false);
	ShieldNiagara->CastShadow = false;

	// 기본값
	ShieldScale = 2.0f;

	RelativeOffset = FVector(0.f, 0.f, 88.f);

	// 나이아가라는 메쉬보다 아주 살짝 바깥/위에 둘 수 있게 분리
	NiagaraRelativeOffset = FVector(0.f, 0.f, 88.f);
	NiagaraRelativeScale = FVector(1.0f, 1.0f, 1.0f);

	bAutoActivateNiagara = true;

	// 생성자 기본 적용
	ShieldMesh->SetRelativeLocation(RelativeOffset);
	ShieldMesh->SetRelativeScale3D(FVector(ShieldScale));

	ShieldNiagara->SetRelativeLocation(NiagaraRelativeOffset);
	ShieldNiagara->SetRelativeScale3D(NiagaraRelativeScale);

}

void AProtegoActor::BeginPlay()
{
	Super::BeginPlay();

	
	// Material 적용
	if (ShieldMaterial)
	{
		ShieldMID = UMaterialInstanceDynamic::Create(ShieldMaterial, this);
		if (ShieldMID)
		{
			ShieldMesh->SetMaterial(0, ShieldMID);
		}
	}

	// Niagara 시스템 적용
	if (ShieldNiagara)
	{
		
		if (ShieldNiagaraSystem)
		{
			ShieldNiagara->SetAsset(ShieldNiagaraSystem);
		}

		if (bAutoActivateNiagara && ShieldNiagaraSystem)
		{
			ShieldNiagara->Activate(true);
		}
	}
}



