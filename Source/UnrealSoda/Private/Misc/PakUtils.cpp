
#include "Soda/Misc/PakUtils.h"
#include "Soda/UnrealSoda.h"
#include "GenericPlatform/GenericPlatformFile.h"
#include "IPlatformFilePak.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonWriter.h"
#include "Serialization/JsonSerializer.h"

static FString GetSodaPakDir()
{
	return FPaths::ProjectSavedDir() / TEXT("Paks");
}

class FFindSPakFilesVisitor : public IPlatformFile::FDirectoryVisitor
{
	TArray<FString>& FoundFiles;
public:
	FFindSPakFilesVisitor(TArray<FString>& InFoundFiles)
		: FoundFiles(InFoundFiles)
	{}
	virtual bool Visit(const TCHAR* FilenameOrDirectory, bool bIsDirectory)
	{
		if (bIsDirectory == false)
		{
			FString Filename(FilenameOrDirectory);
			if (Filename.MatchesWildcard(TEXT("*.spak")) && !FoundFiles.Contains(Filename))
			{
				FoundFiles.Add(Filename);
			}
		}
		return true;
	}
};

class FFindByWildcardVisitor : public IPlatformFile::FDirectoryVisitor
{
	const FString& Wildcard;
	TArray<FString>& FoundFiles;
public:
	FFindByWildcardVisitor(const FString & Wildcard, TArray<FString>& InFoundFiles)
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
		UE_LOG(LogSoda, Error, TEXT("FPakUtils::LoadFromJson(%s); Can't read file"), *FileName);
		return false;
	}

	TSharedPtr<FJsonObject> JSON;
	TSharedRef<TJsonReader<TCHAR>> Reader = TJsonReaderFactory<TCHAR>::Create(JsonString);
	if (!FJsonSerializer::Deserialize(Reader, JSON))
	{
		UE_LOG(LogSoda, Error, TEXT("FPakUtils::LoadFromJson(%s); Can't JSON deserialize "), *FileName);
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
		UE_LOG(LogSoda, Error, TEXT("FPakUtils::LoadFromJson(%s); Can't find \"FriendlyName\""), *FileName);
		return false;
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
		UE_LOG(LogSoda, Error, TEXT("SaveToJson(); Can't serialize"));
		return false;
	}

	if (!FFileHelper::SaveStringToFile(JsonString, *FileName))
	{
		UE_LOG(LogSoda, Error, TEXT("SaveToJson(); Can't write to '%s' file"), *FileName);
		return false;
	}
	return true;
}

FSodaPak::FSodaPak(const FSodaPakDescriptor& Descriptor, const FString& BaseDir)
	: Descriptor(Descriptor)
	, BaseDir(BaseDir)
{
	InstalledPakFile = BaseDir / Descriptor.PakName + TEXT(".pak");
	UninstalledPakFile = BaseDir / Descriptor.PakName + TEXT(".pak_uninstalled");

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

void FSodaPakLoader::Initialize()
{
	TArray<FString>	FoundSPaks;
	FFindSPakFilesVisitor PakVisitor(FoundSPaks);
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	PlatformFile.IterateDirectoryRecursively(*GetSodaPakDir(), PakVisitor);
	TSet<FString> PakFileNames;
	for (const FString& SPakPath : FoundSPaks)
	{
		UE_LOG(LogSoda, Log, TEXT("FSodaPakLoader::Initialize(); Found: \"%s\""), *SPakPath);
		FSodaPakDescriptor Desc;
		if (LoadFromJson(SPakPath, Desc))
		{
			TSharedPtr<FSodaPak> Pak = MakeShared<FSodaPak>(Desc, GetSodaPakDir());
			SodaPaks.Add(Pak);
		}
	}

	if (FoundSPaks.IsEmpty())
	{
		UE_LOG(LogSoda, Warning, TEXT("FSodaPakLoader::Initialize(); Can't find any .spak in \"%s\""), *GetSodaPakDir());
	}
}
