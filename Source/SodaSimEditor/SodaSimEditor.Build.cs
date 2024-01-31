// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

using System;
using System.Diagnostics;
using System.IO;
using UnrealBuildTool;

public class SodaSimEditor : ModuleRules
{
    public SodaSimEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"UnrealSoda",
				"AnimGraphRuntime",
				"AnimGraph",
                "UnrealEd",
                "EditorFramework",
                "BlueprintGraph",
            }
		);

		bEnableExceptions = true;
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		bEnableUndefinedIdentifierWarnings = false;
		SetupModulePhysicsSupport(Target);
		//bUsePrecompiled = true;
		bUseUnity = false;

        if (!Directory.Exists(ModuleDirectory + "/Private"))
        {
            bUsePrecompiled = true;
        }
    }
}
