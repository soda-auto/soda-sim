// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.


#include "Soda/VehicleComponents/Others/LightSet.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Components/LightComponent.h"

void FVehicleLightItem::Enable( bool bNewEnabled, bool bForce)
{
	if (bNewEnabled != bEnabled || bForce)
	{
		bEnabled = bNewEnabled;
		for (auto& It : Materials)
		{
			if(IsValid(It)) It->SetScalarParameterValue(TEXT("Intencity"), bEnabled ? 1.0 : 0.0);
		}

		for (auto& It : Lights)
		{
			if (IsValid(It)) It->SetVisibility(bEnabled);
		}

		if (auto* Exchange = IOPinSetup.GetExchange())
		{
			Exchange->SetValue(FIOExchangeValue {
				EIOExchangeMode::Fixed,
				bNewEnabled,
				bNewEnabled ? 12.0f : 0.0f,
				0, 0
			});
		}
	}

}

ULightSetMode1Component::ULightSetMode1Component(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void ULightSetMode1Component::EnableAll(bool bEnabled, bool bForce)
{
	for (auto& It : LightItems)
	{
		It->Enable(bEnabled, bForce);
	}
}

bool ULightSetMode1Component::OnActivateVehicleComponent()
{
	if (!Super::OnActivateVehicleComponent())
	{
		return false;
	}

	LightItems.Add(&HeightL);
	LightItems.Add(&HeightR);
	LightItems.Add(&LowL);
	LightItems.Add(&LowR);
	LightItems.Add(&FogL);
	LightItems.Add(&FogR);
	LightItems.Add(&DaytimeFL);
	LightItems.Add(&DaytimeFR);
	LightItems.Add(&DaytimeRL);
	LightItems.Add(&DaytimeRR);
	LightItems.Add(&BrakeL);
	LightItems.Add(&BrakeR);
	LightItems.Add(&ReversL);
	LightItems.Add(&ReversR);

	EnableAll(false, true);

	for (auto& It : LightItems)
	{
		if (It->bIOPinUsed)
		{
			It->IOPinSetup.Bind(GetOwner());
		}
	}

	return true;
}

void ULightSetMode1Component::OnDeactivateVehicleComponent()
{
	Super::OnDeactivateVehicleComponent();
}

ULightSetFunctionLibrary::ULightSetFunctionLibrary(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}