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

	UObject* TorqueTransmissionObject1 = LinkToTorqueTransmission1.GetObject<UObject>(GetOwner());
	ITorqueTransmission* TorqueTransmissionInterface1 = Cast<ITorqueTransmission>(TorqueTransmissionObject1);
	if (TorqueTransmissionObject1 && TorqueTransmissionInterface1)
	{
		OutputTorqueTransmission1.SetInterface(TorqueTransmissionInterface1);
		OutputTorqueTransmission1.SetObject(TorqueTransmissionObject1);
	}
	else
	{
		SetHealth(EVehicleComponentHealth::Error, TEXT("Transmission1 isn't connected"));
		return false;
	}

	UObject* TorqueTransmissionObject2 = LinkToTorqueTransmission2.GetObject<UObject>(GetOwner());
	ITorqueTransmission* TorqueTransmissionInterface2 = Cast<ITorqueTransmission>(TorqueTransmissionObject2);
	if (TorqueTransmissionObject2 && TorqueTransmissionInterface2)
	{
		OutputTorqueTransmission2.SetInterface(TorqueTransmissionInterface2);
		OutputTorqueTransmission2.SetObject(TorqueTransmissionObject2);
	}
	else
	{
		SetHealth(EVehicleComponentHealth::Error, TEXT("Transmission2 isn't connected"));
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

		OutputTorqueTransmission1->PassTorque(OutTorq);
		OutputTorqueTransmission2->PassTorque(OutTorq);
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
		InAngularVelocity = std::max(OutputTorqueTransmission1->ResolveAngularVelocity(), OutputTorqueTransmission2->ResolveAngularVelocity());
		OutAngularVelocity = InAngularVelocity * Ratio;
		return OutAngularVelocity;
	}
	return 0;
}

bool UVehicleDifferentialSimpleComponent::FindWheelRadius(float& OutRadius) const
{
	if (GetHealth() == EVehicleComponentHealth::Ok)
	{
		float Radius1, Radius2;
		if (OutputTorqueTransmission1->FindWheelRadius(Radius1) && OutputTorqueTransmission2->FindWheelRadius(Radius2))
		{
			OutRadius = (Radius1 + Radius1) / 2;
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
