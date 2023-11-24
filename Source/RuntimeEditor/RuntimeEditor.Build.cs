// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

using UnrealBuildTool;
using System.Diagnostics;
using System.Collections.Generic;
using System.IO;
using System;

public class RuntimeEditor : ModuleRules
{
	public RuntimeEditor(ReadOnlyTargetRules Target) : base(Target)
	{
        PublicIncludePaths.AddRange(
			new string[] 
			{
				Path.Combine(ModuleDirectory, "Public"),
			}
		);

		PrivateIncludePaths.AddRange(
			new string[] {
				Path.Combine(ModuleDirectory, "Private"),
			}
		);

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"PropertyPath",
				"DesktopPlatform",
				"AppFramework",
				"ApplicationCore",
				"InputCore",
				"Slate",
				"SlateCore",
				"Json",
				"JsonUtilities",
				"SodaStyle"
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"ToolWidgets",
				"RHI",
				//"Settings",
				//"MainFrame"
				"Projects",
                "DeveloperSettings",
                "AssetRegistry"
            }
		);
		
		//bEnableExceptions = true;
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		//bEnableUndefinedIdentifierWarnings = false;
		//bUseRTTI = true;
		//MinFilesUsingPrecompiledHeaderOverride = 1;
		//bFasterWithoutUnity = true;
		//bUsePrecompiled = true;
		bUseUnity = false;

		// The Shipping configuration isn't supported yet
		/*
        if (Target.Configuration != UnrealTargetConfiguration.Shipping)
        {
            PrecompileForTargets = PrecompileTargetsType.Any;
        }
        else
        {
            PrecompileForTargets = PrecompileTargetsType.None;
        }
		*/

		if (Target.Configuration == UnrealTargetConfiguration.Shipping)
		{
			throw new NullReferenceException("The Shipping configuration isn't supported yet");
        }

        if (!Directory.Exists(ModuleDirectory + "/Private"))
        {
            bUsePrecompiled = true;
        }
    }
}
