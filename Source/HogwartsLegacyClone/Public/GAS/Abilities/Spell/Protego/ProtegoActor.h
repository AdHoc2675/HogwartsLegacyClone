// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProtegoActor.generated.h"

class USceneComponent;
class UStaticMeshComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class UNiagaraComponent;
class UNiagaraSystem;


UCLASS()
class HOGWARTSLEGACYCLONE_API AProtegoActor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AProtegoActor();
	
protected:
	virtual void BeginPlay() override;


protected:
	// Root
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Protego")
	TObjectPtr<USceneComponent> Root;

	// 보호막 구체 메쉬
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Protego")
	TObjectPtr<UStaticMeshComponent> ShieldMesh;

	// 보호막 표면/입자용 나이아가라
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Protego")
	TObjectPtr<UNiagaraComponent> ShieldNiagara;

	// 에디터에서 지정할 기본 머티리얼
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Protego|Visual")
	TObjectPtr<UMaterialInterface> ShieldMaterial;

	// 에디터에서 지정할 나이아가라 시스템
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Protego|Visual")
	TObjectPtr<UNiagaraSystem> ShieldNiagaraSystem;

	// 런타임 머티리얼 인스턴스
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> ShieldMID;

	// 메쉬 크기
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Protego|Visual")
	float ShieldScale;

	// 공통 위치 오프셋
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Protego|Visual")
	FVector RelativeOffset;

	// 나이아가라 전용 위치 오프셋
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Protego|Visual")
	FVector NiagaraRelativeOffset;

	// 나이아가라 전용 스케일
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Protego|Visual")
	FVector NiagaraRelativeScale;

	// BeginPlay 때 자동 활성화할지
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Protego|Visual")
	bool bAutoActivateNiagara;
	

};
