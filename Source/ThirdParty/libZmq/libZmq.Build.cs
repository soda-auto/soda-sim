// Fill out your copyright notice in the Description page of Project Settings.

using System.IO;
using UnrealBuildTool;

public class libZmq : ModuleRules
{
	private string BinariesPath
	{
		get { return Path.GetFullPath(Path.Combine(ModuleDirectory, "../../../Binaries/", Target.Platform.ToString())); }
	}

    string ModuleDirectoryFullPath
    {
        get { return System.IO.Path.GetFullPath(ModuleDirectory); }
    }

	public libZmq(ReadOnlyTargetRules Target) : base(Target)
	{
		Type = ModuleType.External;

		if (Target.Platform == UnrealTargetPlatform.Win64)
		{
			PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "include/Win64")); 
			PublicAdditionalLibraries.Add(Path.Combine(ModuleDirectory, "lib/Win64/libzmq-v141-mt-4_3_2.lib"));
			RuntimeDependencies.Add(Path.Combine(BinariesPath, "libzmq-v141-mt-4_3_2.dll"));
			RuntimeDependencies.Add(Path.Combine(BinariesPath, "libsodium.dll"));
			PrivateDependencyModuleNames.Add("zlib");
            PublicDelayLoadDLLs.Add("libsodium.dll");
            PublicDelayLoadDLLs.Add("libzmq-v141-mt-4_3_2.dll");
        }
		else if (Target.Platform == UnrealTargetPlatform.Linux)
		{
			//string LibsPath = Path.Combine(ModuleDirectoryFullPath, "lib", "Linux");
			PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "include/Linux")); 
			PublicAdditionalLibraries.Add(Path.Combine(BinariesPath,"libzmq.so"));
			RuntimeDependencies.Add(Path.Combine(BinariesPath,"libzmq.so"));
			RuntimeDependencies.Add(Path.Combine(BinariesPath,"libzmq.so.5"));
			RuntimeDependencies.Add(Path.Combine(BinariesPath,"libzmq.so.5.2.4"));
		}
	}
}
