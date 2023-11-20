#pragma once

#include "Modules/ModuleManager.h"

class SODALOADINGSCREEN_API FSodaLoadingScreenModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	virtual bool IsGameModule() const override;

private:
	/**
	 * Loading screen callback, it won't be called if we've already explicitly setup the loading screen
	 */
	void PreSetupLoadingScreen();

	/**
	 * Setup loading screen settings 
	 */
	void SetupLoadingScreen();
};
