using UnrealBuildTool;
using System.Diagnostics;
using System.Collections.Generic;
using System.IO;
using System;

public class SodaProtoV1 : ModuleRules
{

	public SodaProtoV1(ReadOnlyTargetRules Target) : base(Target)
	{
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
                "Core",
                "Engine",
                "CoreUObject",
                "UnrealSoda",
                "RuntimeEditor",
                "sodasimprotov1",
                "Sockets",
                "Networking",
                "libZmq",
            }
		);

        if (!Directory.Exists(ModuleDirectory +"/Private"))
        {
            bUsePrecompiled = true;
        }

        bEnableExceptions = true;

        //bRequiresImplementModule = false;
    }
}
