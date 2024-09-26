// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

using System.IO;
using UnrealBuildTool;

public class SodaPakPostLoad : ModuleRules
{
	public SodaPakPostLoad(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				//"CoreUObject",
				"Engine",
				"SodaPak"
			}
		);

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "AssetRegistry",
                "PakFile",
            }
        );

        if (!Directory.Exists(ModuleDirectory + "/Private"))
        {
            bUsePrecompiled = true;
        }
    }
}
