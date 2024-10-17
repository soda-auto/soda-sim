// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/VehicleComponents/Mechanicles/VehicleSteeringComponent.h"
#include "Soda/UnrealSoda.h"
#include "Soda/Vehicles/SodaWheeledVehicle.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "VehicleUtility.h"
#include "Soda/Vehicles/IWheeledVehicleMovementInterface.h"
#include "Soda/VehicleComponents/VehicleInputComponent.h"
#include "UObject/ConstructorHelpers.h"
#include <algorithm>
#include "Soda/DBGateway.h"

#include "bsoncxx/builder/stream/helpers.hpp"
#include "bsoncxx/exception/exception.hpp"
#include "bsoncxx/builder/stream/document.hpp"
#include "bsoncxx/builder/stream/array.hpp"
#include "bsoncxx/json.hpp"

UVehicleSteeringRackBaseComponent::UVehicleSteeringRackBaseComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	GUI.Category = TEXT("Vehicle Mechanicles");
	GUI.IcanName = TEXT("SodaIcons.Steering");
	Common.Activation = EVehicleComponentActivation::OnStartScenario;
}

// ------------------------------------------------------------------------------------------------
UVehicleSteeringRackSimpleComponent::UVehicleSteeringRackSimpleComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	GUI.ComponentNameOverride = TEXT("Steering Rack Simple");
	GUI.bIsPresentInAddMenu = true;

	TickData.bAllowVehiclePrePhysTick = true;

	PrimaryComponentTick.bCanEverTick = true;
	bTickInEditor = false;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;

	static ConstructorHelpers::FObjectFinder<UCurveFloat> SteeringCurvePtr(TEXT("/SodaSim/Assets/CPP/Curves/SteeringRack_SteerSpeed.SteeringRack_SteerSpeed"));
	SteeringCurve.ExternalCurve = SteeringCurvePtr.Object;
}

bool UVehicleSteeringRackSimpleComponent::OnActivateVehicleComponent()
{
	if (!Super::OnActivateVehicleComponent())
	{
		return false;
	}

	SteerInputRatio = 0;

	if (!GetWheeledVehicle()->Is4WDVehicle())
	{
		SetHealth(EVehicleComponentHealth::Error, TEXT("Support only 4WD vehicles"));
		return false;
	}

	if (!SteeringCurve.ExternalCurve)
	{
		SetHealth(EVehicleComponentHealth::Error, TEXT("SteeringCurve isn't set"));
		return false;
	}

	return true;
}

void UVehicleSteeringRackSimpleComponent::OnDeactivateVehicleComponent()
{
	Super::OnDeactivateVehicleComponent();

	SteerInputRatio = 0;
}

void UVehicleSteeringRackSimpleComponent::RequestByAngle(float Angle)
{
	TargetSteerAng = FMath::Clamp(Angle, -MaxSteerAngle / 180.0 * M_PI, MaxSteerAngle / 180.0 * M_PI);
}

void UVehicleSteeringRackSimpleComponent::UpdateSteer(float DeltaTime, const FPhysBodyKinematic& VehicleKinematic, const TTimestamp& Timestamp)
{
	float ForwardSpeed = Chaos::CmSToKmH(VehicleKinematic.Curr.GetLocalVelocity().X);

	float SteerSpeed = SteeringCurve.ExternalCurve->GetFloatValue(ForwardSpeed) / 180.0 * M_PI;
	float DeltaSteerReq = TargetSteerAng - CurrentSteerAng;
	float DeltaSteer = SteerSpeed * DeltaTime;

	if (DeltaSteerReq < 0) DeltaSteer = -DeltaSteer;

	CurrentSteerAng += std::abs(DeltaSteerReq) < std::abs(DeltaSteer) ? DeltaSteerReq : DeltaSteer;
	CurrentSteerAng = FMath::Clamp(CurrentSteerAng, -MaxSteerAngle, MaxSteerAngle);

	GetWheeledVehicle()->GetWheelByIndex(EWheelIndex::Ind0_FL)->ReqSteer = CurrentSteerAng;
	GetWheeledVehicle()->GetWheelByIndex(EWheelIndex::Ind1_FR)->ReqSteer = CurrentSteerAng;
}

void UVehicleSteeringRackSimpleComponent::PrePhysicSimulation(float DeltaTime, const FPhysBodyKinematic& VehicleKinematic, const TTimestamp & Timestamp)
{
	Super::PrePhysicSimulation(DeltaTime, VehicleKinematic, Timestamp);

	if (GetHealth() != EVehicleComponentHealth::Ok) return;

	if (bAcceptSteerFromVehicleInput)
	{
		RequestByRatio(SteerInputRatio);
	}

	UpdateSteer(DeltaTime, VehicleKinematic, Timestamp);
}

void UVehicleSteeringRackSimpleComponent::DrawDebug(UCanvas* Canvas, float& YL, float& YPos)
{
	Super::DrawDebug(Canvas, YL, YPos);

	if (Common.bDrawDebugCanvas)
	{
		UFont* RenderFont = GEngine->GetSmallFont();
		Canvas->SetDrawColor(FColor::White);
		YPos += Canvas->DrawText(RenderFont, FString::Printf(TEXT("Current Angle: %.2f "), CurrentSteerAng / M_PI * 180), 16, YPos);
		YPos += Canvas->DrawText(RenderFont, FString::Printf(TEXT("Target Angle:  %.2f "), TargetSteerAng / M_PI * 180), 16, YPos);
		YPos += Canvas->DrawText(RenderFont, FString::Printf(TEXT("Steer Input Ratio:  %.2f "), SteerInputRatio), 16, YPos);
	}
}

void UVehicleSteeringRackSimpleComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bAcceptSteerFromVehicleInput)
	{
		if (UVehicleInputComponent* VehicleInput = GetWheeledVehicle()->GetActiveVehicleInput())
		{
			SteerInputRatio = VehicleInput->GetInputState().Steering;
		}
		else
		{
			SteerInputRatio = 0;
		}
	}
}

void UVehicleSteeringRackSimpleComponent::OnPushDataset(soda::FActorDatasetData& Dataset) const
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
			<< "CurrentSteer" << CurrentSteerAng
			<< "TargetSteer" << TargetSteerAng
			<< close_document;
	}
	catch (const std::system_error& e)
	{
		UE_LOG(LogSoda, Error, TEXT("URacingSensor::OnPushDataset(); %s"), UTF8_TO_TCHAR(e.what()));
	}
}



