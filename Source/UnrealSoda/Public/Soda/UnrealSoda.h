// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "Modules/ModuleManager.h"

# pragma warning(disable:4996)

class UNREALSODA_API FUnrealSodaModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	TArray<void*> LoadedDlls;
};

UNREALSODA_API DECLARE_LOG_CATEGORY_EXTERN(LogSoda, Log, All);