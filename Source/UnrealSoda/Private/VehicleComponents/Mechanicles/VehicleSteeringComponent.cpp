// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

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

UVehicleSteeringRackBaseComponent::UVehicleSteeringRackBaseComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	GUI.Category = TEXT("Vehicle Mechanicles");
	GUI.IcanName = TEXT("SodaIcons.Steering");
	Common.Activation = EVehicleComponentActivation::OnStartScenario;
}

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

void UVehicleSteeringRackSimpleComponent::UpdateSteer(float DeltaTime, const FPhysBodyKinematic& VehicleKinematic, const TTimestamp& Timestamp)
{
	float ForwardSpeed = Chaos::CmSToKmH(VehicleKinematic.Curr.GetLocalVelocity().X);

	float SteerSpeed = SteeringCurve.ExternalCurve->GetFloatValue(ForwardSpeed) / 180.0 * M_PI;
	float DeltaSteerReq = TargetSteerAng - CurrentSteerAng;
	float DeltaSteer = SteerSpeed * DeltaTime;

	if (DeltaSteerReq < 0) DeltaSteer = -DeltaSteer;

	CurrentSteerAng += std::abs(DeltaSteerReq) < std::abs(DeltaSteer) ? DeltaSteerReq : DeltaSteer;
	CurrentSteerAng = FMath::Clamp(CurrentSteerAng, -MaxSteerAngle, MaxSteerAngle);

	GetWheeledVehicle()->GetWheel4WD(E4WDWheelIndex::FL)->ReqSteer = CurrentSteerAng;
	GetWheeledVehicle()->GetWheel4WD(E4WDWheelIndex::FR)->ReqSteer = CurrentSteerAng;
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
			SteerInputRatio = VehicleInput->GetSteeringInput();
		}
		else
		{
			SteerInputRatio = 0;
		}
	}
}


