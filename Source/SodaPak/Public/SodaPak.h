#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

DECLARE_LOG_CATEGORY_EXTERN(LogSodaPak, Log, All);

struct SODAPAK_API FSodaPakDescriptor
{
	/** File name of the pak in the Saved/SodaPaks folder without extension */
	FString PakName{};

	/** Content pak priority */
	//int32 PakOrder = 0;

	/** Friendly name of the pak */
	FString FriendlyName{};

	/** Description of the pak */
	FString Description{};

	/** The name of the category this pak */
	FString Category{};

	/** Version number for the pak.  The version number must increase with every version of the pak, so that the system
		can determine whether one version of a pak is newer than another, or to enforce other requirements.  */
	int32 PakVersion = 0;

	/** Version of the engine that this pak is compatible with */
	FString EngineVersion{};

	FString CreatedBy{};

	FString CreatedByURL{};

	//TArray<FString> MountPoints{};

	//bool bIsEnabled = false;
};

enum ESodaPakInstallStatus
{
	Uninstalled,
	Installed,
	Broken
};


class SODAPAK_API FSodaPak
{
public:
	FSodaPak(const FSodaPakDescriptor& Descriptor, const FString& BaseDir);

	const FSodaPakDescriptor& GetDescriptor() const { return Descriptor; }
	ESodaPakInstallStatus GetInstallStatus() const { return InstallStatus; }
	bool IsMounted() const { return bIsMounted; }
	const FString& GetBaseDir() const { return BaseDir; }
	FString GetIconFileName() const { return GetBaseDir() / GetDescriptor().PakName + TEXT(".png"); }

	bool Install();
	bool Uninstall();

	void UpdateInstallStatus();
	void UpdateMountStatus();

private:
	FSodaPakDescriptor Descriptor{};
	FString BaseDir;
	FString InstalledPakFile;
	FString UninstalledPakFile;

	ESodaPakInstallStatus InstallStatus = ESodaPakInstallStatus::Uninstalled;
	bool bIsMounted = false;
};



class SODAPAK_API FSodaPakModule : public IModuleInterface
{
public:
	static inline FSodaPakModule& Get()
	{
		return FModuleManager::LoadModuleChecked< FSodaPakModule >( "SodaPak" );
	}

	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	const TArray<TSharedPtr<FSodaPak>>& GetSodaPaks() const { return SodaPaks; }

private:
	TArray<TSharedPtr<FSodaPak>> SodaPaks;

};
