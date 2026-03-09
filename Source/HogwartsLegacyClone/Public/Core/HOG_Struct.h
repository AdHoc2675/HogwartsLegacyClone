
#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GameplayEffectTypes.h"

#include "HOG_Struct.generated.h"

USTRUCT(BlueprintType)
struct HOGWARTSLEGACYCLONE_API FDamageRequest
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Damage")
	TObjectPtr<AActor> SourceActor = nullptr;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Damage")
	TObjectPtr<AActor> TargetActor = nullptr;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Damage")
	TObjectPtr<AActor> InstigatorActor = nullptr;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Damage")
	TObjectPtr<AActor> DamageCauser = nullptr;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Damage")
	float BaseDamage = 0.0f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Damage")
	FGameplayTag DamageTypeTag;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Damage")
	FHitResult HitResult;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Damage")
	FGameplayTagContainer SourceTags;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Damage")
	FGameplayTagContainer TargetTags;
};

USTRUCT(BlueprintType)
struct HOGWARTSLEGACYCLONE_API FDamageResult
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, Category="Damage")
	bool bWasApplied = false;

	UPROPERTY(BlueprintReadOnly, Category="Damage")
	bool bWasBlocked = false;

	UPROPERTY(BlueprintReadOnly, Category="Damage")
	bool bWasParried = false;

	UPROPERTY(BlueprintReadOnly, Category="Damage")
	bool bKilledTarget = false;

	UPROPERTY(BlueprintReadOnly, Category="Damage")
	float FinalDamage = 0.0f;
};