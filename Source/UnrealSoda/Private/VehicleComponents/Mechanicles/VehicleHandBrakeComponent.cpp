// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/VehicleComponents/Mechanicles/VehicleHandBrakeComponent.h"
#include "Soda/UnrealSoda.h"
#include "Soda/Vehicles/SodaWheeledVehicle.h"
#include "Soda/Vehicles/IWheeledVehicleMovementInterface.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Soda/DBGateway.h"

#include "bsoncxx/builder/stream/helpers.hpp"
#include "bsoncxx/exception/exception.hpp"
#include "bsoncxx/builder/stream/document.hpp"
#include "bsoncxx/builder/stream/array.hpp"
#include "bsoncxx/json.hpp"


UVehicleHandBrakeBaseComponent::UVehicleHandBrakeBaseComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	GUI.Category = TEXT("Vehicle Mechanicles");
	Common.Activation = EVehicleComponentActivation::OnStartScenario;
}

UVehicleHandBrakeSimpleComponent::UVehicleHandBrakeSimpleComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	GUI.ComponentNameOverride = TEXT("Hand Brake Simple");
	GUI.IcanName = TEXT("SodaIcons.Braking");
	GUI.bIsPresentInAddMenu = true;
}

bool UVehicleHandBrakeSimpleComponent::OnActivateVehicleComponent()
{
	if (!Super::OnActivateVehicleComponent())
	{
		return false;
	}

	if (!GetWheeledVehicle()->Is4WDVehicle())
	{
		SetHealth(EVehicleComponentHealth::Error, TEXT("Support only 4WD vehicles"));
		return false;
	}

	return true;
}

void UVehicleHandBrakeSimpleComponent::OnDeactivateVehicleComponent()
{
	Super::OnDeactivateVehicleComponent();
}

void UVehicleHandBrakeSimpleComponent::RequestByRatio(float InRatio)
{
	if (GetHealth() == EVehicleComponentHealth::Ok)
	{
		switch (HandBrakeMode)
		{
		case EHandBrakeMode::FrontWheels:
			GetWheeledVehicle()->GetWheel4WD(E4WDWheelIndex::FL)->ReqBrakeTorque += MaxHandBrakeTorque * InRatio;
			GetWheeledVehicle()->GetWheel4WD(E4WDWheelIndex::FR)->ReqBrakeTorque += MaxHandBrakeTorque * InRatio;
			break;
		case EHandBrakeMode::RearWheels:
			GetWheeledVehicle()->GetWheel4WD(E4WDWheelIndex::RL)->ReqBrakeTorque += MaxHandBrakeTorque * InRatio;
			GetWheeledVehicle()->GetWheel4WD(E4WDWheelIndex::RR)->ReqBrakeTorque += MaxHandBrakeTorque * InRatio;
			break;
		case EHandBrakeMode::FourWheel:
			GetWheeledVehicle()->GetWheel4WD(E4WDWheelIndex::FL)->ReqBrakeTorque += MaxHandBrakeTorque * InRatio;
			GetWheeledVehicle()->GetWheel4WD(E4WDWheelIndex::FR)->ReqBrakeTorque += MaxHandBrakeTorque * InRatio;
			GetWheeledVehicle()->GetWheel4WD(E4WDWheelIndex::RL)->ReqBrakeTorque += MaxHandBrakeTorque * InRatio;
			GetWheeledVehicle()->GetWheel4WD(E4WDWheelIndex::RR)->ReqBrakeTorque += MaxHandBrakeTorque * InRatio;
			break;
		}

		CurrentRatio = 0;
	}
}

void UVehicleHandBrakeSimpleComponent::DrawDebug(UCanvas* Canvas, float& YL, float& YPos)
{
	Super::DrawDebug(Canvas, YL, YPos);

	if (Common.bDrawDebugCanvas)
	{
		UFont* RenderFont = GEngine->GetSmallFont();
		Canvas->SetDrawColor(FColor::White);
		YPos += Canvas->DrawText(RenderFont, FString::Printf(TEXT("Ratio: %.2f"), CurrentRatio), 16, YPos);
	}
}

void UVehicleHandBrakeSimpleComponent::OnPushDataset(soda::FActorDatasetData& Dataset) const
{
	using bsoncxx::builder::stream::document;
	using bsoncxx::builder::stream::finalize;
	using bsoncxx::builder::stream::open_document;
	using bsoncxx::builder::stream::close_document;
	using bsoncxx::builder::stream::open_array;
	using bsoncxx::builder::stream::close_array;

	try
	{
		Dataset.GetRowDoc()
			<< std::string(TCHAR_TO_UTF8(*GetName())) << open_document
			<< "Ratio" << CurrentRatio
			<< close_document;
	}
	catch (const std::system_error& e)
	{
		UE_LOG(LogSoda, Error, TEXT("URacingSensor::OnPushDataset(); %s"), UTF8_TO_TCHAR(e.what()));
	}
}
