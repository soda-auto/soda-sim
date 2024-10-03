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
#include "Misc/ConfigContext.h"
#include "Misc/ConfigHierarchy.h"

namespace
{
	const TCHAR* PAK_EXT = TEXT(".pak");
	const TCHAR* PAK_UNINSTALEED_EXT = TEXT(".pak_uninstalled");
	const TCHAR* SPAK_EXT = TEXT(".spak");
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

static bool GetJsonArrayStrings(TSharedPtr<FJsonObject>& JSON, const FString& Name, TArray<FString>& Out)
{
	Out.Empty();
	const TArray<TSharedPtr<FJsonValue>>* ArrayValues;
	if (JSON->TryGetArrayField(Name, ArrayValues))
	{
		for (auto& It : *ArrayValues)
		{
			FString Val;
			if (It->TryGetString(Val))
			{
				Out.Add(Val);
			}
		}
		return true;
	}
	return false;
}

static void SetJsonArrayStrings(TSharedRef<FJsonObject>& JSON, const FString& Name, const TArray<FString>& In)
{
	TArray<TSharedPtr<FJsonValue>> ArrayValues;
	for (auto& It : In) ArrayValues.Add(MakeShared<FJsonValueString>(It));
	JSON->SetArrayField(Name, ArrayValues);
}


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

	Out.PakFileName = FPaths::GetBaseFilename(FileName);
	/*
	if (!JSON->TryGetNumberField(TEXT("PakOrder"), Out.PakOrder))
	{
		UE_LOG(LogSoda, Error, TEXT("FPakUtils::LoadFromJson(%s); Can't find \"PakOrder\""), *FileName);
		return false;
	}
	*/
	if (!JSON->TryGetStringField(TEXT("PakName"), Out.PakName))
	{
		Out.PakName = Out.PakName;
	}
	JSON->TryGetStringField(TEXT("Description"), Out.Description);
	JSON->TryGetStringField(TEXT("Category"), Out.Category);
	JSON->TryGetNumberField(TEXT("PakVersion"), Out.PakVersion);
	JSON->TryGetStringField(TEXT("EngineVersion"), Out.EngineVersion);
	JSON->TryGetStringField(TEXT("CreatedByURL"), Out.CreatedByURL);
	JSON->TryGetStringField(TEXT("CreatedBy"), Out.CreatedBy);
	JSON->TryGetStringField(TEXT("CustomConfig"), Out.CustomConfig);
	//JSON->TryGetBoolField(TEXT("IsEnabled"), Out.bIsEnabled);
	//GetJsonArrayStrings(JSON, TEXT("MountPoints"), Out.MountPoints);
	GetJsonArrayStrings(JSON, TEXT("PakBlackListNames"), Out.PakBlackListNames);
	GetJsonArrayStrings(JSON, TEXT("MapsWildcardFilters"), Out.MapsWildcardFilters);
	return true;
}

static bool SaveToJson(const FString& FileName, const FSodaPakDescriptor& In)
{
	TSharedRef<FJsonObject> JSON = MakeShared<FJsonObject>();

	//JSON->SetNumberField(TEXT("PakOrder"), In.PakOrder);
	JSON->SetStringField(TEXT("PakName"), In.PakName);
	JSON->SetStringField(TEXT("Description"), In.Description);
	JSON->SetStringField(TEXT("Category"), In.Category);
	JSON->SetNumberField(TEXT("PakVersion"), In.PakVersion);
	JSON->SetStringField(TEXT("EngineVersion"), In.EngineVersion);
	JSON->SetStringField(TEXT("CreatedByURL"), In.CreatedByURL);
	JSON->SetStringField(TEXT("CreatedBy"), In.CreatedBy);
	JSON->SetStringField(TEXT("CustomConfig"), In.CustomConfig);
	//JSON->SetBoolField(TEXT("IsEnabled"), In.bIsEnabled);
	//SetJsonArrayStrings(JSON, TEXT("MountPoints"), In.MountPoints);
	SetJsonArrayStrings(JSON, TEXT("PakBlackListNames"), In.PakBlackListNames);
	SetJsonArrayStrings(JSON, TEXT("MapsWildcardFilters"), In.MapsWildcardFilters);

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
	InstalledPakFile = BaseDir / Descriptor.PakFileName + PAK_EXT;
	UninstalledPakFile = BaseDir / Descriptor.PakFileName + PAK_UNINSTALEED_EXT;

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

static bool LoadCustomConfig(const FString& CustomConfig, const FString& IniName)
{ 
	FConfigContext Contet = FConfigContext::ForceReloadIntoGConfig();
	Contet.OverrideLayers = TArray<FConfigLayer>(GConfigLayers, UE_ARRAY_COUNT(GConfigLayers));
	int CustomConfigInd = Contet.OverrideLayers.IndexOfByPredicate([](const auto& It) { return FString(TEXT("CustomConfig")) == It.EditorName; });
	check(CustomConfigInd >= 0);
	FString Layer = FString::Printf(TEXT("{PROJECT}/Config/Custom/%s/Default{TYPE}.ini"), *CustomConfig);
	Contet.OverrideLayers.Insert({ TEXT("PakConfig"), *Layer }, CustomConfigInd);
	FString ConfigName;
	UE_LOG(LogSodaPak, Log, TEXT("LoadCustomConfig(); Layer:\"%s\"; IniName: \"%s\""), *Layer, *IniName);
	return Contet.Load(*IniName, ConfigName);
}

static void LoadAllCustomConfigs(const FString& CustomConfig)
{
	FString CustomConfigDir = FPaths::ProjectConfigDir() / TEXT("Custom") / CustomConfig;
	TArray<FString>FoundIni;
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	FFindByWildcardVisitor Visitor(TEXT("*Default*.ini"), FoundIni);
	PlatformFile.IterateDirectoryRecursively(*CustomConfigDir, Visitor);
	for (const FString & Ini : FoundIni)
	{
		FString IniName = FPaths::GetBaseFilename(Ini);
		IniName.RemoveFromStart(TEXT("Default"));
		LoadCustomConfig(CustomConfig, IniName);
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
			Desc.PakName = Desc.PakName = PakName;
			SaveToJson(SPakPath, Desc);
			TSharedPtr<FSodaPak> Pak = MakeShared<FSodaPak>(Desc, GetSodaPakDir());
			SodaPaks.Add(Pak);
		}
	}

	for (const auto& Pak : SodaPaks)
	{
		if (Pak->GetInstallStatus() == ESodaPakInstallStatus::Installed)
		{
			if (!Pak->GetDescriptor().CustomConfig.IsEmpty())
			{
				LoadAllCustomConfigs(Pak->GetDescriptor().CustomConfig);
			}
		}
	}

	//LoadAllCustomConfigs(TEXT("CitySample"));
}

void FSodaPakModule::ShutdownModule()
{
}


DEFINE_LOG_CATEGORY(LogSodaPak);
IMPLEMENT_MODULE(FSodaPakModule, SodaPak)