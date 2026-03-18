// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class HogwartsLegacyClone : ModuleRules
{
	public HogwartsLegacyClone(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { 
			"Core", 
			"CoreUObject",
			"Engine",
			"InputCore", 
			"EnhancedInput",
			"GameplayAbilities", 
			"GameplayTags", 
			"GameplayTasks",

			"Niagara",
			"AnimGraphRuntime",
			"AIModule",
			
			"UMG",
			"Slate",
			"SlateCore"

		});

		PrivateDependencyModuleNames.AddRange(new string[] {  });
    }
}
