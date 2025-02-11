// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

using System;
using System.Diagnostics;
using System.IO;
using UnrealBuildTool;

public class UnrealSoda : ModuleRules
{
    public UnrealSoda(ReadOnlyTargetRules Target) : base(Target)
	{
		PrivateIncludePaths.AddRange(
			new string[] 
			{
				EngineDirectory + "/Source/Runtime/Sockets/Private",
				ModuleDirectory + "/Private/ThirdParty",
				ModuleDirectory + "/Private/ThirdParty/opendrive_reader/include",
				ModuleDirectory + "/Private/ThirdParty/pugixml",
            }
		);

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"InputCore",
				"Sockets", 
				"Networking",
                "HTTP",
				"Json",
				"JsonUtilities",
				"libWebSockets",
				"UMG",
				"SodaStyle",
				"SodaJoystick",
				"libZmq",
                "dbcppp",
                "Projects",
				"ProceduralMeshComponent",
				"ImageWriteQueue",
				"ChaosVehicles",
				"ChaosVehiclesCore",
				"RuntimeEditor",
				"Slate",
				"SlateCore",
				"ToolMenus",
				"TypedElementFramework",
				"TypedElementRuntime",
                "AnimGraphRuntime",
                "MassActors",
                "MassSpawner"
            }
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"RHI",
				"RenderCore",
				"Projects",
				"Slate",
				"SlateCore",
				"HTTPServer",
				"WebRemoteControl",
				"RemoteControlCommon",
				"DesktopPlatform",
				"ToolWidgets",
				"mongodb_cpp",
                "mongodb_c",
                "Boost",
                "Eigen",
                "ApplicationCore",
                "PROJ",
                "GeometryAlgorithms",
                "MassEntity",
				"SodaPak",
                "StructUtils",
                "SQLiteCore"
            } 
		);

		RuntimeDependencies.Add(Path.Combine(PluginDirectory, "Content/Slate/..."));
		RuntimeDependencies.Add("$(TargetOutputDir)/../../Scripts/...", Path.Combine( PluginDirectory, "Scripts/..."));
		//RuntimeDependencies.Add("$(TargetOutputDir)/../../ReleaseNotes", Path.Combine( PluginDirectory, "ReleaseNotes"));
		//RuntimeDependencies.Add("$(TargetOutputDir)/../../Install/...", Path.Combine( PluginDirectory, "Install/..."));
        //RuntimeDependencies.Add("$(TargetOutputDir)/../../VERSION", Path.Combine(PluginDirectory, "VERSION"));
        //RuntimeDependencies.Add(Path.Combine(PluginDirectory, "Scripts/..."));
        //RuntimeDependencies.Add(Path.Combine(PluginDirectory, "ReleaseNotes"));
        //RuntimeDependencies.Add(Path.Combine(PluginDirectory, "Install/..."));

        AddEngineThirdPartyPrivateStaticDependencies(Target, "Eigen");

        bEnableExceptions = true;
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		bEnableUndefinedIdentifierWarnings = false;
		SetupModulePhysicsSupport(Target);
		//bUsePrecompiled = true;
		bUseUnity = false;
        //bUseRTTI = false;

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

        // Temporary hack to fix build for Linux (Ubuntu)
        if (Target.Platform == UnrealTargetPlatform.Linux)
		{
            PublicIncludePaths.Add("/usr/include/eigen3");
        }	

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
