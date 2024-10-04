
#include "Interfaces/IPluginManager.h"

class FSodaProtoV1Module : public IModuleInterface
{
	virtual void StartupModule() override
	{
	}

	virtual void ShutdownModule() override
	{
	}
};


IMPLEMENT_MODULE(FSodaProtoV1Module, SodaProtoV1)
