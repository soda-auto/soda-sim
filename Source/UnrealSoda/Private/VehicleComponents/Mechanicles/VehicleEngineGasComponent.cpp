// Fill out your copyright notice in the Description page of Project Settings.


#include "VehicleComponents/Mechanicles/VehicleEngineGasComponent.h"
#include "Soda/VehicleComponents/VehicleInputComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Soda/UnrealSoda.h"
#include "Soda/Vehicles/SodaWheeledVehicle.h"
#include "Soda/SodaStatics.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"

UVehicleEngineGasComponent::UVehicleEngineGasComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	GUI.ComponentNameOverride = TEXT("Engine Gas");
	GUI.bIsPresentInAddMenu = true;

	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PostPhysics;

	TickData.bAllowVehiclePrePhysTick = true;
	TickData.bAllowVehiclePostPhysTick = true;
	TickData.PostPhysTickGroup = EVehicleComponentPostPhysTickGroup::TickGroup5;

	static ConstructorHelpers::FObjectFinder<UCurveFloat> EngineCurevePtr(TEXT("/SodaSim/Assets/CPP/Curves/GasEngine/Curve_SmedleyEngine_Float.Curve_SmedleyEngine_Float"));
	TorqueCurve.ExternalCurve = EngineCurevePtr.Object;
}

void UVehicleEngineGasComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (!IsTickOnCurrentFrame() || !HealthIsWorkable()) return;

	if (GetHealth() == EVehicleComponentHealth::Ok)
	{
		// Torque - effective engine torque
		// InitialTorque - torque after combustion
		// FrictionTorque - torque resistance because of friction
		// 
		//
		// Torque =  InitialTorque - FrictionTorque
		// FrictionTorque = FrictionCoeff * RPM + FrictionStart  
		// InitialTorque = (EngineTorqueCurve->GetFloat(RPM) + FrictionTorque) * Throttle
		FrictionTorque = StartFriction + ((AngularVelocity * ANG2RPM) * FrictionCoeff); // calculating friction torque
		MaxTorq = (TorqueCurve.ExternalCurve->GetFloatValue(AngularVelocity * ANG2RPM) + FrictionTorque) * TorqueCurveMultiplier;
		EffectiveTorque = MaxTorq - FrictionTorque;

		//EngineAcceleration = ActualTorque/Inertia
		//AnuglarVelocityDelta = Acceleration * DeltaTime
		// AnuglarVelocityDelta + AngularVelocity 
		// Clamp(RPM2RAD * EngineIdleRPM,RPM2RAD * EngineMaxRPM)
	}

	if (bAcceptPedalFromVehicleInput)
	{
		if (UVehicleInputComponent* VehicleInput = GetWheeledVehicle()->GetActiveVehicleInput())
		{
			PedalPos = VehicleInput->GetThrottleInput();
		}
		else
		{
			PedalPos = 0;
		}
	}
}

bool UVehicleEngineGasComponent::OnActivateVehicleComponent()
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

void UVehicleEngineGasComponent::OnDeactivateVehicleComponent()
{
	Super::OnDeactivateVehicleComponent();
	PedalPos = 0;
}

void UVehicleEngineGasComponent::RequestByTorque(float InTorque)
{
	if (GetHealth() == EVehicleComponentHealth::Ok)
	{

		RequestedTorque = InTorque;
		if (InTorque >= 0)
		{
			//EffectiveTorque = MaxTorque - FrictionTorque
			ActualTorque = std::min(InTorque, EffectiveTorque);
		}
		else
		{
			ActualTorque = -std::min(-InTorque, EffectiveTorque);
		}

		float Out = ActualTorque * Ratio;

		if (bVerboseLog)
		{
			UE_LOG(LogSoda, Log, TEXT("UVehicleEngineBaseComponent::RequestByTorque(); In torq: %f; Out torq: %f; Name: %s"), InTorque, Out, *GetFName().ToString());
		}

		OutputTorqueTransmission->PassTorque(Out);
	}
}

void UVehicleEngineGasComponent::PrePhysicSimulation(float DeltaTime, const FPhysBodyKinematic& VehicleKinematic, const TTimestamp& Timestamp)
{
	Super::PrePhysicSimulation(DeltaTime, VehicleKinematic, Timestamp);

	if (bAcceptPedalFromVehicleInput)
	{
		RequestByRatio(PedalPos);
	}
}

void UVehicleEngineGasComponent::PostPhysicSimulation(float DeltaTime, const FPhysBodyKinematic& VehicleKinematic, const TTimestamp& Timestamp)
{
	Super::PostPhysicSimulation(DeltaTime, VehicleKinematic, Timestamp);

	if (GetHealth() == EVehicleComponentHealth::Ok)
	{
		AngularVelocity = OutputTorqueTransmission->ResolveAngularVelocity() * Ratio;
		if (bVerboseLog)
		{
			UE_LOG(LogSoda, Log, TEXT("UVehicleEngineBaseComponent::PostPhysicSimulation(); AngularVelocity: %f; MaxTorq: %f Name: %s"), AngularVelocity, MaxTorq, *GetFName().ToString());
		}
	}
}

void UVehicleEngineGasComponent::DrawDebug(UCanvas* Canvas, float& YL, float& YPos)
{
	Super::DrawDebug(Canvas, YL, YPos);

	if (Common.bDrawDebugCanvas)
	{
		UFont* RenderFont = GEngine->GetSmallFont();
		Canvas->SetDrawColor(FColor::White);
		YPos += Canvas->DrawText(RenderFont, FString::Printf(TEXT("Req/Act/Max Torq: %.2f / %.2f / %.2f H/m"), RequestedTorque, ActualTorque, MaxTorq), 16, YPos);
		YPos += Canvas->DrawText(RenderFont, FString::Printf(TEXT("Load: %d"), int(GetEngineLoad() * 100 + 0.5)), 16, YPos);
		YPos += Canvas->DrawText(RenderFont, FString::Printf(TEXT("AngVel: %.2f rad/s"), AngularVelocity), 16, YPos);
		YPos += Canvas->DrawText(RenderFont, FString::Printf(TEXT("Pedal Pos: %.2f"), PedalPos), 16, YPos);
	}
}

