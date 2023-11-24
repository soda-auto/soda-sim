#include "SodaStyleModule.h"
#include "SodaStyle.h"
#include "Styling/CoreStyle.h"

void FSodaStyleModule::StartupModule()
{
	check(FCoreStyle::IsStarshipStyle());
	FStarshipSodaStyle::Initialize();
}

void FSodaStyleModule::ShutdownModule()
{
	FStarshipSodaStyle::Shutdown();
}

IMPLEMENT_MODULE(FSodaStyleModule, SodaStyle);

