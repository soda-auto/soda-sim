using System.IO;
using UnrealBuildTool;

using System;

public class mongodb_cpp : ModuleRules
{
	private string BinariesPath
	{
		get { return Path.GetFullPath(Path.Combine(ModuleDirectory, "../../../Binaries/", Target.Platform.ToString())); }
	}

	public mongodb_cpp(ReadOnlyTargetRules Target) : base(Target)
	{
		Type = ModuleType.External;

		if (Target.Platform == UnrealTargetPlatform.Win64)
		{
			string InstallDir = Path.Combine(ModuleDirectory, "Win64");
			PublicIncludePaths.Add(Path.Combine(InstallDir, "include", "bsoncxx", "v_noabi"));
			PublicIncludePaths.Add(Path.Combine(InstallDir, "include", "mongocxx", "v_noabi"));
			PublicAdditionalLibraries.Add(Path.Combine(InstallDir, "lib", "bsoncxx-static.lib"));
			PublicAdditionalLibraries.Add(Path.Combine(InstallDir, "lib", "mongocxx-static.lib"));
			PublicDefinitions.Add("MONGOCXX_STATIC");
            PublicDefinitions.Add("BSONCXX_STATIC");
        }
		else if (Target.Platform == UnrealTargetPlatform.Linux)
		{
            string InstallDir = Path.Combine(ModuleDirectory, "Linux");
            PublicIncludePaths.Add(Path.Combine(InstallDir, "include", "bsoncxx", "v_noabi"));
            PublicIncludePaths.Add(Path.Combine(InstallDir, "include", "mongocxx", "v_noabi"));
            PublicAdditionalLibraries.Add(Path.Combine(InstallDir, "lib", "libbsoncxx-static.a"));
            PublicAdditionalLibraries.Add(Path.Combine(InstallDir, "lib", "libmongocxx-static.a"));
            PublicDefinitions.Add("MONGOCXX_STATIC");
            PublicDefinitions.Add("BSONCXX_STATIC");
        }

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Boost",
				"mongodb_c"
			}
		);
	}
}
