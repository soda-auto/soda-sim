// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.
#include "Soda/VehicleComponents/IODev.h"
#include "Soda/VehicleComponents/IOBus.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"

UIODevComponent::UIODevComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	GUI.Category = TEXT("IO");
	GUI.IcanName = TEXT("SodaIcons.Net");
	//GUI.ComponentNameOverride = TEXT("IO Dev");
	//GUI.bIsPresentInAddMenu = true;

	Common.bIsTopologyComponent = true;
	Common.Activation = EVehicleComponentActivation::OnStartScenario;
}

void UIODevComponent::OnPreActivateVehicleComponent()
{
	Super::OnPreActivateVehicleComponent();

	IOBus = LinkToIOBus.GetObject<UIOBusComponent>(GetOwner());
	if (IOBus)
	{
		IOBus->RegisterIODev(this);
	}
}

bool UIODevComponent::OnActivateVehicleComponent()
{
	bool bRet = Super::OnActivateVehicleComponent();

	return bRet;
}

void UIODevComponent::OnDeactivateVehicleComponent()
{
	Super::OnDeactivateVehicleComponent();

	if (IsValid(IOBus))
	{
		IOBus->UnregisterIODev(this);
	}
	IOBus = nullptr;
}

FString UIODevComponent::GetRemark() const
{
	return "Bus:" + LinkToIOBus.PathToSubobject;
}

void UIODevComponent::DrawDebug(UCanvas* Canvas, float& YL, float& YPos)
{
	Super::DrawDebug(Canvas, YL, YPos);

	if (Common.bDrawDebugCanvas)
	{
		UFont* RenderFont = GEngine->GetSmallFont();
		Canvas->SetDrawColor(FColor::White);
		YPos += Canvas->DrawText(RenderFont, FString::Printf(TEXT("Bus: %s"), *LinkToIOBus.PathToSubobject), 16, YPos);

	}
}

