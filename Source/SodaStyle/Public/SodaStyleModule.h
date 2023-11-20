#pragma once

#include "Modules/ModuleManager.h"

class SODASTYLE_API FSodaStyleModule : public IModuleInterface
{
public:
	virtual void StartupModule();
	virtual void ShutdownModule();
};
