#include "Character/Enemy/EnemyCharacterBase.h"

#include "HOGDebugHelper.h"
#include "Character/Enemy/Interface/IMeleeAttacker.h"
#include "Components/CapsuleComponent.h"
#include "Components/TextBlock.h"
#include "Components/WidgetComponent.h"
#include "Data/Enemy/DA_EnemyConfigBase.h"
#include "GAS/Attributes/HOGAttributeSet.h"
#include "Core/HOG_GameplayTags.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/HOG_PlayerController.h"
#include "Pool/DamageNumberPool.h"
#include "UI/DamageNumberWidget.h"

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

// 초기화 위치(빙의 시)
void AEnemyCharacterBase::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	BindDamageDelegate();
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
	if (!AbilitySystemComponent)
	{
		// Debug::Print(TEXT("[EnemyCharacterBase] InitializeAbilitySystem failed | ASC is null"), FColor::Red);
		return;
	}

	AbilitySystemComponent->InitAbilityActorInfo(this, this);

	// BaseCharacter의 TeamTag를 ASC Loose Tag로 동기화
	SyncTeamTagToAbilitySystem();

	if (AbilitySystemComponent->AbilityActorInfo.IsValid())
	{
		// Debug::Print(FString::Printf(
		// 	TEXT("[EnemyCharacterBase] ASC Init Success | ASC=%s | Owner=%s | Avatar=%s"),
		// 	*GetNameSafe(AbilitySystemComponent),
		// 	*GetNameSafe(AbilitySystemComponent->AbilityActorInfo->OwnerActor.Get()),
		// 	*GetNameSafe(AbilitySystemComponent->AbilityActorInfo->AvatarActor.Get())
		// ), FColor::Green);
	}
	else
	{
		// Debug::Print(FString::Printf(
		// 	TEXT("[EnemyCharacterBase] ASC Init FAILED | ASC=%s | ActorInfo invalid"),
		// 	*GetNameSafe(AbilitySystemComponent)
		// ), FColor::Red);
	}

	InitializeAttributes();
	GiveStartupAbilities();
	BindAttributeCallbacks();
}

// Attribute 초기화
void AEnemyCharacterBase::InitializeAttributes()
{
	UDA_EnemyConfigBase* EnemyConfig = GetEnemyConfig();
	if (!AbilitySystemComponent || !EnemyConfig || !EnemyConfig->DefaultAttributes) return;

	// Effect 컨텍스트 생성
	FGameplayEffectContextHandle Context = AbilitySystemComponent->MakeEffectContext();
	// 발생 주체 설정(->자기 자신)
	Context.AddSourceObject(this);

	// Effect 정보
	FGameplayEffectSpecHandle Spec = AbilitySystemComponent->MakeOutgoingSpec(
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

void AEnemyCharacterBase::SpawnDamageNumber(float Damage)
{
	AHOG_PlayerController* PC = Cast<AHOG_PlayerController>(GetWorld()->GetFirstPlayerController());
	if (!PC) return;

	UDamageNumberPool* Pool = PC->GetDamageNumberPool();

	if (!Pool) return;

	// 메시 중간 위치
	float MeshHeight = GetMesh()->Bounds.BoxExtent.Z * 2.f;
	float BaseHeight = MeshHeight * 0.5f;

	double CurrentTime = GetWorld()->GetTimeSeconds();
	
	// 마지막 데미지로부터 1초 이내면 → 위로 쌓기
	if (CurrentTime - LastDamageNumberTime < 1.0f)
	{
		// 이전 데미지 숫자 위에 표시되도록 간격 추가
		LastDamageNumberZ += DamageNumberSpacing;
	}
	else
	{
		LastDamageNumberZ = 0.f;
	}
	// 이번 데미지 시간 기록
	LastDamageNumberTime = CurrentTime;

	FVector Offset = FVector(FMath::RandRange(-30.f, 30.f), 0.f, BaseHeight + LastDamageNumberZ);

	UWidgetComponent* Component = Pool->Acquire(GetMesh(), Offset);
	if (!Component) return;

	if (UDamageNumberWidget* Widget = Cast<UDamageNumberWidget>(Component->GetWidget()))
	{
		Widget->FloatDamageNumber(Damage, Component, Pool);
	}
}

void AEnemyCharacterBase::BindDamageDelegate()
{
	OnEnemyDamaged.AddUObject(this, &AEnemyCharacterBase::SpawnDamageNumber);
}
