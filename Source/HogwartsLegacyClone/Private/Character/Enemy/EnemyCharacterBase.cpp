#include "Character/Enemy/EnemyCharacterBase.h"


#include "Character/Enemy/Anim/EnemyAnimInstanceBase.h"
#include "Character/Enemy/Handler/EnemyDamageHandler.h"
#include "Character/Enemy/Interface/IMeleeAttacker.h"
#include "Components/CapsuleComponent.h"
#include "Data/Enemy/DA_EnemyConfigBase.h"
#include "GAS/Attributes/HOGAttributeSet.h"
#include "Core/HOG_GameplayTags.h"
#include "GameFramework/CharacterMovementComponent.h"


AEnemyCharacterBase::AEnemyCharacterBase()
{
	PrimaryActorTick.bCanEverTick = true;

	// 팀 태그
	TeamTag = HOGGameplayTags::Team_Enemy;

	// GAS
	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("EnemyAbilitySystemComp"));
	AttributeSet = CreateDefaultSubobject<UHOGAttributeSet>(TEXT("EnemyAttributeSet"));

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

float AEnemyCharacterBase::GetMinAttackRange() const
{
	return 120.f;
}

UBehaviorTree* AEnemyCharacterBase::GetBehaviorTree() const
{
	UDA_EnemyConfigBase* EnemyConfig = GetEnemyConfig();
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

void AEnemyCharacterBase::BeginPlay()
{
	Super::BeginPlay();
	
	DamageHandler = NewObject<UEnemyDamageHandler>(this);
	DamageHandler->Initialize(this);
}

void AEnemyCharacterBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (DamageHandler)
	{
		DamageHandler->Shutdown();
	}
	
	Super::EndPlay(EndPlayReason);
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
	
	// 애니메이션 실행중지
	if (UEnemyAnimInstanceBase* AnimInstance = Cast<UEnemyAnimInstanceBase>(GetMesh()->GetAnimInstance()))
	{
		AnimInstance->SetDead();
		AnimInstance->StopAllMontages(0.0f);
	}
	
	// 콜리전 Disabled
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	
	// 움직임 멈춤
	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->DisableMovement();
	
	OnEnemyDeath.Broadcast();
}

// GAS 초기화
void AEnemyCharacterBase::InitializeAbilitySystem()
{
	if (!AbilitySystemComponent)
	{
		// Debug::Print(TEXT("[EnemyCharacterBase] InitializeAbilitySystem failed | ASC is null"), FColor::Red);
		return;
	}

	AbilitySystemComponent->InitAbilityActorInfo(this, this);

	// BaseCharacter의 TeamTag를 ASC Loose Tag로 동기화
	SyncTeamTagToAbilitySystem();

	GiveStartupAbilities();
	BindAttributeCallbacks();
}

// StartUp Ability 주입(데이터 어셋에 캐싱된 데이터 참조)
void AEnemyCharacterBase::GiveStartupAbilities()
{
	UDA_EnemyConfigBase* EnemyConfig = GetEnemyConfig();
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
	if (!AbilitySystemComponent) return;
	
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
		UHOGAttributeSet::GetHealthAttribute()
	).AddUObject(this, &AEnemyCharacterBase::OnHealthChangedInternal);
}

void AEnemyCharacterBase::SyncTeamTagToAbilitySystem()
{
	if (!AbilitySystemComponent || !TeamTag.IsValid())
	{
		return;
	}

	if (!AbilitySystemComponent->HasMatchingGameplayTag(TeamTag))
	{
		AbilitySystemComponent->AddLooseGameplayTag(TeamTag);
	}
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
	
	// 피격
	if (Data.NewValue < Data.OldValue)
	{
		float Damage = Data.OldValue - Data.NewValue;

		// 데미지 받는 경우 호출
		// Ex) UI 업데이트, Sound etc..
		OnEnemyDamaged.Broadcast(Damage);
	}

	// 적 Die
	if (Data.NewValue <= 0.f && !IsDead())
	{
		Die();
	}
}