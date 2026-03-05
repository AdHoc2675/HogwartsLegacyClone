#include "GAS/Attributes/HOGAttributeSet.h"
#include "GameplayEffectExtension.h"

UHOGAttributeSet::UHOGAttributeSet()
{
	// 기본값
	InitMaxHealth(100.f);
	InitHealth(100.f);

	InitAttackPower(10.f);
}

void UHOGAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	if (Attribute == GetMaxHealthAttribute())
	{
		NewValue = FMath::Max(1.f, NewValue);
	}
	else if (Attribute == GetHealthAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxHealth());
	}
}

void UHOGAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	if (Data.EvaluatedData.Attribute == GetHealthAttribute()
		|| Data.EvaluatedData.Attribute == GetMaxHealthAttribute())
	{
		ClampHealth();
	}
}

void UHOGAttributeSet::ClampHealth()
{
	SetHealth(FMath::Clamp(GetHealth(), 0.f, GetMaxHealth()));
}