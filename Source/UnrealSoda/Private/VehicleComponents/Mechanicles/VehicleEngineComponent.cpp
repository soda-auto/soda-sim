// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/VehicleComponents/Mechanicles/VehicleEngineComponent.h"
#include "Soda/UnrealSoda.h"
#include "Soda/Vehicles/SodaWheeledVehicle.h"
#include "Soda/SodaStatics.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Soda/VehicleComponents/VehicleInputComponent.h"
#include "UObject/ConstructorHelpers.h"

#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/PlatformFileManager.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "DesktopPlatformModule.h"

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
		if (bCustomTorqueCurve)
		{
			MaxTorq = LookupTable1d(AngularVelocity * ANG2RPM, CustomTorqueCurveRPM, CustomTorqueCurveTorque) * TorqueCurveMultiplier;
		}
		else
		{
			MaxTorq = TorqueCurve.ExternalCurve->GetFloatValue(AngularVelocity * ANG2RPM) * TorqueCurveMultiplier;
		}
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

	if (bEnableLossesCalculation)
	{
		InitPowerLossesData();
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

		if (bEnableLossesCalculation)
		{
			CalculatePowerLosses();
		}


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

	SyncDataset();
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




void UVehicleEngineBaseComponent::LoadCsvRPMvsTorqueCustomCurve()
{
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	if (!DesktopPlatform)
	{
		UE_LOG(LogSoda, Error, TEXT("UVehicleEngineBaseComponent::LoadCsvRPMvsTorqueCustomCurve() Can't get the IDesktopPlatform ref"));
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

	FString& FilePath = OpenFilenames[0];

	if (!FPlatformFileManager::Get().GetPlatformFile().FileExists(*FilePath))
	{
		UE_LOG(LogTemp, Error, TEXT("File does not exist: %s"), *FilePath);
		return;
	}



	TArray<FString> FileLines;
	if (!FFileHelper::LoadFileToStringArray(FileLines, *FilePath))
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to load file: %s"), *FilePath);
		return;
	}


	if (FileLines.Num() < 2)
	{
		FNotificationInfo Info(FText::FromString(FString("Load input Error: file has no data") + FilePath));
		Info.ExpireDuration = 5.0f;
		Info.Image = FCoreStyle::Get().GetBrush(TEXT("Icons.ErrorWithColor"));
		FSlateNotificationManager::Get().AddNotification(Info);
		return;
	}




	bool bErrorWasShown = false;

	CustomTorqueCurveRPM.Empty();
	CustomTorqueCurveTorque.Empty();


	for (int32 i = 1; i < FileLines.Num(); i++) // Skip the header line
	{
		TArray<FString> Values;
		FileLines[i].ParseIntoArray(Values, TEXT(","), true);

		if (Values.Num() < 2)
		{
			if (!bErrorWasShown)
			{
				FNotificationInfo Info(FText::FromString(FString("Load input Error: some of the lines are invalid") + FilePath));
				Info.ExpireDuration = 5.0f;
				Info.Image = FCoreStyle::Get().GetBrush(TEXT("Icons.ErrorWithColor"));
				FSlateNotificationManager::Get().AddNotification(Info);
				bErrorWasShown = true;
			}

			UE_LOG(LogTemp, Error, TEXT("Invalid data at line %d"), i);
			continue;
		}

		CustomTorqueCurveRPM.Add(FCString::Atof(*Values[0]));
		CustomTorqueCurveTorque.Add(FCString::Atof(*Values[1]));

	}
	bCustomTorqueCurve = true;




}