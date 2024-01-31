using System.IO;
using UnrealBuildTool;
using System;

public class sodasimprotov1 : ModuleRules
{
	public sodasimprotov1(ReadOnlyTargetRules Target) : base(Target)
	{
		Type = ModuleType.External;

        if (Target.Platform == UnrealTargetPlatform.Win64)
		{
			string InstallDir = Path.Combine(ModuleDirectory, "Win64");
			PublicIncludePaths.Add(Path.Combine(InstallDir, "include"));
            PublicAdditionalLibraries.Add(Path.Combine(InstallDir, "lib/soda-sim-proto-v1.lib"));


        }
		else if (Target.Platform == UnrealTargetPlatform.Linux)
		{
            string InstallDir = Path.Combine(ModuleDirectory, "Linux");
            PublicIncludePaths.Add(Path.Combine(InstallDir, "include"));
            PublicAdditionalLibraries.Add(Path.Combine(InstallDir, "lib/libsoda-sim-proto-v1.a"));
        }
	}
}
