// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/VehicleComponents/CANDev.h"
#include "Soda/VehicleComponents/CANBus.h"
#include "Soda/UnrealSoda.h"
#include "Soda/SodaApp.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Soda/DBC/Serialization.h"

UCANDevComponent::UCANDevComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	GUI.Category = TEXT("CAN");
	GUI.IcanName = TEXT("SodaIcons.CAN");

	Common.bIsTopologyComponent = true;
	Common.Activation = EVehicleComponentActivation::OnStartScenario;
}

void UCANDevComponent::OnPreActivateVehicleComponent()
{
	Super::OnPreActivateVehicleComponent();

	CANBus = LinkToCANBus.GetObject<UCANBusComponent>(GetOwner());
	if (CANBus)
	{
		CANBus->RegisterCanDev(this);
	}
}

bool UCANDevComponent::OnActivateVehicleComponent()
{
	if (!CANBus)
	{
		SetHealth(EVehicleComponentHealth::Warning, TEXT("CAN bus isn't connectd"));
	}

	//RecvMessages.clear();
	//SendMessages.clear();

	return Super::OnActivateVehicleComponent();
}

void UCANDevComponent::OnDeactivateVehicleComponent()
{
	Super::OnDeactivateVehicleComponent();

	if (IsValid(CANBus))
	{
		CANBus->UnregisterCanDev(this);
	}
	CANBus = nullptr;

	//RecvMessages.clear();
	//SendMessages.clear();
}

int UCANDevComponent::SendFrame(const dbc::FCanFrame& CanFrame) 
{ 
	return -1;
}

void UCANDevComponent::ProcessRecvMessage(const TTimestamp& Timestamp, const dbc::FCanFrame& CanFrame)
{
	if (CANBus)
	{
		CANBus->ProcessRecvMessage(Timestamp, CanFrame);
	}
}


FString UCANDevComponent::GetRemark() const
{
	return "Bus:" + LinkToCANBus.PathToSubobject;
}

void UCANDevComponent::DrawDebug(UCanvas* Canvas, float& YL, float& YPos)
{
	Super::DrawDebug(Canvas, YL, YPos);

	if (Common.bDrawDebugCanvas)
	{
		UFont* RenderFont = GEngine->GetSmallFont();
		Canvas->SetDrawColor(FColor::White);
		YPos += Canvas->DrawText(RenderFont, FString::Printf(TEXT("Bus: %s"), *LinkToCANBus.PathToSubobject), 16, YPos);
		
	}
}
