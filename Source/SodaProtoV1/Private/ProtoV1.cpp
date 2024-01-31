
#include "Interfaces/IPluginManager.h"

class FProtoV1Module : public IModuleInterface
{
	virtual void StartupModule() override
	{
	}

	virtual void ShutdownModule() override
	{
	}
};


IMPLEMENT_MODULE(FProtoV1Module, ProtoV1)
