// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

using System.IO;
using UnrealBuildTool;

public class SodaPak : ModuleRules
{
	public SodaPak(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				//"CoreUObject",
				"Engine",
			}
		);

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "AssetRegistry",
                "PakFile",
				"Json"
            }
        );

        if (!Directory.Exists(ModuleDirectory + "/Private"))
        {
            bUsePrecompiled = true;
        }
    }
}
