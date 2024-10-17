// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/VehicleComponents/Mechanicles/VehicleDifferentialComponent.h"
#include "Soda/UnrealSoda.h"
#include "Soda/Vehicles/SodaWheeledVehicle.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Soda/Vehicles/IWheeledVehicleMovementInterface.h"
#include "Soda/DBGateway.h"

#include "bsoncxx/builder/stream/helpers.hpp"
#include "bsoncxx/exception/exception.hpp"
#include "bsoncxx/builder/stream/document.hpp"
#include "bsoncxx/builder/stream/array.hpp"
#include "bsoncxx/json.hpp"

UVehicleDifferentialBaseComponent::UVehicleDifferentialBaseComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	GUI.Category = TEXT("Vehicle Mechanicles");
	GUI.IcanName = TEXT("SodaIcons.Differential");

	Common.Activation = EVehicleComponentActivation::OnStartScenario;
}

UVehicleDifferentialSimpleComponent::UVehicleDifferentialSimpleComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	GUI.ComponentNameOverride = TEXT("Differential Simple");
	GUI.bIsPresentInAddMenu = true;
}

bool UVehicleDifferentialSimpleComponent::OnActivateVehicleComponent()
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

void UVehicleDifferentialSimpleComponent::OnDeactivateVehicleComponent()
{
	Super::OnDeactivateVehicleComponent();
}

void UVehicleDifferentialSimpleComponent::PassTorque(float InTorque)
{
	if (GetHealth() == EVehicleComponentHealth::Ok)
	{
		InTorq = InTorque;
		OutTorq = InTorque * Ratio * 0.5;

		switch (DifferentialType)
		{
		case EVehicleDifferentialType::Open_FrontDrive:
			GetWheeledVehicle()->GetWheelByIndex(EWheelIndex::Ind0_FL)->ReqTorq += OutTorq;
			GetWheeledVehicle()->GetWheelByIndex(EWheelIndex::Ind1_FR)->ReqTorq += OutTorq;
			break;

		case EVehicleDifferentialType::Open_RearDrive:
			GetWheeledVehicle()->GetWheelByIndex(EWheelIndex::Ind2_RL)->ReqTorq += OutTorq;
			GetWheeledVehicle()->GetWheelByIndex(EWheelIndex::Ind3_RR)->ReqTorq += OutTorq;
			break;
		}
	}
	else
	{
		InTorq = 0;
		OutTorq = 0;
	}
}

float UVehicleDifferentialSimpleComponent::ResolveAngularVelocity() const
{
	if (GetHealth() == EVehicleComponentHealth::Ok)
	{
		switch (DifferentialType)
		{
		case EVehicleDifferentialType::Open_FrontDrive:
			InAngularVelocity = std::max(GetWheeledVehicle()->GetWheelByIndex(EWheelIndex::Ind0_FL)->AngularVelocity, GetWheeledVehicle()->GetWheelByIndex(EWheelIndex::Ind1_FR)->AngularVelocity);
			break;
		case EVehicleDifferentialType::Open_RearDrive:
			InAngularVelocity = std::max(GetWheeledVehicle()->GetWheelByIndex(EWheelIndex::Ind2_RL)->AngularVelocity, GetWheeledVehicle()->GetWheelByIndex(EWheelIndex::Ind3_RR)->AngularVelocity);
			break;
		}
		OutAngularVelocity = InAngularVelocity * Ratio;
		return OutAngularVelocity;
	}
	return 0;
}

bool UVehicleDifferentialSimpleComponent::FindWheelRadius(float& OutRadius) const
{
	if (GetHealth() == EVehicleComponentHealth::Ok)
	{
		switch (DifferentialType)
		{
		case EVehicleDifferentialType::Open_FrontDrive:
			OutRadius = (GetWheeledVehicle()->GetWheelByIndex(EWheelIndex::Ind0_FL)->Radius + GetWheeledVehicle()->GetWheelByIndex(EWheelIndex::Ind1_FR)->Radius) / 2.0;
			return true;
		case EVehicleDifferentialType::Open_RearDrive:
			OutRadius = (GetWheeledVehicle()->GetWheelByIndex(EWheelIndex::Ind2_RL)->Radius + GetWheeledVehicle()->GetWheelByIndex(EWheelIndex::Ind3_RR)->Radius) / 2.0;
			return true;
		}
	}
	return false;
}

void UVehicleDifferentialSimpleComponent::DrawDebug(UCanvas* Canvas, float& YL, float& YPos)
{
	Super::DrawDebug(Canvas, YL, YPos);

	if (Common.bDrawDebugCanvas)
	{
		UFont* RenderFont = GEngine->GetSmallFont();
		Canvas->SetDrawColor(FColor::White);
		YPos += Canvas->DrawText(RenderFont, FString::Printf(TEXT("InTorq: %.2f "), InTorq), 16, YPos);
		YPos += Canvas->DrawText(RenderFont, FString::Printf(TEXT("OutTorq: %.2f "), OutTorq), 16, YPos);
		YPos += Canvas->DrawText(RenderFont, FString::Printf(TEXT("InAngVel: %.2f "), InAngularVelocity), 16, YPos);
		YPos += Canvas->DrawText(RenderFont, FString::Printf(TEXT("OutAngVel: %.2f "), OutAngularVelocity), 16, YPos);
	}
}

void UVehicleDifferentialSimpleComponent::OnPushDataset(soda::FActorDatasetData& Dataset) const
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
			<< "InTorq" << InTorq
			<< "OutTorq" << OutTorq
			<< "InAngVel" << InAngularVelocity
			<< "OutAngVel" << OutAngularVelocity
			<< close_document;
	}
	catch (const std::system_error& e)
	{
		UE_LOG(LogSoda, Error, TEXT("URacingSensor::OnPushDataset(); %s"), UTF8_TO_TCHAR(e.what()));
	}
}
