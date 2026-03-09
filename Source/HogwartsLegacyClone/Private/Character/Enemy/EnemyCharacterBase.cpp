// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/Enemy/EnemyCharacterBase.h"

#include "Components/CapsuleComponent.h"
#include "Data/Enemy/DA_EnemyConfig.h"
#include "GAS/Attributes/EnemyAttributeSet.h"
#include "Core/HOG_GameplayTags.h"
#include "GameFramework/CharacterMovementComponent.h"

AEnemyCharacterBase::AEnemyCharacterBase()
{
	PrimaryActorTick.bCanEverTick = true;

	// 팀 태그
	TeamTag = HOGGameplayTags::Team_Enemy;

	// GAS
	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("EnemyAbilitySystemComp"));
	AttributeSet = CreateDefaultSubobject<UEnemyAttributeSet>(TEXT("EnemyAttributeSet"));

	// AI
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
}

UAbilitySystemComponent* AEnemyCharacterBase::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

float AEnemyCharacterBase::GetHealth() const
{
	return AttributeSet ? AttributeSet->GetHealth() : 0.f;
}

float AEnemyCharacterBase::GetMaxHealth() const
{
	return AttributeSet ? AttributeSet->GetMaxHealth() : 0.f;
}

// UI 표시용 또는 보스 패턴 변화에 사용
float AEnemyCharacterBase::GetHealthPercent() const
{
	float MaxHP = GetMaxHealth();
	return MaxHP > 0.f ? GetHealth() / MaxHP : 0.f;
}

UBehaviorTree* AEnemyCharacterBase::GetBehaviorTree() const
{
	return EnemyConfig ? EnemyConfig->BehaviorTree : nullptr;
}

// 현재 태그 활성화 체크
bool AEnemyCharacterBase::HasGameplayTag(FGameplayTag Tag) const
{
	return AbilitySystemComponent ? AbilitySystemComponent->HasMatchingGameplayTag(Tag) : false;
}

// 태그 활성화
void AEnemyCharacterBase::AddGameplayTag(FGameplayTag Tag)
{
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->AddLooseGameplayTag(Tag);
	}
}

// 태그 비활성화
void AEnemyCharacterBase::RemoveGameplayTag(FGameplayTag Tag)
{
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->RemoveLooseGameplayTag(Tag);
	}
}

// 초기화 위치(빙의 시)
void AEnemyCharacterBase::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	InitializeAbilitySystem();
}

void AEnemyCharacterBase::HandleDeath_Implementation()
{
	Super::HandleDeath_Implementation();
	
	// 태그 상태를 Dead로 변경
	AddGameplayTag(HOGGameplayTags::State_Dead);
	// Dead 상태에서의 UI, Sound etc.. 에 알림
	OnEnemyDeath.Broadcast();
	
	// 콜리전 Disabled
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	
	// 움직임 멈춤
	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->DisableMovement();
	
	// 모든 어빌리티 제거
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->CancelAllAbilities();
	}
	
	// 현재는 Dead 상태일 경우 3초 후에 파괴
	SetLifeSpan(LifeSpanWhenDead);
}

// GAS 초기화
void AEnemyCharacterBase::InitializeAbilitySystem()
{
	if (!AbilitySystemComponent) return;

	AbilitySystemComponent->InitAbilityActorInfo(this, this);

	InitializeAttributes();
	GiveStartupAbilities();
	BindAttributeCallbacks();
}

// Attribute 초기화
void AEnemyCharacterBase::InitializeAttributes()
{
	if (!AbilitySystemComponent || !EnemyConfig || !EnemyConfig->DefaultAttributes) return;
	
	// Effect 컨텍스트 생성
	FGameplayEffectContextHandle Context = AbilitySystemComponent->MakeEffectContext();
	// 발생 주체 설정(->자기 자신)
	Context.AddSourceObject(this);
	
	// Effect 정보
	FGameplayEffectSpecHandle Spec= AbilitySystemComponent->MakeOutgoingSpec(
		EnemyConfig->DefaultAttributes,
		1,
		Context);
	
	// Effect 적용(->자신에서 적용)
	if (Spec.IsValid())
	{
		AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
	}
}

// StartUp Ability 주입(데이터 어셋에 캐싱된 데이터 참조)
void AEnemyCharacterBase::GiveStartupAbilities()
{
	if (!AbilitySystemComponent || !EnemyConfig) return;
	
	for (const TSubclassOf<UGameplayAbility>& AbilityClass : EnemyConfig->StartupAbilities)
	{
		if (!AbilityClass) continue;
		
		AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(AbilityClass, 1, INDEX_NONE, this));
	}
}

// 어트리뷰트 콜백 바인딩
void AEnemyCharacterBase::BindAttributeCallbacks()
{
	if (!AbilitySystemComponent || !AttributeSet) return;

	// 현재 체력이 변경되었을때 호출되는 콜백
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(AttributeSet->GetHealthAttribute())
	                      .AddUObject(this, &AEnemyCharacterBase::OnHealthChangedInternal);
}

// 각 Enemy가 체력이 변경될 시에 Specific한 로직 실행
void AEnemyCharacterBase::OnHealthChanged(float OldValue, float NewValue)
{
	// 자식에서 오버라이드
}

// 체력이 변경시 호출되는 콜백 함수
void AEnemyCharacterBase::OnHealthChangedInternal(const FOnAttributeChangeData& Data)
{
	// 각 Enemy에 맞는 로직 실행
	OnHealthChanged(Data.OldValue, Data.NewValue);
	
	// 공통로직
	
	// 피격
	if (Data.NewValue < Data.OldValue)
	{
		float Damage = Data.OldValue - Data.NewValue;
		
		// 데미지 받는 경우 호출
		// Ex) UI 업데이트, Sound etc..
		OnEnemyDamaged.Broadcast(Damage);
		
		// 피격 Ability 실행(태그/몽타주는 어빌리티가 관리)
		if (AbilitySystemComponent)
		{
			FGameplayTagContainer HitReactTag;
			// 피격 태그 추가(제거는 피격 애니메이션이 끝나면 제거)
			HitReactTag.AddTag(HOGGameplayTags::State_Hit);
			// 피격 어빌리티 활성화
			AbilitySystemComponent->TryActivateAbilitiesByTag(HitReactTag);
		}
	}
	
	// 적 Die
	if (Data.NewValue <= 0.0f && !IsDead())
	{
		Die();
	}
}
