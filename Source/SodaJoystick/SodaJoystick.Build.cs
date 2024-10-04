// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

using System.IO;
using UnrealBuildTool;

public class SodaJoystick : ModuleRules
{
	public SodaJoystick(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicIncludePaths.AddRange(
			new string[] {
				"Runtime/ApplicationCore/Public/GenericPlatform",
			}
		);
				
		
		PrivateIncludePaths.AddRange(
			new string[] {
				"SodaJoystick/Private",
			}
		);
			
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"InputCore",
				"InputDevice",
				"Slate",
				"SlateCore",
			}
		);

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "CoreUObject",
                "Engine",
                "Slate",
                "SlateCore",
                "ApplicationCore"
            }
        );

        if (Target.Platform == UnrealTargetPlatform.Win64)
		{
			PublicDependencyModuleNames.Add("SDL2_2010");
		}
			
		if (Target.Platform == UnrealTargetPlatform.Linux)
		{
			PrivateDependencyModuleNames.Add("SDL2");
		}

        if (!Directory.Exists(ModuleDirectory + "/Private"))
        {
            bUsePrecompiled = true;
        }
    }
}
