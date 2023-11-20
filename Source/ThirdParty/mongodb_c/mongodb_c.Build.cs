using System.IO;
using UnrealBuildTool;
using System;

public class mongodb_c : ModuleRules
{
	private string BinariesPath
	{
		get { return Path.GetFullPath(Path.Combine(ModuleDirectory, "../../../Binaries/", Target.Platform.ToString())); }
	}

	public mongodb_c(ReadOnlyTargetRules Target) : base(Target)
	{
		Type = ModuleType.External;

		if (Target.Platform == UnrealTargetPlatform.Win64)
		{
			string InstallDir = Path.Combine(ModuleDirectory, "Win64");
			PublicIncludePaths.Add(Path.Combine(InstallDir, "include", "libbson-1.0")); 
			PublicIncludePaths.Add(Path.Combine(InstallDir, "include", "libmongoc-1.0")); 
			PublicAdditionalLibraries.Add(Path.Combine(InstallDir, "lib", "bson-static-1.0.lib"));
            PublicAdditionalLibraries.Add(Path.Combine(InstallDir, "lib", "mongoc-static-1.0.lib"));
            PublicDefinitions.Add("BSON_STATIC");
            PublicDefinitions.Add("MONGOC_STATIC");

            PublicAdditionalLibraries.Add("Bcrypt.lib");
			PublicAdditionalLibraries.Add("Dnsapi.lib");
            PublicAdditionalLibraries.Add("Secur32.lib");
        }
		else if (Target.Platform == UnrealTargetPlatform.Linux)
		{
            string InstallDir = Path.Combine(ModuleDirectory, "Linux");
            PublicIncludePaths.Add(Path.Combine(InstallDir, "include", "libbson-1.0"));
            PublicIncludePaths.Add(Path.Combine(InstallDir, "include", "libmongoc-1.0"));
            PublicAdditionalLibraries.Add(Path.Combine(InstallDir, "lib", "libbson-static-1.0.a"));
            PublicAdditionalLibraries.Add(Path.Combine(InstallDir, "lib", "libmongoc-static-1.0.a"));
            PublicAdditionalLibraries.Add(Path.Combine(BinariesPath, "libresolv.so"));
            RuntimeDependencies.Add(Path.Combine(BinariesPath, "libresolv.so"));
            PublicDefinitions.Add("BSON_STATIC");
            PublicDefinitions.Add("MONGOC_STATIC");
        }
	}
}
