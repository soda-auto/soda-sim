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

namespace
{
	const TCHAR* PAK_EXT = TEXT(".pak");
	const TCHAR* PAK_UNINSTALEED_EXT = TEXT(".pak_uninstalled");
	const TCHAR* SPAK_EXT = TEXT(".spak");
}

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

class FFindByWildcardVisitor : public IPlatformFile::FDirectoryVisitor
{
	FString Wildcard;
	TArray<FString>& FoundFiles;
public:
	FFindByWildcardVisitor(const FString& Wildcard, TArray<FString>& InFoundFiles)
		: Wildcard(Wildcard)
		, FoundFiles(InFoundFiles)
	{}
	virtual bool Visit(const TCHAR* FilenameOrDirectory, bool bIsDirectory)
	{
		if (bIsDirectory == false)
		{
			FString Filename(FilenameOrDirectory);
			if (Filename.MatchesWildcard(Wildcard) && !FoundFiles.Contains(Filename))
			{
				FoundFiles.Add(Filename);
			}
		}
		return true;
	}
};


static bool LoadFromJson(const FString& FileName, FSodaPakDescriptor& Out)
{
	FString JsonString;
	if (!FFileHelper::LoadFileToString(JsonString, *FileName))
	{
		UE_LOG(LogSodaPak, Error, TEXT("FPakUtils::LoadFromJson(%s); Can't read file"), *FileName);
		return false;
	}

	TSharedPtr<FJsonObject> JSON;
	TSharedRef<TJsonReader<TCHAR>> Reader = TJsonReaderFactory<TCHAR>::Create(JsonString);
	if (!FJsonSerializer::Deserialize(Reader, JSON))
	{
		UE_LOG(LogSodaPak, Error, TEXT("FPakUtils::LoadFromJson(%s); Can't JSON deserialize "), *FileName);
		return false;
	}

	Out.PakName = FPaths::GetBaseFilename(FileName);
	/*
	if (!JSON->TryGetNumberField(TEXT("PakOrder"), Out.PakOrder))
	{
		UE_LOG(LogSoda, Error, TEXT("FPakUtils::LoadFromJson(%s); Can't find \"PakOrder\""), *FileName);
		return false;
	}
	*/
	if (!JSON->TryGetStringField(TEXT("FriendlyName"), Out.FriendlyName))
	{
		Out.FriendlyName = Out.PakName;
	}
	JSON->TryGetStringField(TEXT("Description"), Out.Description);
	JSON->TryGetStringField(TEXT("Category"), Out.Category);
	JSON->TryGetNumberField(TEXT("PakVersion"), Out.PakVersion);
	JSON->TryGetStringField(TEXT("EngineVersion"), Out.EngineVersion);
	JSON->TryGetStringField(TEXT("CreatedByURL"), Out.CreatedByURL);
	JSON->TryGetStringField(TEXT("CreatedBy"), Out.CreatedBy);
	//JSON->TryGetBoolField(TEXT("IsEnabled"), Out.bIsEnabled);

	/*
	const TArray<TSharedPtr<FJsonValue>>* MountPointsArray;
	if (JSON->TryGetArrayField(TEXT("MountPoints"), MountPointsArray))
	{
		for (auto& It : *MountPointsArray)
		{
			FString Val;
			if (It->TryGetString(Val))
			{
				Out.MountPoints.Add(Val);
			}
		}
	}
	*/

	//"PluginsDependencies"
	//"PaksDependencies"

	return true;
}

static bool SaveToJson(const FString& FileName, const FSodaPakDescriptor& In)
{
	TSharedRef<FJsonObject> JSON = MakeShared<FJsonObject>();

	//JSON->SetNumberField(TEXT("PakOrder"), In.PakOrder);
	JSON->SetStringField(TEXT("FriendlyName"), In.FriendlyName);
	JSON->SetStringField(TEXT("Description"), In.Description);
	JSON->SetStringField(TEXT("Category"), In.Category);
	JSON->SetNumberField(TEXT("PakVersion"), In.PakVersion);
	JSON->SetStringField(TEXT("EngineVersion"), In.EngineVersion);
	JSON->SetStringField(TEXT("CreatedByURL"), In.CreatedByURL);
	JSON->SetStringField(TEXT("CreatedBy"), In.CreatedBy);
	//JSON->SetBoolField(TEXT("IsEnabled"), In.bIsEnabled);
	/*
	TArray<TSharedPtr<FJsonValue>> MountPointsArray;
	for (auto& It : In.MountPoints) MountPointsArray.Add(MakeShared<FJsonValueString>(It));
	JSON->SetArrayField(TEXT("MountPoints"), MoveTemp(MountPointsArray));
	*/

	FString JsonString;
	TSharedRef< TJsonWriter<> > Writer = TJsonWriterFactory<>::Create(&JsonString);
	if (!FJsonSerializer::Serialize(JSON, Writer))
	{
		UE_LOG(LogSodaPak, Error, TEXT("SaveToJson(); Can't serialize"));
		return false;
	}

	if (!FFileHelper::SaveStringToFile(JsonString, *FileName))
	{
		UE_LOG(LogSodaPak, Error, TEXT("SaveToJson(); Can't write to '%s' file"), *FileName);
		return false;
	}
	return true;
}

FSodaPak::FSodaPak(const FSodaPakDescriptor& Descriptor, const FString& BaseDir)
	: Descriptor(Descriptor)
	, BaseDir(BaseDir)
{
	InstalledPakFile = BaseDir / Descriptor.PakName + PAK_EXT;
	UninstalledPakFile = BaseDir / Descriptor.PakName + PAK_UNINSTALEED_EXT;

	UpdateInstallStatus();
	UpdateMountStatus();
}

bool FSodaPak::Install()
{
	if (InstallStatus != ESodaPakInstallStatus::Installed)
	{
		bool bRes = IFileManager::Get().Move(*InstalledPakFile, *UninstalledPakFile);
		InstallStatus = bRes ? ESodaPakInstallStatus::Installed : ESodaPakInstallStatus::Broken;
	}
	return InstallStatus == ESodaPakInstallStatus::Installed;
}

bool FSodaPak::Uninstall()
{
	if (InstallStatus != ESodaPakInstallStatus::Uninstalled)
	{
		bool bRes = IFileManager::Get().Move(*UninstalledPakFile, *InstalledPakFile);
		InstallStatus = bRes ? ESodaPakInstallStatus::Uninstalled : ESodaPakInstallStatus::Broken;
	}
	return InstallStatus == ESodaPakInstallStatus::Uninstalled;
}

void FSodaPak::UpdateInstallStatus()
{
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();

	bool bInstalledExist = FPaths::FileExists(InstalledPakFile);
	bool bUninstalledExist = FPaths::FileExists(UninstalledPakFile);

	if (bInstalledExist && bUninstalledExist)
	{
		InstallStatus = ESodaPakInstallStatus::Broken;
	}
	else if (!bInstalledExist && !bUninstalledExist)
	{
		InstallStatus = ESodaPakInstallStatus::Broken;
	}
	else if (bInstalledExist && !bUninstalledExist)
	{
		InstallStatus = ESodaPakInstallStatus::Installed;
	}
	else //if (bInstalledExist && !bUninstalledExist)
	{
		InstallStatus = ESodaPakInstallStatus::Uninstalled;
	}
}

void FSodaPak::UpdateMountStatus()
{
	if (FPakPlatformFile* PakPlatformFile = static_cast<FPakPlatformFile*>(FPlatformFileManager::Get().FindPlatformFile(TEXT("PakFile"))))
	{
		TArray<FString> MountedPakFilenames;
		PakPlatformFile->GetMountedPakFilenames(MountedPakFilenames);
		bIsMounted = MountedPakFilenames.FindByPredicate([this](const auto& It) { return FPaths::IsSamePath(It, InstalledPakFile); }) != nullptr;

	}
	else
	{
		bIsMounted = false;
	}
}

void FSodaPakModule::StartupModule()
{
	TArray<FString>	FoundPaksInstalled;
	TArray<FString>	FoundPaksUninstalled;
	TSet<FString>FoundPaks;
	FFindByWildcardVisitor PakVisitorInstalled(FString(TEXT("*")) + PAK_EXT, FoundPaksInstalled);
	FFindByWildcardVisitor PakVisitorUninstalled(FString(TEXT("*")) + PAK_UNINSTALEED_EXT, FoundPaksUninstalled);
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	PlatformFile.IterateDirectoryRecursively(*GetSodaPakDir(), PakVisitorInstalled);
	PlatformFile.IterateDirectoryRecursively(*GetSodaPakDir(), PakVisitorUninstalled);

	for (const FString& PakName : FoundPaksInstalled)
	{
		FoundPaks.Add(FPaths::GetBaseFilename(PakName));
	}
	for (const FString& PakName : FoundPaksUninstalled)
	{
		FoundPaks.Add(FPaths::GetBaseFilename(PakName));
	}


	for (const FString& PakName : FoundPaks)
	{
		UE_LOG(LogSodaPak, Log, TEXT("FSodaPakModule::StartupModule(); Found: \"%s\""), *PakName);
		
		FString SPakPath = GetSodaPakDir() / PakName + SPAK_EXT;

		if (FPaths::FileExists(SPakPath))
		{
			FSodaPakDescriptor Desc;
			if (LoadFromJson(SPakPath, Desc))
			{
				TSharedPtr<FSodaPak> Pak = MakeShared<FSodaPak>(Desc, GetSodaPakDir());
				SodaPaks.Add(Pak);
			}
		}
		else
		{
			FSodaPakDescriptor Desc;
			Desc.FriendlyName = Desc.PakName = PakName;
			SaveToJson(SPakPath, Desc);
			TSharedPtr<FSodaPak> Pak = MakeShared<FSodaPak>(Desc, GetSodaPakDir());
			SodaPaks.Add(Pak);
		}
	}

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

void FSodaPakModule::ShutdownModule()
{
}


DEFINE_LOG_CATEGORY(LogSodaPak);
IMPLEMENT_MODULE(FSodaPakModule, SodaPak)