// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

using System.IO;
using UnrealBuildTool;

public class SodaLoadingScreen : ModuleRules
{
	public SodaLoadingScreen(ReadOnlyTargetRules Target) : base(Target)
	{		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core"
			}
		);
			
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
				"MoviePlayer",
			}
		);

        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        if (!Directory.Exists(ModuleDirectory + "/Private"))
        {
            bUsePrecompiled = true;
        }
    }
}
