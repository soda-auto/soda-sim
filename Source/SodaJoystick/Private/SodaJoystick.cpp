#include "SodaJoystick.h"
#include "SDLJoystickDevice.h"

// Settings
#include "JoystickGameSettings.h"
#include "Developer/Settings/Public/ISettingsModule.h"
#include "Developer/Settings/Public/ISettingsSection.h"
#include "Developer/Settings/Public/ISettingsContainer.h"

#define LOCTEXT_NAMESPACE "FSodaJoystickPlugin"

DEFINE_LOG_CATEGORY(SodaJoystick);

class FSodaJoystick : public ISodaJoystickPlugin
{
public:
	/** Implements the rest of the IInputDeviceModule interface **/
	
	/** Creates a new instance of the IInputDevice associated with this IInputDeviceModule **/
	virtual TSharedPtr<class IInputDevice> CreateInputDevice(const TSharedRef<FGenericApplicationMessageHandler>& InMessageHandler) override;

	virtual void StartupModule() override
	{
		ISodaJoystickPlugin::StartupModule();
		RegisterSettings();
	}

	void RegisterSettings()
	{
		// Registering some settings is just a matter of exposing the default UObject of
		// your desired class, feel free to add here all those settings you want to expose
		// to your LDs or artists.

		if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
		{
			// Create the new category
			ISettingsContainerPtr SettingsContainer = SettingsModule->GetContainer("Editor");

			SettingsContainer->DescribeCategory("JoystickSettings",
				LOCTEXT("RuntimeWDCategoryName", "JoystickSettings"),
				LOCTEXT("RuntimeWDCategoryDescription", "Game configuration for the JoystickSettings game module"));

			// Register the settings
			ISettingsSectionPtr SettingsSection = SettingsModule->RegisterSettings("Editor", "JoystickSettings", "General",
				LOCTEXT("RuntimeGeneralSettingsName", "General"),
				LOCTEXT("RuntimeGeneralSettingsDescription", "Configuration for Soda Joystick Plugin."),
				GetMutableDefault<UJoystickGameSettings>()
				);

			// Register the save handler to your settings, you might want to use it to
			// validate those or just act to settings changes.
			if (SettingsSection.IsValid())
			{
				SettingsSection->OnModified().BindRaw(this, &FSodaJoystick::HandleSettingsApplyedAndSaved);
			}
		}
	}

	void UnregisterSettings()
	{
		// Ensure to unregister all of your registered settings here, hot-reload would
		// otherwise yield unexpected results.

		if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
		{
			SettingsModule->UnregisterSettings("Editor", "JoystickSettings", "General");
		}
	}

	virtual bool SupportsDynamicReloading() override
	{
		return true;
	}

	virtual void RotateToPosition(float Position) override
	{
		if (SDLJoystickDevice.IsValid())
			SDLJoystickDevice->RotateToPosition(Position);
	}

	virtual void ApplyConstantForce(int ForceVal) override
	{
		if (SDLJoystickDevice.IsValid())
			SDLJoystickDevice->ApplyConstantForce(ForceVal);
	}

	virtual void StopConstantForceEffect() override
	{
		if (SDLJoystickDevice.IsValid())
			SDLJoystickDevice->StopConstantForceEffect();
	}

	virtual void ApplyBumpEffect(int ForceVal, int Len) override
	{
		if (SDLJoystickDevice.IsValid())
			SDLJoystickDevice->ApplyBumpEffect(ForceVal, Len);
	}

	virtual int GetJoyNum() const override
	{
		if (SDLJoystickDevice.IsValid())
			return SDLJoystickDevice->GetJoyNum();
		return 0;
	}

	virtual void Reset() override
	{
		if (SDLJoystickDevice.IsValid())
			return SDLJoystickDevice->CloseDevice();
	}

	virtual void UpdateSettings() override
	{
		if (SDLJoystickDevice.IsValid())
		{
			UJoystickGameSettings* Settings = GetMutableDefault<UJoystickGameSettings>();
			if (Settings->GetAnalogAxesNum() > 0 && SDLJoystickDevice)
			{
				check(SDLJoystickDevice.IsValid());
				SDLJoystickDevice->SetCentringForce(Settings->CentringForcePercent);
				SDLJoystickDevice->ActualizeAxesNum();
				SDLJoystickDevice->ActualizeButtonsNum();
			}
		}
	}

private:

	// Callback for when the settings were saved.
	bool HandleSettingsApplyedAndSaved()
	{
		UJoystickGameSettings* Settings = GetMutableDefault<UJoystickGameSettings>();
		Settings->SaveConfig();
		UpdateSettings();
		return true;
	}

	/** Called before the module is unloaded, right before the module object is destroyed. **/
	virtual void ShutdownModule() override;

	TSharedPtr< class FSDLJoystickDevice > SDLJoystickDevice;
};

TSharedPtr<class IInputDevice> FSodaJoystick::CreateInputDevice(const TSharedRef<FGenericApplicationMessageHandler>& InMessageHandler)
{
	UE_LOG(SodaJoystick, Log, TEXT("Created new input device!"));

	// See GenericInputDevice.h for the definition of the IInputDevice we are returning here
	SDLJoystickDevice = MakeShareable(new FSDLJoystickDevice(InMessageHandler));

	check(SDLJoystickDevice.IsValid());
	return SDLJoystickDevice;
}

void FSodaJoystick::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.

	UE_LOG(SodaJoystick, Log, TEXT("Shutdown Module"));

	if (UObjectInitialized())
	{
		UnregisterSettings();
	}

	// Close if opened
	
	if (SDLJoystickDevice.IsValid())
		SDLJoystickDevice->CloseDevice();
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FSodaJoystick, SodaJoystick)