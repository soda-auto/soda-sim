// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"
#include "Modules/ModuleManager.h"

class FSodaSimEditor : public IModuleInterface
{

public:
	virtual void StartupModule() override
	{
	}

	virtual void ShutdownModule() override
	{
	}
};

IMPLEMENT_MODULE(FSodaSimEditor, SodaSimEditor)