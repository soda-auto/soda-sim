#include "Soda/SodaUserSettings.h"
#include "Misc/ConfigContext.h"
#include "RuntimeMetaData.h"

FText USodaUserSettings::GetMenuItemText() const
{
	return FRuntimeMetaData::GetDisplayNameText(GetClass());
}

FText USodaUserSettings::GetMenuItemDescription() const
{
	return FRuntimeMetaData::GetToolTipText(GetClass(), true);
}

FName USodaUserSettings::GetMenuItemIconName() const
{
	static FName IconName = "Icons.Adjust";
	return IconName;
}

TSharedPtr<SWidget> USodaUserSettings::GetCustomSettingsWidget() const
{
	return TSharedPtr<SWidget>();
}

void USodaUserSettings::ResetToDefault()
{
	/*
	if (GetClass()->HasAnyClassFlags(CLASS_Config)
		// && !GetClass()->HasAnyClassFlags(CLASS_DefaultConfig | CLASS_GlobalUserConfig | CLASS_ProjectUserConfig) 
		)
	{
		FString ConfigName = GetClass()->GetConfigName();

		GConfig->EmptySection(*GetClass()->GetPathName(), ConfigName);
		GConfig->Flush(false);

		FConfigContext::ForceReloadIntoGConfig().Load(*FPaths::GetBaseFilename(ConfigName));

		ReloadConfig(nullptr, nullptr, UE::LCPF_PropagateToInstances | UE::LCPF_PropagateToChildDefaultObjects);
	}
	*/
}