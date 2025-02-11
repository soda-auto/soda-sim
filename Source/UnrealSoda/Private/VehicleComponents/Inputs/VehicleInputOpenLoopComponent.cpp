// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/VehicleComponents/Inputs/VehicleInputOpenLoopComponent.h"
#include "Soda/UnrealSoda.h"
#include "Soda/Vehicles/SodaWheeledVehicle.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Soda/SodaStatics.h"
#include "Soda/SodaApp.h"
#include "UObject/ConstructorHelpers.h"
#include "Soda/Vehicles/IWheeledVehicleMovementInterface.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"

#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/PlatformFileManager.h"
#include "Containers/UnrealString.h"
#include "Misc/OutputDeviceDebug.h"
#include "DesktopPlatformModule.h"

UVehicleInputOpenLoopComponent::UVehicleInputOpenLoopComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	GUI.ComponentNameOverride = TEXT("OpenLoop");
	/*GUI.IcanName = TEXT("SodaIcons.Keyboard");*/
	GUI.bIsPresentInAddMenu = true;

	Common.Activation = EVehicleComponentActivation::OnStartScenario;

	TickData.bAllowVehiclePrePhysTick = true;

	PrimaryComponentTick.TickGroup = TG_PrePhysics;

	CurveFloatSteering = CreateDefaultSubobject<UCurveFloat>(TEXT("CurveSteer"));
	CurveFloatBraking = CreateDefaultSubobject<UCurveFloat>(TEXT("CurveBrk"));
	CurveFloatTraction = CreateDefaultSubobject<UCurveFloat>(TEXT("CurveTq"));


}

bool UVehicleInputOpenLoopComponent::OnActivateVehicleComponent()
{
	Super::OnActivateVehicleComponent();

	if (!GetWheeledVehicle() && !GetWheeledVehicle()->IsXWDVehicle(4))
	{
		SetHealth(EVehicleComponentHealth::Error, TEXT("Support only 4WD vehicles"));
		return false;
	}

	WheeledComponentInterface = GetWheeledComponentInterface();
	Veh = WheeledComponentInterface->GetWheeledVehicle();

	const TArray<USodaVehicleWheelComponent*>& Wheels = Veh->GetWheelsSorted();

	if (!GetWheeledVehicle()->IsXWDVehicle(4))
	{
		SetHealth(EVehicleComponentHealth::Error, TEXT("Supports only 4wd vehicle"));
		return false;
	}

	WheelFL = Veh->GetWheelByIndex(EWheelIndex::FL);
	WheelFR = Veh->GetWheelByIndex(EWheelIndex::FR);
	WheelRL = Veh->GetWheelByIndex(EWheelIndex::RL);
	WheelRR = Veh->GetWheelByIndex(EWheelIndex::RR);

	switch (OpenLoopManeuverType)
	{
	case EOpenLoopManeuverType::DoubleTripleStepSteer:
		SetupDoubleStepSteer();
		ScenarioSt.ScenarioStatus = EScenarioStatus::Preparation;
		break;
	case EOpenLoopManeuverType::RampSteer:
		SetupRampSteer();
		ScenarioSt.ScenarioStatus = EScenarioStatus::Preparation;
		break;
	case EOpenLoopManeuverType::AccelerationAndBraking:
		SetupAccelerationBraking();
		ScenarioSt.ScenarioStatus = EScenarioStatus::Preparation;
		break;
	case EOpenLoopManeuverType::CustomInputFile:
		SetupCustomInputFile();
		break;
	case EOpenLoopManeuverType::LookupTable:
		SetupLookupTable();
		break;
	default:
		ScenarioSt.bScenarioAcv = false;
		break;
	}

	return true;
}



void UVehicleInputOpenLoopComponent::OnDeactivateVehicleComponent()
{
	Super::OnDeactivateVehicleComponent();
}

void UVehicleInputOpenLoopComponent::PrePhysicSimulation(float DeltaTime, const FPhysBodyKinematic& VehicleKinematic, const TTimestamp& Timestamp)
{
	Super::PrePhysicSimulation(DeltaTime, VehicleKinematic, Timestamp);

	GetNewInput(DeltaTime);

	//SyncDataset();
}



void UVehicleInputOpenLoopComponent::SetupCustomInputFile()
{
	ScenarioSt.TargetVLgt = 0.0;
	ScenarioSt.bFeedbackControlLongitudinal = false;
}
void UVehicleInputOpenLoopComponent::SetupDoubleStepSteer()
{

	float TimeToRamp = (DoubleTripleStepSteerPrm.MaxSteeringAngle / DoubleTripleStepSteerPrm.SteeringRate);

	float LastTime = 0;

	float MaxSteeringAngleSigned = (DoubleTripleStepSteerPrm.bSteerPositive ? 1.0 : -1.0) * DoubleTripleStepSteerPrm.MaxSteeringAngle;

	ResetInputCurves();

	ScenarioSt.TimeToStart = DoubleTripleStepSteerPrm.TimeToStartScenario;


	CurveFloatSteering->FloatCurve.AddKey(LastTime, 0 * PI / 180);

	LastTime += TimeToRamp;
	CurveFloatSteering->FloatCurve.AddKey(LastTime, MaxSteeringAngleSigned * PI / 180);

	LastTime += DoubleTripleStepSteerPrm.TimeToKeepMaxSteering;
	CurveFloatSteering->FloatCurve.AddKey(LastTime, MaxSteeringAngleSigned * PI / 180);

	LastTime += TimeToRamp * 2;
	CurveFloatSteering->FloatCurve.AddKey(LastTime, -MaxSteeringAngleSigned * PI / 180);

	LastTime += DoubleTripleStepSteerPrm.TimeToKeepMaxSteering;
	CurveFloatSteering->FloatCurve.AddKey(LastTime, -MaxSteeringAngleSigned * PI / 180);

	LastTime += TimeToRamp;
	CurveFloatSteering->FloatCurve.AddKey(LastTime, 0 * PI / 180);

	if (DoubleTripleStepSteerPrm.bTripleSteer)
	{
		LastTime += TimeToRamp;
		CurveFloatSteering->FloatCurve.AddKey(LastTime, MaxSteeringAngleSigned * PI / 180);

		LastTime += DoubleTripleStepSteerPrm.TimeToKeepMaxSteering;
		CurveFloatSteering->FloatCurve.AddKey(LastTime, MaxSteeringAngleSigned * PI / 180);

		LastTime += TimeToRamp;
		CurveFloatSteering->FloatCurve.AddKey(LastTime, 0 * PI / 180);

	}

	ScenarioSt.TargetVLgt = DoubleTripleStepSteerPrm.StartSpeed;
	ScenarioSt.bFeedbackControlLongitudinal = true;

	CopyCurveTimestamps(CurveFloatSteering, CurveFloatBraking);
	CopyCurveTimestamps(CurveFloatSteering, CurveFloatTraction);

	ScenarioSt.TimeOfFinish = FMath::Max3(CurveFloatTraction->FloatCurve.GetLastKey().Time, CurveFloatSteering->FloatCurve.GetLastKey().Time, CurveFloatBraking->FloatCurve.GetLastKey().Time);

}


void UVehicleInputOpenLoopComponent::SetupLookupTable()
{

	if (!ValidateLookupInput(LookupTable.BrkPedl) || !ValidateLookupInput(LookupTable.Steering) || !ValidateLookupInput(LookupTable.AccrPedl))
	{
		ScenarioSt.bScenarioAcv = false;
		return;
	}

	ScenarioSt.TargetVLgt = 0;
	ScenarioSt.bFeedbackControlLongitudinal = false;

	InitZeroLut(LookupTable.AccrPedl);
	InitZeroLut(LookupTable.BrkPedl);
	InitZeroLut(LookupTable.Steering);

	for (auto& Elem : LookupTable.BrkPedl)
	{
		CurveFloatBraking->FloatCurve.AddKey(Elem.Time, Elem.Value * MaxTorque2Pedal / 100);
	}

	for (auto& Elem : LookupTable.Steering)
	{
		CurveFloatSteering->FloatCurve.AddKey(Elem.Time, Elem.Value / 180 * PI);
	}

	for (auto& Elem : LookupTable.AccrPedl)
	{
		CurveFloatTraction->FloatCurve.AddKey(Elem.Time, Elem.Value * MaxTorque2Pedal / 100);
	}

	ScenarioSt.TimeOfFinish = FMath::Max3(CurveFloatTraction->FloatCurve.GetLastKey().Time, CurveFloatSteering->FloatCurve.GetLastKey().Time, CurveFloatBraking->FloatCurve.GetLastKey().Time);


	if (LookupTable.bUseGearPosnLut && LookupTable.Gear.Num() > 0)
	{
		ScenarioSt.TimeOfFinish = FMath::Max(ScenarioSt.TimeOfFinish, LookupTable.Gear[LookupTable.Gear.Num() - 1].Time);
	}
}

void UVehicleInputOpenLoopComponent::InitZeroLut(TArray<FTimeVsValue>& Lut)
{
	// Add zero element to array in case if it was not initialized at all
	FTimeVsValue ZeroTimeValue;

	if (Lut.Num() == 0)
	{
		Lut.Add(ZeroTimeValue);
	}

}




void UVehicleInputOpenLoopComponent::SetupRampSteer()
{

	float TimeToRamp = (RampSteerPrms.MaxSteeringAngle / RampSteerPrms.SteerRate);

	float LastTime = 0;

	ResetInputCurves();

	CurveFloatSteering->FloatCurve.AddKey(LastTime, 0 * PI / 180);

	LastTime += TimeToRamp;
	CurveFloatSteering->FloatCurve.AddKey(LastTime, (RampSteerPrms.bSteerPositive ? 1.0 : -1.0) * RampSteerPrms.MaxSteeringAngle * PI / 180);

	ScenarioSt.TargetVLgt = RampSteerPrms.StartSpeed;
	ScenarioSt.bFeedbackControlLongitudinal = true;

	CopyCurveTimestamps(CurveFloatSteering, CurveFloatBraking);
	CopyCurveTimestamps(CurveFloatSteering, CurveFloatTraction);

	ScenarioSt.TimeOfFinish = FMath::Max3(CurveFloatTraction->FloatCurve.GetLastKey().Time, CurveFloatSteering->FloatCurve.GetLastKey().Time, CurveFloatBraking->FloatCurve.GetLastKey().Time);

}


void UVehicleInputOpenLoopComponent::SetupAccelerationBraking()
{
	float TimeToRamp = (RampSteerPrms.MaxSteeringAngle / RampSteerPrms.SteerRate);

	float LastTime = 0;

	ResetInputCurves();

	CurveFloatTraction->FloatCurve.AddKey(LastTime, 0);
	CurveFloatBraking->FloatCurve.AddKey(LastTime, 0);

	LastTime += AccelerationAndBraking.TimeToStartScenario;
	CurveFloatTraction->FloatCurve.AddKey(LastTime, 0);
	CurveFloatBraking->FloatCurve.AddKey(LastTime, 0);

	LastTime += AccelerationAndBraking.TorqueBuildUpTime;
	CurveFloatTraction->FloatCurve.AddKey(LastTime, AccelerationAndBraking.AccelerationTorqueTotal);
	CurveFloatBraking->FloatCurve.AddKey(LastTime, 0);

	LastTime += AccelerationAndBraking.AccelerationDuration;
	CurveFloatTraction->FloatCurve.AddKey(LastTime, AccelerationAndBraking.AccelerationTorqueTotal);
	CurveFloatBraking->FloatCurve.AddKey(LastTime, 0);

	LastTime += AccelerationAndBraking.TorqueBuildUpTime;

	LastTime += AccelerationAndBraking.AccelerationDuration;
	CurveFloatTraction->FloatCurve.AddKey(LastTime, 0);
	CurveFloatBraking->FloatCurve.AddKey(LastTime, AccelerationAndBraking.DecelerationTorqueTotal);

	LastTime += AccelerationAndBraking.DecelerationDuration;
	CurveFloatTraction->FloatCurve.AddKey(LastTime, 0);
	CurveFloatBraking->FloatCurve.AddKey(LastTime, AccelerationAndBraking.DecelerationTorqueTotal);

	LastTime += AccelerationAndBraking.TorqueBuildUpTime;
	CurveFloatTraction->FloatCurve.AddKey(LastTime, 0);
	CurveFloatBraking->FloatCurve.AddKey(LastTime, 0);

	ScenarioSt.TargetVLgt = 0;
	ScenarioSt.bFeedbackControlLongitudinal = false;

	CopyCurveTimestamps(CurveFloatBraking, CurveFloatSteering);

	ScenarioSt.TimeOfFinish = FMath::Max3(CurveFloatTraction->FloatCurve.GetLastKey().Time, CurveFloatSteering->FloatCurve.GetLastKey().Time, CurveFloatBraking->FloatCurve.GetLastKey().Time);


}

void UVehicleInputOpenLoopComponent::ResetInputCurves()
{
	CurveFloatTraction->FloatCurve.Reset();
	CurveFloatBraking->FloatCurve.Reset();
	CurveFloatSteering->FloatCurve.Reset();
}



bool UVehicleInputOpenLoopComponent::ValidateLookupInput(const TArray<FTimeVsValue>& Lut)
{
	float Time0 = -1.0;
	bool bInputIsCorrect = true;

	for (auto& Elem : Lut)
	{
		if (Elem.Time > Time0)
		{
			Time0 = Elem.Time;
		}
		else
		{
			bInputIsCorrect = false;
		}
	}

	if (!bInputIsCorrect)
	{
		FNotificationInfo Info(FText::FromString(FString("Lookup table based input generation FAILED. Time must be strictly monotonically increasing! ")));
		Info.ExpireDuration = 5.0f;
		Info.Image = FCoreStyle::Get().GetBrush(TEXT("Icons.ErrorWithColor"));
		FSlateNotificationManager::Get().AddNotification(Info);

		return false;
	}

	return true;
}


void UVehicleInputOpenLoopComponent::CopyCurveTimestamps(const UCurveFloat* CopyFrom, UCurveFloat* CopyTo)
{
	// Make sure that curves that are not used in current preset still have the same number of points and times
	CopyTo->FloatCurve.Reset();

	for (auto& Elem : CopyFrom->FloatCurve.GetCopyOfKeys())
	{
		CopyTo->FloatCurve.AddKey(Elem.Time, 0.0);
	}

}


void UVehicleInputOpenLoopComponent::DoInitSequence(float Ts)
{

	if (!InitToDrivePrms.bDoInitToDriveSequence || InitToDrivePrms.bInitSequenceIsDone)
	{
		return;
	}

	FInitToDrivePrms& Prm = InitToDrivePrms;

	GlobalTime = -1.0;
	LocalTime = 0.0;
	Prm.InitSequenceLocalTime = Prm.InitSequenceLocalTime + Ts;


	float TimeToEndBrakePress = Prm.TimeToStayInPark + Prm.TimeToPressBrakePedal;
	float TimeToPressToDrive = TimeToEndBrakePress + Prm.TimeToWaitBeforeSwitchToDrive;
	float TimeToStartBrakeRelease = TimeToPressToDrive + Prm.TimeToStayInDriveWithPedalPressed;
	float TimeToFinishInit = TimeToStartBrakeRelease + Prm.TimeToReleaseBrakePedal;


	if (Prm.InitSequenceLocalTime < TimeToEndBrakePress)
	{
		Prm.CurPedlPosn = LinInterp1(Prm.InitSequenceLocalTime, Prm.TimeToStayInPark, TimeToEndBrakePress, 0, 1);
	}
	else if (Prm.InitSequenceLocalTime > TimeToStartBrakeRelease)
	{
		Prm.CurPedlPosn = LinInterp1(Prm.InitSequenceLocalTime, TimeToStartBrakeRelease, TimeToStartBrakeRelease + Prm.TimeToReleaseBrakePedal, 1, 0);
	}


	OpenLoopInput.BrkTqFL = Prm.CurPedlPosn * MaxTorque2Pedal * 0.5 * BrkTqBalance;
	OpenLoopInput.BrkTqFR = Prm.CurPedlPosn * MaxTorque2Pedal * 0.5 * BrkTqBalance;
	OpenLoopInput.BrkTqRL = Prm.CurPedlPosn * MaxTorque2Pedal * 0.5 * (1 - BrkTqBalance);
	OpenLoopInput.BrkTqRR = Prm.CurPedlPosn * MaxTorque2Pedal * 0.5 * (1 - BrkTqBalance);

	if (Prm.InitSequenceLocalTime < TimeToPressToDrive)
	{
		OpenLoopInput.GearState = EGearState::Park;
	}
	else
	{
		OpenLoopInput.GearState = EGearState::Drive;
	}

	if (Prm.InitSequenceLocalTime > TimeToFinishInit)
	{
		Prm.bInitSequenceIsDone = true;
	}

	//UE_LOG(LogTemp, Warning, TEXT("Time %f, pedal %f, gear %d "), Prm.InitSequenceLocalTime, Prm.CurPedlPosn, static_cast<int32>(OpenLoopInput.GearState));

	AssignOutputs();


}


void UVehicleInputOpenLoopComponent::GetNewInput(float DeltaTime)
{

	FVehicleSimData SimData = WheeledComponentInterface->GetSimData();
	float SpeedNorm = SimData.VehicleKinematic.Curr.GlobalVelocityOfCenterMass.Size() / 100;


	GlobalTime = GlobalTime + DeltaTime;


	DoInitSequence(DeltaTime);


	if (GlobalTime < ScenarioSt.TimeToStart)
	{
		return;
	}


	if (ScenarioSt.bScenarioAcv)
	{
		LocalTime = LocalTime + DeltaTime;

		if (OpenLoopManeuverType != EOpenLoopManeuverType::CustomInputFile)
		{
			// Work with parameterised input

			if (ScenarioSt.ScenarioStatus == EScenarioStatus::Preparation && FMath::Abs(SpeedNorm - ScenarioSt.TargetVLgt) > VLgtTolerance)
			{
				// Don't start scenario until target speed is reached
				LocalTime = 0;
			}
			else
			{
				ScenarioSt.ScenarioStatus = EScenarioStatus::Active;
			}

			if (LocalTime > ScenarioSt.TimeOfFinish)
			{
				ScenarioIsDone();
			}

			float TqReq = 0;
			float BrkTqReq = 0;

			if (ScenarioSt.bFeedbackControlLongitudinal)
			{
				float Error = FMath::Clamp(SpeedNorm - ScenarioSt.TargetVLgt, -5.0, 5.0);

				if (Error < 5.0)
				{
					VLgtIError = VLgtIError + Error * GetWorld()->GetDeltaSeconds();
				}
				else
				{
					VLgtIError = 0;
				}

				float ControlSignal = VLgtCtlI * VLgtIError + VLgtCtlP * FMath::Clamp(SpeedNorm - ScenarioSt.TargetVLgt, -5.0, 5.0);

				TqReq = FMath::Max(ControlSignal, 0);
				BrkTqReq = FMath::Min(ControlSignal, 0);
			}
			else
			{
				TqReq = CurveFloatTraction->FloatCurve.Eval(LocalTime);
				BrkTqReq = CurveFloatBraking->FloatCurve.Eval(LocalTime);
			}

			float SteerReq = CurveFloatSteering->FloatCurve.Eval(LocalTime);

			OpenLoopInput.TracTqFL = TqReq * 0.5 * TracTqBalance;
			OpenLoopInput.TracTqFR = TqReq * 0.5 * TracTqBalance;
			OpenLoopInput.TracTqRL = TqReq * 0.5 * (1 - TracTqBalance);
			OpenLoopInput.TracTqRR = TqReq * 0.5 * (1 - TracTqBalance);

			OpenLoopInput.BrkTqFL = BrkTqReq * 0.5 * BrkTqBalance;
			OpenLoopInput.BrkTqFR = BrkTqReq * 0.5 * BrkTqBalance;
			OpenLoopInput.BrkTqRL = BrkTqReq * 0.5 * (1 - BrkTqBalance);
			OpenLoopInput.BrkTqRR = BrkTqReq * 0.5 * (1 - BrkTqBalance);

			OpenLoopInput.SteerFL = SteerReq;
			OpenLoopInput.SteerFR = SteerReq;
			OpenLoopInput.SteerRL = 0.0;
			OpenLoopInput.SteerRR = 0.0;

			if (OpenLoopManeuverType == EOpenLoopManeuverType::LookupTable && LookupTable.bUseGearPosnLut)
			{
				OpenLoopInput.GearState = LookupTable1dGear(LocalTime, LookupTable.Gear);
			}
			else
			{
				OpenLoopInput.GearState = EGearState::Drive;
			}


		}
		else
		{
			// Work with custom input file

			OpenLoopInput.TracTqFL = LookupTable1d(LocalTime, OpenLoopExternalInput.Time, OpenLoopExternalInput.TracTqFL);
			OpenLoopInput.TracTqFR = LookupTable1d(LocalTime, OpenLoopExternalInput.Time, OpenLoopExternalInput.TracTqFR);
			OpenLoopInput.TracTqRL = LookupTable1d(LocalTime, OpenLoopExternalInput.Time, OpenLoopExternalInput.TracTqRL);
			OpenLoopInput.TracTqRR = LookupTable1d(LocalTime, OpenLoopExternalInput.Time, OpenLoopExternalInput.TracTqRR);

			OpenLoopInput.BrkTqFL = LookupTable1d(LocalTime, OpenLoopExternalInput.Time, OpenLoopExternalInput.BrkTqFL);
			OpenLoopInput.BrkTqFR = LookupTable1d(LocalTime, OpenLoopExternalInput.Time, OpenLoopExternalInput.BrkTqFR);
			OpenLoopInput.BrkTqRL = LookupTable1d(LocalTime, OpenLoopExternalInput.Time, OpenLoopExternalInput.BrkTqRL);
			OpenLoopInput.BrkTqRR = LookupTable1d(LocalTime, OpenLoopExternalInput.Time, OpenLoopExternalInput.BrkTqRR);

			OpenLoopInput.SteerFL = LookupTable1d(LocalTime, OpenLoopExternalInput.Time, OpenLoopExternalInput.SteerFL);
			OpenLoopInput.SteerFR = LookupTable1d(LocalTime, OpenLoopExternalInput.Time, OpenLoopExternalInput.SteerFR);
			OpenLoopInput.SteerRL = LookupTable1d(LocalTime, OpenLoopExternalInput.Time, OpenLoopExternalInput.SteerRL);
			OpenLoopInput.SteerRR = LookupTable1d(LocalTime, OpenLoopExternalInput.Time, OpenLoopExternalInput.SteerRR);


			if (LocalTime > OpenLoopExternalInput.Time.Last())
			{
				ScenarioIsDone();
			}

		}

	}

	if (ScenarioSt.ScenarioStatus == EScenarioStatus::Completed && bBrakeWhenScenarioIsDone)
	{
		OpenLoopInput.BrkTqFL = BrkTqReqWhenScenarioIsDone * 0.5 * BrkTqBalance;
		OpenLoopInput.BrkTqFR = BrkTqReqWhenScenarioIsDone * 0.5 * BrkTqBalance;
		OpenLoopInput.BrkTqRL = BrkTqReqWhenScenarioIsDone * 0.5 * (1 - BrkTqBalance);
		OpenLoopInput.BrkTqRR = BrkTqReqWhenScenarioIsDone * 0.5 * (1 - BrkTqBalance);
	}

	AssignOutputs();

}

void UVehicleInputOpenLoopComponent::AssignOutputs()
{
	if (OutputMode == EOutputMode::DirectControl)
	{
		WheelFL->ReqTorq = OpenLoopInput.TracTqFL;
		WheelFL->ReqBrakeTorque = OpenLoopInput.BrkTqFL;
		WheelFL->ReqSteer = OpenLoopInput.SteerFL;

		WheelFR->ReqTorq = OpenLoopInput.TracTqFR;
		WheelFR->ReqBrakeTorque = OpenLoopInput.BrkTqFR;
		WheelFR->ReqSteer = OpenLoopInput.SteerFR;

		WheelRL->ReqTorq = OpenLoopInput.TracTqRL;
		WheelRL->ReqBrakeTorque = OpenLoopInput.BrkTqRL;
		WheelRL->ReqSteer = OpenLoopInput.SteerRL;

		WheelRR->ReqTorq = OpenLoopInput.TracTqRR;
		WheelRR->ReqBrakeTorque = OpenLoopInput.BrkTqRR;
		WheelRR->ReqSteer = OpenLoopInput.SteerRR;
	}
	else
	{
		InputState.Brake = LinInterp1((OpenLoopInput.BrkTqFL + OpenLoopInput.BrkTqFR + OpenLoopInput.BrkTqRL + OpenLoopInput.BrkTqRR), 0.0, MaxTorque2Pedal, 0.0, 1.0);
		InputState.Throttle = LinInterp1((OpenLoopInput.TracTqFL + OpenLoopInput.TracTqFR + OpenLoopInput.TracTqRL + OpenLoopInput.TracTqRR), 0.0, MaxTorque2Pedal, 0.0, 1.0);
		InputState.Steering = LinInterp1((OpenLoopInput.SteerFL + OpenLoopInput.SteerFR) * 0.5, -MaxSteeringAngle * PI / 180, MaxSteeringAngle * PI / 180, -1.0, 1.0);

		InputState.SetGearState(OpenLoopInput.GearState);
	}

}


void UVehicleInputOpenLoopComponent::GenerateInputFileTemplate()
{
	// Open a dialog to let the user choose the directory
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	if (DesktopPlatform)
	{
		FString SelectedFolder;
		const void* ParentWindowHandle = nullptr; // Can set to a valid window handle if needed

		// Open the directory picker
		bool bFolderSelected = DesktopPlatform->OpenDirectoryDialog(
			ParentWindowHandle,
			TEXT("Select Folder to Save CSV"),
			FPaths::ProjectDir(),
			SelectedFolder
		);

		if (bFolderSelected)
		{
			// Define the CSV filename and full path
			FString FilePath = SelectedFolder / TEXT("CarDataTemplate.csv");

			// Define the header row and an example data row
			FString CSVContent = TEXT("time_sec,RoadWhlAngFL_deg,RoadWhlAngFR_deg,RoadWhlAngRL_deg,RoadWhlAngRR_deg,TracTqFL_Nm,TracTqFR_Nm,TracTqRL_Nm,TracTqRR_Nm,BrkTqFL_Nm,BrkTqFR_Nm,BrkTqRL_Nm,BrkTqRR_Nm\n");


			CSVContent += TEXT("0.0, 0.0, 0.0, 0.0, 0.0,0.0, 0.0, 0.0, 0.0,0.0, 0.0, 0.0, 0.0\n");
			CSVContent += TEXT("15.0, 2.0, 2.0, 0.0, 0.0,150.0, 150.0, 150.0, 150.0,0.0, 0.0, 0.0, 0.0\n");


			// Save the CSV content to a file
			if (FFileHelper::SaveStringToFile(CSVContent, *FilePath))
			{
				soda::ShowNotification(ENotificationLevel::Success, 5.0, TEXT("CSV template was created at: %s"), *FilePath);

			}
			else
			{
				soda::ShowNotification(ENotificationLevel::Error, 5.0, TEXT("CSV template creation FAILED"));
			}
		}
		else
		{
			soda::ShowNotification(ENotificationLevel::Error, 5.0, TEXT("CSV template creation: no folder selected"));
		}
	}
}


void UVehicleInputOpenLoopComponent::GenerateLutInputFileTemplate()
{
	// Open a dialog to let the user choose the directory
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	if (DesktopPlatform)
	{
		FString SelectedFolder;
		const void* ParentWindowHandle = nullptr; // Can set to a valid window handle if needed

		// Open the directory picker
		bool bFolderSelected = DesktopPlatform->OpenDirectoryDialog(
			ParentWindowHandle,
			TEXT("Select Folder to Save CSV"),
			FPaths::ProjectDir(),
			SelectedFolder
		);

		if (bFolderSelected)
		{
			// Define the CSV filename and full path
			FString FilePath = SelectedFolder / TEXT("OpenLoopLutTemplate.csv");

			// Define the header row and an example data row
			FString CSVContent = TEXT("AccPedlTime,AccPedlPerc,BrkPedlTime,BrkPedlPerc,RoadWhlAngTime,RoadWhlAngDeg,GearTime,GearPosnPRND3201\n");

			//Neutral = 0,
			//	Drive = 1,
			//	Reverse = 2,
			//	Park = 3,

			CSVContent += TEXT("0.0, 0.0, 0.0, 0.0, 0.0, 0.0, NA, NA\n");
			CSVContent += TEXT("1.0, 0.0, 1.0, 0.0, 1.0, 0.0, NA, NA\n");
			CSVContent += TEXT("2.0, 50.0, NA, NA, NA, NA, NA, NA\n");

			// Save the CSV content to a file
			if (FFileHelper::SaveStringToFile(CSVContent, *FilePath))
			{
				FNotificationInfo Info(FText::FromString(FString("CSV template was created at: ") + FilePath));
				Info.ExpireDuration = 5.0f;
				Info.Image = FCoreStyle::Get().GetBrush(TEXT("Icons.SuccessWithColor"));
				FSlateNotificationManager::Get().AddNotification(Info);
			}
			else
			{
				FNotificationInfo Info(FText::FromString(FString("CSV template creation FAILED")));
				Info.ExpireDuration = 5.0f;
				Info.Image = FCoreStyle::Get().GetBrush(TEXT("Icons.ErrorWithColor"));
				FSlateNotificationManager::Get().AddNotification(Info);
			}
		}
		else
		{
			FNotificationInfo Info(FText::FromString(FString("CSV template creation: no folder selected")));
			Info.ExpireDuration = 5.0f;
			Info.Image = FCoreStyle::Get().GetBrush(TEXT("Icons.WarningWithColor"));
			FSlateNotificationManager::Get().AddNotification(Info);

		}
	}
}

void UVehicleInputOpenLoopComponent::LoadInputFromFile()
{
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	if (!DesktopPlatform)
	{
		UE_LOG(LogSoda, Error, TEXT("UVehicleInputOpenLoopComponent::LoadCustomInput() Can't get the IDesktopPlatform ref"));
		return;
	}

	const FString FileTypes = TEXT("(*.csv)|*.csv");

	TArray<FString> OpenFilenames;
	int32 FilterIndex = -1;
	if (!DesktopPlatform->OpenFileDialog(nullptr, TEXT("Import custom input from CSV"), TEXT(""), TEXT(""), FileTypes, EFileDialogFlags::None, OpenFilenames, FilterIndex) || OpenFilenames.Num() <= 0)
	{
		soda::ShowNotification(ENotificationLevel::Error, 5.0, TEXT("Load input Error: can't open the file"));
		return;
	}

	LoadCustomInput(OpenFilenames[0]);

	OpenLoopManeuverType = EOpenLoopManeuverType::CustomInputFile;
}

void UVehicleInputOpenLoopComponent::LoadLutInputFromFile()
{
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	if (!DesktopPlatform)
	{
		UE_LOG(LogSoda, Error, TEXT("UVehicleInputOpenLoopComponent::LoadLutInputFromFile() Can't get the IDesktopPlatform ref"));
		return;
	}

	const FString FileTypes = TEXT("(*.csv)|*.csv");

	TArray<FString> OpenFilenames;
	int32 FilterIndex = -1;
	if (!DesktopPlatform->OpenFileDialog(nullptr, TEXT("Import custom lut input from CSV"), TEXT(""), TEXT(""), FileTypes, EFileDialogFlags::None, OpenFilenames, FilterIndex) || OpenFilenames.Num() <= 0)
	{
		FNotificationInfo Info(FText::FromString(FString("Load input Error: can't open the file")));
		Info.ExpireDuration = 5.0f;
		Info.Image = FCoreStyle::Get().GetBrush(TEXT("Icons.ErrorWithColor"));
		FSlateNotificationManager::Get().AddNotification(Info);
		return;
	}

	LoadLutCustomInput(OpenFilenames[0]);

	OpenLoopManeuverType = EOpenLoopManeuverType::LookupTable;
}

bool UVehicleInputOpenLoopComponent::LoadCustomInput(const FString& FilePath)
{
	if (!FPlatformFileManager::Get().GetPlatformFile().FileExists(*FilePath))
	{
		UE_LOG(LogTemp, Error, TEXT("File does not exist: %s"), *FilePath);
		return false;
	}

	TArray<FString> FileLines;
	if (!FFileHelper::LoadFileToStringArray(FileLines, *FilePath))
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to load file: %s"), *FilePath);
		return false;
	}


	if (FileLines.Num() < 2)
	{
		soda::ShowNotification(ENotificationLevel::Error, 5.0, TEXT("Load input Error: file has no data %s"), *FilePath);
		return false;
	}

	FileLines.RemoveAt(0);

	OpenLoopExternalInput.Time.Reserve(FileLines.Num() - 1);
	OpenLoopExternalInput.BrkTqFL.Reserve(FileLines.Num() - 1);
	OpenLoopExternalInput.BrkTqFR.Reserve(FileLines.Num() - 1);
	OpenLoopExternalInput.BrkTqRL.Reserve(FileLines.Num() - 1);
	OpenLoopExternalInput.BrkTqRR.Reserve(FileLines.Num() - 1);
	OpenLoopExternalInput.SteerFL.Reserve(FileLines.Num() - 1);
	OpenLoopExternalInput.SteerFR.Reserve(FileLines.Num() - 1);
	OpenLoopExternalInput.SteerRL.Reserve(FileLines.Num() - 1);
	OpenLoopExternalInput.SteerRR.Reserve(FileLines.Num() - 1);
	OpenLoopExternalInput.TracTqFL.Reserve(FileLines.Num() - 1);
	OpenLoopExternalInput.TracTqFR.Reserve(FileLines.Num() - 1);
	OpenLoopExternalInput.TracTqRL.Reserve(FileLines.Num() - 1);
	OpenLoopExternalInput.TracTqRR.Reserve(FileLines.Num() - 1);


	bool bNoErrorPreviously = true;
	// Parse each line
	for (const FString& Line : FileLines)
	{
		TArray<FString> ParsedValues;
		Line.ParseIntoArray(ParsedValues, TEXT(","), true);

		if (ParsedValues.Num() != 13)
		{
			if (bNoErrorPreviously)
			{
				soda::ShowNotification(ENotificationLevel::Error, 5.0, TEXT("Load input Error: incorrect number of columns %s"), *FilePath);
				bNoErrorPreviously = false;
			}
				

			continue;
		}

		int32 k = 0;

		OpenLoopExternalInput.Time.Add(FCString::Atof(*ParsedValues[k])); k++;
		OpenLoopExternalInput.SteerFL.Add(FCString::Atof(*ParsedValues[k]) * PI / 180); k++;
		OpenLoopExternalInput.SteerFR.Add(FCString::Atof(*ParsedValues[k]) * PI / 180); k++;
		OpenLoopExternalInput.SteerRL.Add(FCString::Atof(*ParsedValues[k]) * PI / 180); k++;
		OpenLoopExternalInput.SteerRR.Add(FCString::Atof(*ParsedValues[k]) * PI / 180); k++;
		OpenLoopExternalInput.TracTqFL.Add(FCString::Atof(*ParsedValues[k])); k++;
		OpenLoopExternalInput.TracTqFR.Add(FCString::Atof(*ParsedValues[k])); k++;
		OpenLoopExternalInput.TracTqRL.Add(FCString::Atof(*ParsedValues[k])); k++;
		OpenLoopExternalInput.TracTqRR.Add(FCString::Atof(*ParsedValues[k])); k++;
		OpenLoopExternalInput.BrkTqFL.Add(FCString::Atof(*ParsedValues[k])); k++;
		OpenLoopExternalInput.BrkTqFR.Add(FCString::Atof(*ParsedValues[k])); k++;
		OpenLoopExternalInput.BrkTqRL.Add(FCString::Atof(*ParsedValues[k])); k++;
		OpenLoopExternalInput.BrkTqRR.Add(FCString::Atof(*ParsedValues[k]));

	}

	soda::ShowNotification(ENotificationLevel::Success, 5.0, TEXT("Load input: success! %s"), *FilePath);

	return true;
}



bool UVehicleInputOpenLoopComponent::LoadLutCustomInput(const FString& FilePath)
{


	if (!FPlatformFileManager::Get().GetPlatformFile().FileExists(*FilePath))
	{
		UE_LOG(LogTemp, Error, TEXT("File does not exist: %s"), *FilePath);
		return false;
	}


	TArray<FString> FileLines;
	if (!FFileHelper::LoadFileToStringArray(FileLines, *FilePath))
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to load file as string: %s. Make sure you close .csv editing application before loading"), *FilePath);
		return false;
	}


	if (FileLines.Num() < 2)
	{
		soda::ShowNotification(ENotificationLevel::Error, 5.0, TEXT("Load input Error: file has no data %s"), *FilePath);
		return false;
	}


	FileLines.RemoveAt(0);



	bool bNoErrorPreviously = true;
	// Parse each line



	LookupTable.BrkPedl.Empty();
	LookupTable.AccrPedl.Empty();
	LookupTable.Steering.Empty();
	LookupTable.Gear.Empty();
	LookupTable.bUseGearPosnLut = false;

	for (const FString& Line : FileLines)
	{

		TArray<FString> ParsedValues;
		Line.ParseIntoArray(ParsedValues, TEXT(","), true);

		if (ParsedValues.Num() != 8)
		{
			if (bNoErrorPreviously)
			{
				soda::ShowNotification(ENotificationLevel::Error, 5.0, TEXT("Load input Error: incorrect number of columns %s"), *FilePath);
				bNoErrorPreviously = false;
			}

			continue;
		}


		FTimeVsValue TimeVsValue;
		FTimeVsGear TimeVsGear;


		// Do for accelerator pedal
		int32 k = 0;

		if (ParsedValues[k].Compare(FString("NA")) != int32(0) && ParsedValues[k + 1].Compare(FString("NA")) != int32(0))
		{
			TimeVsValue.Time = FCString::Atof(*ParsedValues[k]);
			TimeVsValue.Value = FCString::Atof(*ParsedValues[k + 1]);

			LookupTable.AccrPedl.Add(TimeVsValue);
		}

		// Do for braking pedal
		k = k + 2;

		if (ParsedValues[k].Compare(FString("NA")) != int32(0) && ParsedValues[k + 1].Compare(FString("NA")) != int32(0))
		{
			TimeVsValue.Time = FCString::Atof(*ParsedValues[k]);
			TimeVsValue.Value = FCString::Atof(*ParsedValues[k + 1]);

			LookupTable.BrkPedl.Add(TimeVsValue);
		}

		// Do for steering
		k = k + 2;

		if (ParsedValues[k].Compare(FString("NA")) != int32(0) && ParsedValues[k + 1].Compare(FString("NA")) != int32(0))
		{
			TimeVsValue.Time = FCString::Atof(*ParsedValues[k]);
			TimeVsValue.Value = FCString::Atof(*ParsedValues[k + 1]);

			LookupTable.Steering.Add(TimeVsValue);
		}

		// Do for gear
		k = k + 2;

		if (ParsedValues[k].Compare(FString("NA")) != int32(0) && ParsedValues[k + 1].Compare(FString("NA")) != int32(0))
		{
			TimeVsGear.Time = FCString::Atof(*ParsedValues[k]);

			int32 GearInt = int32(FCString::Atof(*ParsedValues[k + 1]));

			LookupTable.bUseGearPosnLut = true;

			switch (GearInt)
			{
			case int32(0):
				TimeVsGear.GearState = EGearState::Neutral;
				break;
			case int32(1):
				TimeVsGear.GearState = EGearState::Drive;
				break;
			case int32(2):
				TimeVsGear.GearState = EGearState::Reverse;
				break;
			case int32(3):
				TimeVsGear.GearState = EGearState::Park;
				break;
			default:
				soda::ShowNotification(ENotificationLevel::Error, 5.0, TEXT("Load input Error: Input Gear outside of the 0,1,2,3 values. Simulation will ignore Gear Lut %s"), *FilePath);
				LookupTable.bUseGearPosnLut = false;
				break;
			}

			LookupTable.Gear.Add(TimeVsGear);
		}
	}

	InitZeroLut(LookupTable.AccrPedl);
	InitZeroLut(LookupTable.BrkPedl);
	InitZeroLut(LookupTable.Steering);

	if (!ValidateLookupInput(LookupTable.BrkPedl) || !ValidateLookupInput(LookupTable.Steering) || !ValidateLookupInput(LookupTable.AccrPedl))
	{

	}
	else
	{
		soda::ShowNotification(ENotificationLevel::Success, 5.0, TEXT("Load input: success! %s"), *FilePath);
	}

	return true;
}



void UVehicleInputOpenLoopComponent::ScenarioIsDone()
{
	ScenarioSt.ScenarioStatus = EScenarioStatus::Completed;
	ScenarioSt.bScenarioAcv = false;

	OpenLoopInput.TracTqFL = 0;
	OpenLoopInput.TracTqFR = 0;
	OpenLoopInput.TracTqRL = 0;
	OpenLoopInput.TracTqRR = 0;

	OpenLoopInput.BrkTqFL = 0;
	OpenLoopInput.BrkTqFR = 0;
	OpenLoopInput.BrkTqRL = 0;
	OpenLoopInput.BrkTqRR = 0;

	OpenLoopInput.SteerFL = 0;
	OpenLoopInput.SteerFR = 0;
	OpenLoopInput.SteerRL = 0;
	OpenLoopInput.SteerRR = 0;

	if (bGenerateNotificationWhenDone)
	{
		soda::ShowNotification(ENotificationLevel::Success, 5.0, TEXT("Scenario is done, you can stop simulation now"));
	}
}


void UVehicleInputOpenLoopComponent::UpdateInputStates(float DeltaTime, float ForwardSpeed, const APlayerController* PlayerController)
{
	Super::UpdateInputStates(DeltaTime, ForwardSpeed, PlayerController);

	//check(GetWheeledVehicle() != nullptr);

	//SCOPE_CYCLE_COUNTER(STAT_AI_TickComponent);

	//UpdateInputStatesInner(DeltaTime);



}




EGearState UVehicleInputOpenLoopComponent::LookupTable1dGear(float Time, const TArray<FTimeVsGear>& TimeVsGear)
{

	// Check for the same size 

	if (TimeVsGear.Num() == 0)
	{
		return EGearState::Park;
	}


	// Check for clipping

	if (Time <= TimeVsGear[0].Time)
	{
		return TimeVsGear[0].GearState;
	}
	else if (Time >= TimeVsGear[TimeVsGear.Num() - 1].Time)
	{
		return TimeVsGear[TimeVsGear.Num() - 1].GearState;
	}



	// Moving along 
	for (int32 Idx = 0; Idx <= TimeVsGear.Num() - 2; Idx++)
	{

		if (Time >= TimeVsGear[Idx].Time && Time < TimeVsGear[Idx + 1].Time)
		{
			return TimeVsGear[Idx].GearState;
		}

	}

	return EGearState::Park;
}