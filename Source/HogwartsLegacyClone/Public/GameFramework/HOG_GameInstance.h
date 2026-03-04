// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "GameplayTagContainer.h"
#include "HOG_GameInstance.generated.h"

class UDA_SpellDefinition;
/**
 * 
 */
UCLASS()
class HOGWARTSLEGACYCLONE_API UHOG_GameInstance : public UGameInstance
{
	GENERATED_BODY()
	
	//Spell registry 관리 
	
	// 에디터에서 여기에 SpellDefinition들을 넣어두고, BeginPlay/Init에서 레지스트리로 빌드
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="HOG|Spell", meta=(AllowPrivateAccess="true"))
	TArray<TObjectPtr<UDA_SpellDefinition>> SpellDefinitions;
	
	// 런타임 조회용: SpellID(Tag) -> Definition
	UPROPERTY(Transient)
	TMap<FGameplayTag, TObjectPtr<UDA_SpellDefinition>> SpellRegistry;

public:
	virtual void Init() override;

	// 레지스트리 빌드 (중복/유효성 체크 포함)
	UFUNCTION(BlueprintCallable, Category="HOG|Spell")
	void BuildSpellRegistry();

	// SpellID로 Definition 조회
	UFUNCTION(BlueprintCallable, Category="HOG|Spell")
	UDA_SpellDefinition* GetSpellDefinition(FGameplayTag SpellID) const;
	
};
