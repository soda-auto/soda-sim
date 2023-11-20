using System.IO;
using UnrealBuildTool;
using System;

public class dbcppp : ModuleRules
{
	private string BinariesPath
	{
		get { return Path.GetFullPath(Path.Combine(ModuleDirectory, "../../../Binaries/", Target.Platform.ToString())); }
	}

	public dbcppp(ReadOnlyTargetRules Target) : base(Target)
	{
		Type = ModuleType.External;

        PublicDefinitions.Add("DBCPPP_STATIC");

        if (Target.Platform == UnrealTargetPlatform.Win64)
		{
			string InstallDir = Path.Combine(ModuleDirectory, "Win64");
			//RuntimeDependencies.Add(Path.Combine(BinariesPath, "libdbcppp.dll"));
			//RuntimeDependencies.Add(Path.Combine(BinariesPath, "libxml2.dll"));
			//RuntimeDependencies.Add(Path.Combine(BinariesPath, "libxmlmm.dll"));;
			PublicIncludePaths.Add(Path.Combine(InstallDir, "include"));
            //PublicAdditionalLibraries.Add(Path.Combine(InstallDir, "lib", "libdbcppp.lib"));

            PublicAdditionalLibraries.Add(Path.Combine(InstallDir, "lib/libdbcppp.lib"));
            PublicAdditionalLibraries.Add(Path.Combine(InstallDir, "lib/libxml2s.lib"));
            PublicAdditionalLibraries.Add(Path.Combine(InstallDir, "lib/libxmlmm.lib"));


        }
		else if (Target.Platform == UnrealTargetPlatform.Linux)
		{
            string InstallDir = Path.Combine(ModuleDirectory, "Linux");
            //RuntimeDependencies.Add(Path.Combine(BinariesPath, "libdbcppp.so"));
            //RuntimeDependencies.Add(Path.Combine(BinariesPath, "libxml2.so"));
            //RuntimeDependencies.Add(Path.Combine(BinariesPath, "libxml2.so.2.9.10"));
            //RuntimeDependencies.Add(Path.Combine(BinariesPath, "libxmlmm.so"));
            PublicIncludePaths.Add(Path.Combine(InstallDir, "include"));
            //PublicAdditionalLibraries.Add(Path.Combine(BinariesPath, "libdbcppp.so"));

            PublicAdditionalLibraries.Add(Path.Combine(InstallDir, "lib/libdbcppp.a"));
            PublicAdditionalLibraries.Add(Path.Combine(InstallDir, "lib/libxml2.a"));
            PublicAdditionalLibraries.Add(Path.Combine(InstallDir, "lib/libxmlmm.a"));
        }
	}
}
