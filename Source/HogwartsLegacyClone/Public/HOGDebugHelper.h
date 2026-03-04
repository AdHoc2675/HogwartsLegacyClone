// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

namespace Debug
{
    inline void Print(
        const FString& Msg,
        const FColor& Color = FColor::MakeRandomColor(),
        int32 InKey = -1,
        float Duration = 7.f)
    {
        if (GEngine)
        {
            GEngine->AddOnScreenDebugMessage(InKey, Duration, Color, Msg);
        }

        UE_LOG(LogTemp, Warning, TEXT("%s"), *Msg);
    }

    inline void PrintFloat(
        const FString& Title,
        float Value,
        int32 InKey = -1,
        const FColor& Color = FColor::MakeRandomColor(),
        float Duration = 7.f)
    {
        const FString Msg = Title + TEXT(": ") + FString::SanitizeFloat(Value);

        if (GEngine)
        {
            GEngine->AddOnScreenDebugMessage(InKey, Duration, Color, Msg);
        }

        UE_LOG(LogTemp, Warning, TEXT("%s"), *Msg);
    }
}