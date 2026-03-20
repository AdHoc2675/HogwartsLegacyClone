#pragma once

#include "CoreMinimal.h"
#include "Interactable/InteractableBase.h"
#include "InteractableLevitatable.generated.h"

class UStaticMeshComponent;
class UAbilitySystemComponent;
class UNiagaraComponent;

UCLASS()
class HOGWARTSLEGACYCLONE_API AInteractableLevitatable : public AInteractableBase
{
	GENERATED_BODY()
	
public:
	AInteractableLevitatable();

protected:
	virtual void BeginPlay() override;

public:
	virtual bool CanInteract_Implementation(AActor* Interactor) override;

protected:
	virtual void HandleInteract(AActor* Interactor) override;

public:
	UFUNCTION(BlueprintCallable, Category="Interaction")
	void StopLevitation();

	UFUNCTION(BlueprintImplementableEvent, Category="Interaction")
	void OnLevitated();
	
	UFUNCTION(BlueprintImplementableEvent, Category="Interaction")
	void OnDropped();

	// Getter
	float GetLevitateHeight() const { return LevitateHeight; }
	float GetLevitateDuration() const { return LevitateDuration; }
	float GetLevitateHoverDuration() const { return LevitateHoverDuration; } // 추가: 체공 시간 Getter

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Mesh")
	TObjectPtr<UStaticMeshComponent> BaseMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="VFX")
	TObjectPtr<UNiagaraComponent> MagicAuraVFXComp;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HOG|Levitate")
	float LevitateForce = 300.f;

	// === 오브젝트별 개별 부유 설정 ===
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HOG|Levitate")
	float LevitateHeight = 250.f; // 위로 떠오르는 대상 높이

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HOG|Levitate")
	float LevitateDuration = 1.0f; // 상승하는 데 걸리는 시간

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HOG|Levitate")
	float LevitateHoverDuration = 1.0f; // 다 떠오른 후 공중에 머무는 시간

	// === 플랫폼 모드 설정 ===
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HOG|Levitate")
	bool bIsPlatformMode = true;
};