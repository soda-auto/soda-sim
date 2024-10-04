// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/VehicleComponents/Mechanicles/VehicleEngineComponent.h"
#include "Soda/UnrealSoda.h"
#include "Soda/Vehicles/SodaWheeledVehicle.h"
#include "Soda/SodaStatics.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Soda/VehicleComponents/VehicleInputComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Soda/DBGateway.h"

#include "bsoncxx/builder/stream/helpers.hpp"
#include "bsoncxx/exception/exception.hpp"
#include "bsoncxx/builder/stream/document.hpp"
#include "bsoncxx/builder/stream/array.hpp"
#include "bsoncxx/json.hpp"

UVehicleEngineBaseComponent::UVehicleEngineBaseComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	GUI.Category = TEXT("Vehicle Mechanicles");
	GUI.IcanName = TEXT("SodaIcons.Motor");

	Common.Activation = EVehicleComponentActivation::OnStartScenario;
}

bool UVehicleEngineBaseComponent::OnActivateVehicleComponent()
{
	if (!Super::OnActivateVehicleComponent())
	{
		return false;
	}

	UObject* TorqueTransmissionObject = LinkToTorqueTransmission.GetObject<UObject>(GetOwner());
	ITorqueTransmission* TorqueTransmissionInterface = Cast<ITorqueTransmission>(TorqueTransmissionObject);
	if (TorqueTransmissionObject && TorqueTransmissionInterface)
	{
		OutputTorqueTransmission.SetInterface(TorqueTransmissionInterface);
		OutputTorqueTransmission.SetObject(TorqueTransmissionObject);
	}
	else
	{
		SetHealth(EVehicleComponentHealth::Error, TEXT("Transmission isn't connected"));
		return false;
	}
	
	return true;
}

void UVehicleEngineBaseComponent::OnDeactivateVehicleComponent()
{
	Super::OnDeactivateVehicleComponent();
}

bool UVehicleEngineBaseComponent::FindWheelRadius(float& OutRadius) const
{
	if (GetHealth() == EVehicleComponentHealth::Ok)
	{
		return OutputTorqueTransmission->FindWheelRadius(OutRadius);
	}

	return false;
}

bool UVehicleEngineBaseComponent::FindToWheelRatio(float& OutRatio) const
{
	if (GetHealth() == EVehicleComponentHealth::Ok)
	{
		float PrevRatio;
		if (OutputTorqueTransmission->FindToWheelRatio(PrevRatio))
		{
			OutRatio = PrevRatio * Ratio;
			return true;
		}
	}

	return false;
}

float UVehicleEngineBaseComponent::GetEngineLoad() const
{ 
	float MaxTorque = GetMaxTorque();

	//UE_LOG(LogSoda, Log, TEXT("************** %f %f"), MaxTorque, GetTorque());

	if (FMath::IsNearlyZero(MaxTorque))
	{
		return 1.0;
	}
	else
	{
		return std::fabsf(GetTorque() / MaxTorque);
	}
}

/********************************************************************************************************/

UVehicleEngineSimpleComponent::UVehicleEngineSimpleComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	GUI.ComponentNameOverride = TEXT("Engine Simple");
	GUI.bIsPresentInAddMenu = true;

	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PostPhysics;

	TickData.bAllowVehiclePrePhysTick = true;
	TickData.bAllowVehiclePostPhysTick = true;
	TickData.PostPhysTickGroup = EVehicleComponentPostPhysTickGroup::TickGroup5;

	static ConstructorHelpers::FObjectFinder<UCurveFloat> EngineCurevePtr(TEXT("/SodaSim/Assets/CPP/Curves/ElectricEngine/Engine_100Hm.Engine_100Hm"));
	TorqueCurve.ExternalCurve = EngineCurevePtr.Object;
}

void UVehicleEngineSimpleComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (!IsTickOnCurrentFrame() || !HealthIsWorkable()) return;

	if (GetHealth() == EVehicleComponentHealth::Ok)
	{
		MaxTorq = TorqueCurve.ExternalCurve->GetFloatValue(AngularVelocity * ANG2RPM) * TorqueCurveMultiplier;
	}

	if (bAcceptPedalFromVehicleInput)
	{
		if (UVehicleInputComponent* VehicleInput = GetWheeledVehicle()->GetActiveVehicleInput())
		{
			PedalPos = VehicleInput->GetInputState().Throttle;
		}
		else
		{
			PedalPos = 0;
		}
	}
}

bool UVehicleEngineSimpleComponent::OnActivateVehicleComponent()
{
	if (!Super::OnActivateVehicleComponent())
	{
		return false;
	}

	PedalPos = 0;

	if (!TorqueCurve.ExternalCurve)
	{
		SetHealth(EVehicleComponentHealth::Error, TEXT("TorqueCurve isn't set"));
		return false;
	}

	return true;
}

void UVehicleEngineSimpleComponent::OnDeactivateVehicleComponent()
{
	Super::OnDeactivateVehicleComponent();
	PedalPos = 0;
}

void UVehicleEngineSimpleComponent::RequestByTorque(float InTorque)
{
	if (GetHealth() == EVehicleComponentHealth::Ok)
	{
		RequestedTorque = InTorque;
		if (InTorque >= 0)
		{
			ActualTorque = std::min(InTorque, MaxTorq);
		}
		else
		{
			ActualTorque = -std::min(-InTorque, MaxTorq);
		}

		float Out = ActualTorque * Ratio;

		if (bVerboseLog)
		{
			UE_LOG(LogSoda, Log, TEXT("UVehicleEngineBaseComponent::RequestByTorque(); In torq: %f; Out torq: %f; Name: %s"), InTorque,  Out, *GetFName().ToString());
		}

		OutputTorqueTransmission->PassTorque(Out);
	}
}

void UVehicleEngineSimpleComponent::PrePhysicSimulation(float DeltaTime, const FPhysBodyKinematic& VehicleKinematic, const TTimestamp& Timestamp)
{
	Super::PrePhysicSimulation(DeltaTime, VehicleKinematic, Timestamp);

	if (bAcceptPedalFromVehicleInput)
	{
		RequestByRatio(PedalPos);
	}
}

void UVehicleEngineSimpleComponent::PostPhysicSimulation(float DeltaTime, const FPhysBodyKinematic& VehicleKinematic, const TTimestamp & Timestamp)
{
	Super::PostPhysicSimulation(DeltaTime, VehicleKinematic, Timestamp);

	if (GetHealth() == EVehicleComponentHealth::Ok)
	{
		AngularVelocity = OutputTorqueTransmission->ResolveAngularVelocity() * Ratio * (bFlipAngularVelocity ?  -1.0 : 1.0);
		if (bVerboseLog)
		{
			UE_LOG(LogSoda, Log, TEXT("UVehicleEngineBaseComponent::PostPhysicSimulation(); AngularVelocity: %f; MaxTorq: %f Name: %s"), AngularVelocity, MaxTorq, *GetFName().ToString());
		}
	}
}

void UVehicleEngineSimpleComponent::DrawDebug(UCanvas* Canvas, float& YL, float& YPos)
{
	Super::DrawDebug(Canvas, YL, YPos);

	if (Common.bDrawDebugCanvas)
	{
		UFont* RenderFont = GEngine->GetSmallFont();
		Canvas->SetDrawColor(FColor::White);
		YPos += Canvas->DrawText(RenderFont, FString::Printf(TEXT("Req/Act/Max Torq: %.2f / %.2f / %.2f H/m"), RequestedTorque, ActualTorque, MaxTorq), 16, YPos);
		YPos += Canvas->DrawText(RenderFont, FString::Printf(TEXT("Load: %d"), int(GetEngineLoad() * 100 + 0.5)), 16, YPos);
		YPos += Canvas->DrawText(RenderFont, FString::Printf(TEXT("AngVel: %.2f rpm"), AngularVelocity * ANG2RPM), 16, YPos);
		YPos += Canvas->DrawText(RenderFont, FString::Printf(TEXT("Pedal Pos: %.2f"), PedalPos), 16, YPos);
	}
}

void UVehicleEngineSimpleComponent::OnPushDataset(soda::FActorDatasetData& Dataset) const
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
			<< "AngVel" << AngularVelocity
			<< "MaxTorq" << MaxTorq
			<< "ReqTorq" << RequestedTorque
			<< "ActTor" << ActualTorque
			<< "PedalPos" << PedalPos
			<< close_document;
	}
	catch (const std::system_error& e)
	{
		UE_LOG(LogSoda, Error, TEXT("URacingSensor::OnPushDataset(); %s"), UTF8_TO_TCHAR(e.what()));
	}
}