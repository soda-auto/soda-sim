// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/VehicleComponents/Mechanicles/VehicleBrakeSystemComponent.h"
#include "Soda/UnrealSoda.h"
#include "Soda/Vehicles/SodaWheeledVehicle.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Soda/Vehicles/IWheeledVehicleMovementInterface.h"
#include "Soda/VehicleComponents/VehicleInputComponent.h"

UWheelBrake::UWheelBrake(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

UVehicleBrakeSystemBaseComponent::UVehicleBrakeSystemBaseComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	GUI.Category = TEXT("Vehicle Mechanicles");
	GUI.IcanName = TEXT("SodaIcons.Braking");
}

/*
 * UWheelBrakeSimple
 */

UWheelBrakeSimple::UWheelBrakeSimple(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UWheelBrakeSimple::RequestByPressure(float InBar, double DeltaTime)
{
	const float BarToTorq = double(1e5) * BrakeSetup.PadFricCoeff * BrakeSetup.ClpPistNo * BrakeSetup.PistAr * BrakeSetup.EfcRd;
	CurrentTorque = BrakeSystem->MechanicalBrakeRate.InterpInputValue(DeltaTime, CurrentTorque, InBar * BarToTorq);
	ConnectedWheel->ReqBrakeTorque += CurrentTorque;
	CurrentBar = InBar;
}

void UWheelBrakeSimple::RequestByTorque(float InTorque, double DeltaTime)
{
	const float BarToTorq = double(1e5) * BrakeSetup.PadFricCoeff * BrakeSetup.ClpPistNo * BrakeSetup.PistAr * BrakeSetup.EfcRd;
	ConnectedWheel->ReqBrakeTorque += BrakeSystem->MechanicalBrakeRate.InterpInputValue(DeltaTime, CurrentTorque, InTorque);
	CurrentTorque = InTorque;
	CurrentBar = InTorque / BarToTorq;
}

void UWheelBrakeSimple::RequestByRatio(float InRatio, double DeltaTime)
{
	ConnectedWheel->ReqBrakeTorque += BrakeSystem->MechanicalBrakeRate.InterpInputValue(DeltaTime, CurrentTorque, BrakeSetup.MaxTorque * InRatio);
}

float UWheelBrakeSimple::GetTorque() const
{
	return CurrentTorque;
}

float UWheelBrakeSimple::GetPressure() const
{
	return CurrentBar;
}

float UWheelBrakeSimple::GetLoad() const
{
	return CurrentTorque / BrakeSetup.MaxTorque;
}

/*
 * UVehicleBrakeSystemSimpleComponent
 */

UVehicleBrakeSystemSimpleComponent::UVehicleBrakeSystemSimpleComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	GUI.bIsPresentInAddMenu = true;
	GUI.ComponentNameOverride = TEXT("Brake System Simple");

	Common.Activation = EVehicleComponentActivation::OnStartScenario;

	TickData.bAllowVehiclePrePhysTick = true;

	PrimaryComponentTick.bCanEverTick = true;
	bTickInEditor = false;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;

	WheelBrakesSetup.SetNum(4);
}

void UVehicleBrakeSystemSimpleComponent::InitializeComponent()
{
	Super::InitializeComponent();
}

/*
UWheelBrakeSimple* UVehicleBrakeSystemSimpleComponent::GetWheelSimple4WD(EWheelIndex Ind) const
{
	if (WheelBrakes4WD.Num() == 4 && Ind != EWheelIndex::None)
	{
		return WheelBrakes4WD[int(Ind)].Get();;
	}
	else
	{
		return nullptr;
	}
}
*/

UWheelBrake* UVehicleBrakeSystemSimpleComponent::FindWheelByIndex(EWheelIndex Ind) const
{
	auto Found = WheelBrakes.FindByPredicate([Ind](auto& WheelBrake) { return WheelBrake->ConnectedWheel->GetWheelIndex() == Ind; });
	if (Found) return *Found;
	else return nullptr;
}

bool UVehicleBrakeSystemSimpleComponent::OnActivateVehicleComponent()
{
	if (!Super::OnActivateVehicleComponent())
	{
		return false;
	}

	PedalPos = 0;

	WheelBrakes.Empty();

	for (auto& Setup : WheelBrakesSetup)
	{
		UWheelBrakeSimple * WheelBrake = NewObject<UWheelBrakeSimple>(this);
		check(WheelBrake);
		WheelBrake->ConnectedWheel = Setup.ConnectedWheel.GetObject<USodaVehicleWheelComponent>(GetOwner());
		if (!IsValid(WheelBrake->ConnectedWheel))
		{
			WheelBrakes.Empty();
			SetHealth(EVehicleComponentHealth::Error, TEXT("Can't setup WheelBrake \"") + Setup.ConnectedWheel.PathToSubobject + TEXT("\""));
			return false;
		}
		WheelBrake->BrakeSystem = this;
		WheelBrake->BrakeSetup = Setup;

		WheelBrakes.Add(WheelBrake);
	}



	return true;
}
void UVehicleBrakeSystemSimpleComponent::OnDeactivateVehicleComponent()
{
	Super::OnDeactivateVehicleComponent();

	for (auto& It : WheelBrakes)
	{
		It->ConditionalBeginDestroy();
	}
	WheelBrakes.Empty();
	PedalPos = 0;
}

void UVehicleBrakeSystemSimpleComponent::RequestByAcceleration(float InAcceleration, double DeltaTime)
{
	RequestByForce(InAcceleration * GetWheeledComponentInterface()->GetVehicleMass(), DeltaTime);
}

void UVehicleBrakeSystemSimpleComponent::RequestByForce(float InForce, double DeltaTime)
{
	if (GetHealth() == EVehicleComponentHealth::Ok)
	{
		for (int i = 0; i < WheelBrakesSetup.Num(); ++i)
		{
			float BrakeTorq = FMath::Clamp(
				float(InForce * WheelBrakesSetup[i].BrakeDist * WheelBrakes[i]->ConnectedWheel->Radius / 100.f),
				0.0f,
				WheelBrakesSetup[i].MaxTorque);

			WheelBrakes[i]->RequestByTorque(BrakeTorq, DeltaTime);
		}
	}
}

void UVehicleBrakeSystemSimpleComponent::RequestByRatio(float InRatio, double DeltaTime)
{
	if (GetHealth() == EVehicleComponentHealth::Ok)
	{
		for (int i = 0; i < WheelBrakesSetup.Num(); ++i)
		{
			WheelBrakes[i]->RequestByTorque(WheelBrakesSetup[i].MaxTorque * InRatio, DeltaTime);
		}
	}
}

void UVehicleBrakeSystemSimpleComponent::RequestByPressure(float InBar, double DeltaTime)
{
	if (GetHealth() == EVehicleComponentHealth::Ok)
	{
		for (int i = 0; i < WheelBrakesSetup.Num(); ++i)
		{
			WheelBrakes[i]->RequestByPressure(WheelBrakesSetup[i].BrakeDist * InBar, DeltaTime);
		}
	}
}

float UVehicleBrakeSystemSimpleComponent::ComputeFullTorqByRatio(float InRatio)
{
	return (WheelBrakesSetup[0].MaxTorque + WheelBrakesSetup[1].MaxTorque + WheelBrakesSetup[2].MaxTorque + WheelBrakesSetup[3].MaxTorque) * InRatio;
}

void UVehicleBrakeSystemSimpleComponent::DrawDebug(UCanvas* Canvas, float& YL, float& YPos) 
{
	Super::DrawDebug(Canvas, YL, YPos);

	if (Common.bDrawDebugCanvas && (GetHealth() == EVehicleComponentHealth::Ok))
	{
		UFont* RenderFont = GEngine->GetSmallFont();
		Canvas->SetDrawColor(FColor::White);
		for (int i = 0; i < WheelBrakesSetup.Num(); ++i)
		{
			YPos += Canvas->DrawText(RenderFont, FString::Printf(TEXT("Wheel[%i]: %.1fH/mf %i%% "), i, WheelBrakes[i]->GetTorque(), int(WheelBrakes[i]->GetLoad() * 100 + 0.5)), 16, YPos);
		}
		YPos += Canvas->DrawText(RenderFont, FString::Printf(TEXT("Pedal Pos: %f"), PedalPos), 16, YPos);
	}
}

void UVehicleBrakeSystemSimpleComponent::PrePhysicSimulation(float DeltaTime, const FPhysBodyKinematic& VehicleKinematic, const TTimestamp& Timestamp)
{
	Super::PrePhysicSimulation(DeltaTime, VehicleKinematic, Timestamp);

	if (bAcceptPedalFromVehicleInput)
	{
		RequestByRatio(PedalPos, DeltaTime);
	}

	SyncDataset();
}

void UVehicleBrakeSystemSimpleComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bAcceptPedalFromVehicleInput)
	{
		if (UVehicleInputComponent* VehicleInput = GetWheeledVehicle()->GetActiveVehicleInput())
		{
			PedalPos = VehicleInput->GetInputState().Brake;
		}
		else
		{
			PedalPos = 0;
		}
	}
}
