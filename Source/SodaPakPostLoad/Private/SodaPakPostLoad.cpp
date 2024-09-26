#include "SodaPakPostLoad.h"
#include "SodaPak.h"
#include "Serialization/ArrayReader.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonWriter.h"
#include "Serialization/JsonSerializer.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/AssetRegistryState.h"
#include "GenericPlatform/GenericPlatformFile.h"
#include "IPlatformFilePak.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "Dom/JsonObject.h"



static bool LoadAssetRegistryFile(const FString& AssetRegistryFile)
{
	//IPlatformFile* PlatformFile = FPlatformFileManager::Get().FindPlatformFile(TEXT("PakFile"));
	//if (PlatformFile)
	{
		//if (PlatformFile->FileExists(*AssetRegistryFile))
		{
			FArrayReader SerializedAssetData;

			if (FFileHelper::LoadFileToArray(SerializedAssetData, *AssetRegistryFile))
			{
				SerializedAssetData.Seek(0);
				IAssetRegistry& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName).Get();
				FAssetRegistryState PakState;
				if (PakState.RemovePackageData("/SodaSim/MetadataAsset")) // Don't need load MetadataAsset from DLC
				{
					UE_LOG(LogSodaPak, Log, TEXT("LoadAssetRegistryFile(): Removed /SodaSim/MetadataAsset from \"%s\""), *AssetRegistryFile);
				}
				PakState.Load(SerializedAssetData);
				AssetRegistry.AppendState(PakState);

				UE_LOG(LogSodaPak, Log, TEXT("LoadAssetRegistryFile(): Loaded \"%s\""), *AssetRegistryFile);

				return true;
			}
		}
	}

	UE_LOG(LogSodaPak, Warning, TEXT("LoadAssetRegistryFile(): Can't load \"%s\""), *AssetRegistryFile);
	return false;
}


static FString GetSodaPakDir()
{
	return FPaths::ProjectSavedDir() / TEXT("Paks");
}


void FSodaPakPostLoadModule::StartupModule()
{
	// Try to load AssetRegistry.bin for any DLC paks.
	if (FPakPlatformFile* PakPlatformFile = static_cast<FPakPlatformFile*>(FPlatformFileManager::Get().FindPlatformFile(TEXT("PakFile"))))
	{
		TArray<FString> MountedPakFilenames;
		PakPlatformFile->GetMountedPakFilenames(MountedPakFilenames);

		for (auto & MountedPakFilename : MountedPakFilenames)
		{
			TRefCountPtr<FPakFile> PakFile = new FPakFile(PakPlatformFile, *MountedPakFilename, false);
			if (FPaths::IsUnderDirectory(PakFile->GetFilename(), GetSodaPakDir()))
			{
				TArray<FString> Files;
				PakFile->FindPrunedFilesAtPath(*PakFile->GetMountPoint(), Files, true, false, true);
				for (const FString& File : Files)
				{
					if (File.EndsWith("/AssetRegistry.bin", ESearchCase::CaseSensitive))
					{
						LoadAssetRegistryFile(File);
					}
				}
			}

			PakFile.SafeRelease();
		}
	}
}

void FSodaPakPostLoadModule::ShutdownModule()
{
}


IMPLEMENT_MODULE(FSodaPakPostLoadModule, SodaPakPostLoad)