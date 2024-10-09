// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/VehicleComponents/Inputs/VehicleInputOpenLoopComponent.h"
#include "Soda/UnrealSoda.h"
#include "Soda/Vehicles/SodaWheeledVehicle.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Soda/SodaStatics.h"
#include "Soda/SodaApp.h"
#include "Soda/SodaUserSettings.h"
#include "UObject/ConstructorHelpers.h"
#include "Soda/Vehicles/IWheeledVehicleMovementInterface.h"
#include "Soda/DBGateway.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"

#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/PlatformFilemanager.h"
#include "Containers/UnrealString.h"
#include "Misc/OutputDeviceDebug.h"
#include "DesktopPlatformModule.h"

#include "bsoncxx/builder/stream/helpers.hpp"
#include "bsoncxx/exception/exception.hpp"
#include "bsoncxx/builder/stream/document.hpp"
#include "bsoncxx/builder/stream/array.hpp"
#include "bsoncxx/json.hpp"

UVehicleInputOpenLoopComponent::UVehicleInputOpenLoopComponent(const FObjectInitializer &ObjectInitializer)
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

	WheeledComponentInterface = GetWheeledComponentInterface();
	Veh = WheeledComponentInterface->GetWheeledVehicle();
	WheelFL = Veh->GetWheel4WD(E4WDWheelIndex::FL);
	WheelFR = Veh->GetWheel4WD(E4WDWheelIndex::FR);
	WheelRL = Veh->GetWheel4WD(E4WDWheelIndex::RL);
	WheelRR = Veh->GetWheel4WD(E4WDWheelIndex::RR);

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

}

void UVehicleInputOpenLoopComponent::ResetInputCurves()
{
	CurveFloatTraction->FloatCurve.Reset();
	CurveFloatBraking->FloatCurve.Reset();
	CurveFloatSteering->FloatCurve.Reset();
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



void UVehicleInputOpenLoopComponent::GetNewInput(float DeltaTime)
{

	FVehicleSimData SimData = WheeledComponentInterface->GetSimData();
	float SpeedNorm = SimData.VehicleKinematic.Curr.GlobalVelocityOfCenterMass.Size() / 100;

	LocalTime = LocalTime + DeltaTime;



	if (ScenarioSt.bScenarioAcv)
	{
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

			OpenLoopInput.TracTqFL[0] = TqReq * 0.5 * TracTqBalance;
			OpenLoopInput.TracTqFR[0] = TqReq * 0.5 * TracTqBalance;
			OpenLoopInput.TracTqRL[0] = TqReq * 0.5 * (1 - TracTqBalance);
			OpenLoopInput.TracTqRR[0] = TqReq * 0.5 * (1 - TracTqBalance);

			OpenLoopInput.BrkTqFL[0] = BrkTqReq * 0.5 * BrkTqBalance;
			OpenLoopInput.BrkTqFR[0] = BrkTqReq * 0.5 * BrkTqBalance;
			OpenLoopInput.BrkTqRL[0] = BrkTqReq * 0.5 * (1 - BrkTqBalance);
			OpenLoopInput.BrkTqRR[0] = BrkTqReq * 0.5 * (1 - BrkTqBalance);

			OpenLoopInput.SteerFL[0] = SteerReq;
			OpenLoopInput.SteerFR[0] = SteerReq;
			OpenLoopInput.SteerRL[0] = 0.0;
			OpenLoopInput.SteerRR[0] = 0.0;
	
			// Assign to wheels
			WheelFL->ReqTorq = OpenLoopInput.TracTqFL[0];
			WheelFL->ReqBrakeTorque = OpenLoopInput.BrkTqFL[0];
			WheelFL->ReqSteer =  OpenLoopInput.SteerFL[0];

			WheelFR->ReqTorq = OpenLoopInput.TracTqFR[0];
			WheelFR->ReqBrakeTorque =  OpenLoopInput.BrkTqFR[0];
			WheelFR->ReqSteer = OpenLoopInput.SteerFR[0];

			WheelRL->ReqTorq =  OpenLoopInput.TracTqRL[0];
			WheelRL->ReqBrakeTorque = OpenLoopInput.BrkTqRL[0];
			WheelRL->ReqSteer = OpenLoopInput.SteerRL[0];

			WheelRR->ReqTorq = OpenLoopInput.TracTqRR[0];
			WheelRR->ReqBrakeTorque = OpenLoopInput.BrkTqRR[0];
			WheelRR->ReqSteer = OpenLoopInput.SteerRR[0];	

			if (LocalTime > CurveFloatTraction->FloatCurve.GetLastKey().Time)
			{
				ScenarioIsDone();
			}

		}
		else
		{
			// Work with custom input file

			WheelFL->ReqTorq = Lut1d(LocalTime, OpenLoopInput.Time, OpenLoopInput.TracTqFL);
			WheelFL->ReqBrakeTorque = Lut1d(LocalTime, OpenLoopInput.Time, OpenLoopInput.BrkTqFL);
			WheelFL->ReqSteer = Lut1d(LocalTime, OpenLoopInput.Time, OpenLoopInput.SteerFL);

			WheelFR->ReqTorq = Lut1d(LocalTime, OpenLoopInput.Time, OpenLoopInput.TracTqFR);
			WheelFR->ReqBrakeTorque = Lut1d(LocalTime, OpenLoopInput.Time, OpenLoopInput.BrkTqFR);
			WheelFR->ReqSteer = Lut1d(LocalTime, OpenLoopInput.Time, OpenLoopInput.SteerFR);

			WheelRL->ReqTorq = Lut1d(LocalTime, OpenLoopInput.Time, OpenLoopInput.TracTqRL);
			WheelRL->ReqBrakeTorque = Lut1d(LocalTime, OpenLoopInput.Time, OpenLoopInput.BrkTqRL);
			WheelRL->ReqSteer = Lut1d(LocalTime, OpenLoopInput.Time, OpenLoopInput.SteerRL);

			WheelRR->ReqTorq = Lut1d(LocalTime, OpenLoopInput.Time, OpenLoopInput.TracTqRR);
			WheelRR->ReqBrakeTorque = Lut1d(LocalTime, OpenLoopInput.Time, OpenLoopInput.BrkTqRR);
			WheelRR->ReqSteer = Lut1d(LocalTime, OpenLoopInput.Time, OpenLoopInput.SteerRR);

			if (LocalTime > OpenLoopInput.Time.Last())
			{
				ScenarioIsDone();
			}

		}

	}

	if (ScenarioSt.ScenarioStatus == EScenarioStatus::Completed && bBrakeWhenScenarioIsDone)
	{
		WheelFL->ReqBrakeTorque = BrkTqReqWhenScenarioIsDone * 0.5 * BrkTqBalance;
		WheelFR->ReqBrakeTorque = BrkTqReqWhenScenarioIsDone * 0.5 * BrkTqBalance;
		WheelRL->ReqBrakeTorque = BrkTqReqWhenScenarioIsDone * 0.5 * (1 - BrkTqBalance);
		WheelRR->ReqBrakeTorque = BrkTqReqWhenScenarioIsDone * 0.5 * (1 - BrkTqBalance);
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
				FNotificationInfo Info(FText::FromString(FString("CSV template was created at: ")+ FilePath));
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
		FNotificationInfo Info(FText::FromString(FString("Load input Error: can't open the file")));
		Info.ExpireDuration = 5.0f;
		Info.Image = FCoreStyle::Get().GetBrush(TEXT("Icons.ErrorWithColor"));
		FSlateNotificationManager::Get().AddNotification(Info);
		return;
	}

	LoadCustomInput(OpenFilenames[0]);

	OpenLoopManeuverType = EOpenLoopManeuverType::CustomInputFile;
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
			FNotificationInfo Info(FText::FromString(FString("Load input Error: file has no data")+ FilePath));
			Info.ExpireDuration = 5.0f;
			Info.Image = FCoreStyle::Get().GetBrush(TEXT("Icons.ErrorWithColor"));
			FSlateNotificationManager::Get().AddNotification(Info);
			return false;
		}


		FileLines.RemoveAt(0);

		OpenLoopInput.Time.Reserve(FileLines.Num() - 1);
		OpenLoopInput.BrkTqFL.Reserve(FileLines.Num() - 1);
		OpenLoopInput.BrkTqFR.Reserve(FileLines.Num() - 1);
		OpenLoopInput.BrkTqRL.Reserve(FileLines.Num() - 1);
		OpenLoopInput.BrkTqRR.Reserve(FileLines.Num() - 1);
		OpenLoopInput.SteerFL.Reserve(FileLines.Num() - 1);
		OpenLoopInput.SteerFR.Reserve(FileLines.Num() - 1);
		OpenLoopInput.SteerRL.Reserve(FileLines.Num() - 1);
		OpenLoopInput.SteerRR.Reserve(FileLines.Num() - 1);
		OpenLoopInput.TracTqFL.Reserve(FileLines.Num() - 1);
		OpenLoopInput.TracTqFR.Reserve(FileLines.Num() - 1);
		OpenLoopInput.TracTqRL.Reserve(FileLines.Num() - 1);
		OpenLoopInput.TracTqRR.Reserve(FileLines.Num() - 1);

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
					FNotificationInfo Info(FText::FromString(FString("Load input Error: incorrect number of columns") + FilePath));
					Info.ExpireDuration = 5.0f;
					Info.Image = FCoreStyle::Get().GetBrush(TEXT("Icons.ErrorWithColor"));
					FSlateNotificationManager::Get().AddNotification(Info);
					bNoErrorPreviously = false;
				}
				
				continue;
			}

			int32 k = 0;


			OpenLoopInput.Time.Add(FCString::Atof(*ParsedValues[k])); k++;
			OpenLoopInput.SteerFL.Add(FCString::Atof(*ParsedValues[k]) * PI / 180); k++;
			OpenLoopInput.SteerFR.Add(FCString::Atof(*ParsedValues[k]) * PI / 180); k++;
			OpenLoopInput.SteerRL.Add(FCString::Atof(*ParsedValues[k]) * PI / 180); k++;
			OpenLoopInput.SteerRR.Add(FCString::Atof(*ParsedValues[k]) * PI / 180); k++;
			OpenLoopInput.TracTqFL.Add(FCString::Atof(*ParsedValues[k])); k++;
			OpenLoopInput.TracTqFR.Add(FCString::Atof(*ParsedValues[k])); k++;
			OpenLoopInput.TracTqRL.Add(FCString::Atof(*ParsedValues[k])); k++;
			OpenLoopInput.TracTqRR.Add(FCString::Atof(*ParsedValues[k])); k++;
			OpenLoopInput.BrkTqFL.Add(FCString::Atof(*ParsedValues[k])); k++;
			OpenLoopInput.BrkTqFR.Add(FCString::Atof(*ParsedValues[k])); k++;
			OpenLoopInput.BrkTqRL.Add(FCString::Atof(*ParsedValues[k])); k++;
			OpenLoopInput.BrkTqRR.Add(FCString::Atof(*ParsedValues[k]));

		}
	

		FNotificationInfo Info(FText::FromString(FString("Load input: success!")));
		Info.ExpireDuration = 5.0f;
		Info.Image = FCoreStyle::Get().GetBrush(TEXT("Icons.SuccessWithColor"));
		FSlateNotificationManager::Get().AddNotification(Info);

	return true;
}


void UVehicleInputOpenLoopComponent::ScenarioIsDone()
{
	ScenarioSt.ScenarioStatus = EScenarioStatus::Completed;
	ScenarioSt.bScenarioAcv = false;

	WheelFL->ReqTorq = 0.0;
	WheelFL->ReqBrakeTorque = 0.0;
	WheelFL->ReqSteer = 0.0;

	WheelFR->ReqTorq = 0.0;
	WheelFR->ReqBrakeTorque = 0.0;
	WheelFR->ReqSteer = 0.0;

	WheelRL->ReqTorq = 0.0;
	WheelRL->ReqBrakeTorque = 0.0;
	WheelRL->ReqSteer = 0.0;

	WheelRR->ReqTorq = 0.0;
	WheelRR->ReqBrakeTorque = 0.0;
	WheelRR->ReqSteer = 0.0;

	FNotificationInfo Info(FText::FromString(FString("Scenario is done, you can stop simulation now")));
	Info.ExpireDuration = 5.0f;
	Info.Image = FCoreStyle::Get().GetBrush(TEXT("Icons.SuccessWithColor"));
	FSlateNotificationManager::Get().AddNotification(Info);
}

float UVehicleInputOpenLoopComponent::Lut1(float x, float x0, float x1, float y0, float y1)
{
	float xToWorkWith = x;
	
	if (x > x1)
	{
		xToWorkWith = x1;
	}
	else if (x < x0)
	{
		xToWorkWith = x0;
	}
	else if (FMath::Abs(x1 - x0) < 0.00001)
	{
		return y0;
	}
	return y0 + (xToWorkWith - x0) * (y1 - y0) / (x1 - x0);

}

float UVehicleInputOpenLoopComponent::Lut1d(float X, const TArray<float>& BreakPoints, const TArray<float>& Values)
{


	// Check for the same size 

	if (BreakPoints.Num() != Values.Num() || (BreakPoints.Num() == 0))
	{
		return 0;
	}


	// Check for clipping

	if (X <= BreakPoints[0])
	{
		return Values[0];
	}
	else if (X >= BreakPoints[BreakPoints.Num() - 1])
	{
		return Values[Values.Num() - 1];
	}

	// Moving along 
	for (int32 Idx = 0; Idx <= BreakPoints.Num() - 2; Idx++)
	{

		if (X >= BreakPoints[Idx] && X < BreakPoints[Idx + 1])
		{
			return Lut1(X, BreakPoints[Idx], BreakPoints[Idx + 1], Values[Idx], Values[Idx + 1]);
		}

	}


	return 0.0;

}
