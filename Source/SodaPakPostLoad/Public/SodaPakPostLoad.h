#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class SODAPAKPOSTLOAD_API FSodaPakPostLoadModule : public IModuleInterface
{
public:
	static inline FSodaPakPostLoadModule& Get()
	{
		return FModuleManager::LoadModuleChecked< FSodaPakPostLoadModule >( "SodaPakPostLoad" );
	}

	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
