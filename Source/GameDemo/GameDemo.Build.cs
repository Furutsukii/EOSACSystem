// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class GameDemo : ModuleRules
{
	public GameDemo(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore" });

		PrivateDependencyModuleNames.AddRange(new string[] {  });

        if (Target.Platform == UnrealTargetPlatform.Win64)
        {
            PrivateDependencyModuleNames.AddRange(
                new string[] {
                     "EOSPlugin"
                }
            );
        }
    }
}
