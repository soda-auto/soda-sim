// Fill out your copyright notice in the Description page of Project Settings.

using System.IO;
using UnrealBuildTool;

public class SDL2_2010 : ModuleRules
{
	private string BinariesPath
	{
		get { return Path.GetFullPath(Path.Combine(ModuleDirectory, "../../../Binaries/", Target.Platform.ToString())); }
	}

	public SDL2_2010(ReadOnlyTargetRules Target) : base(Target)
	{
		Type = ModuleType.External;

		if (Target.Platform == UnrealTargetPlatform.Win64)
		{
			//PublicDefinitions.Add("SDL_STATIC=0");
			//PublicDefinitions.Add("SDL_SHARED=1");
			//PublicDefinitions.Add("EPIC_EXTENSIONS=0");
			//PublicDefinitions.Add("SDL_DEPRECATED=1");
			PublicDefinitions.Add("SDL_WITH_EPIC_EXTENSIONS=1");
			PublicDefinitions.Add("WIN32");

			string includePath = Path.Combine(ModuleDirectory, "include");
			PublicIncludePaths.AddRange(new string[] { includePath });

			// Add the import library
			PublicAdditionalLibraries.Add(Path.Combine(ModuleDirectory, "lib", "x64","SDL2.lib"));
			PublicAdditionalLibraries.Add(Path.Combine(ModuleDirectory, "lib", "x64","SDL2main.lib"));
			//PublicAdditionalLibraries.Add(Path.Combine(ModuleDirectory, "lib", "x64","Version.lib"));

			// Delay-load the DLL, so we can load it from the right place first
			RuntimeDependencies.Add(Path.Combine(BinariesPath, "SDL2.dll"));
            PublicDelayLoadDLLs.Add("SDL2.dll");
        }
	}
}
