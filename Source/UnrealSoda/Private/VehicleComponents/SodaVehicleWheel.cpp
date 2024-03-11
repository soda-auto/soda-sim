// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/VehicleComponents/SodaVehicleWheel.h"
#include "Soda/Vehicles/SodaWheeledVehicle.h"
#include "Engine/Engine.h"
#include "Engine/Canvas.h"

USodaVehicleWheelComponent::USodaVehicleWheelComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	GUI.Category = TEXT("Wheels");
	GUI.IcanName = TEXT("SodaIcons.Tire");
	GUI.ComponentNameOverride = TEXT("Wheel");
	GUI.bIsPresentInAddMenu = true;
	//Common.UniqueVehiceComponentClass = ;
	Common.bIsTopologyComponent = true;
	Common.Activation = EVehicleComponentActivation::OnStartScenario;
}

/*
void USodaVehicleWheelComponent::OnRegistreVehicleComponent()
{

}
*/

void USodaVehicleWheelComponent::DrawDebug(UCanvas* Canvas, float& YL, float& YPos)
{
	Super::DrawDebug(Canvas, YL, YPos);

	if (Common.bDrawDebugCanvas)
	{
		UFont* RenderFont = GEngine->GetSmallFont();
		Canvas->SetDrawColor(FColor::White);
		YPos += Canvas->DrawText(RenderFont, FString::Printf(TEXT("ReqTorq:%.1fH/m, ReqBrake:%.1fH/m, ReqSteer:%.1fdeg"), ReqTorq, ReqBrakeTorque, ReqSteer / M_PI * 180.0), 16, YPos);
		YPos += Canvas->DrawText(RenderFont, FString::Printf(TEXT("Ang Vel:%.1frad/s, Slip:[%.1f, %.1f], Sus Offset: %.1fcm"), AngularVelocity, Slip.X, Slip.Y, SuspensionOffset), 16, YPos);
		
	}
}

FVector USodaVehicleWheelComponent::GetWheelLocalVelocity() const
{
	FTransform Transform(
		FRotator(0, Steer / M_PI * 180, 0),
		GetWheelLocation(false, false),
		FVector(1));
	return GetWheeledVehicle()->GetSimData().VehicleKinematic.Curr.GetLocalVelocityAtLocalPose(Transform);
}

FVector USodaVehicleWheelComponent::GetWheelLocation( bool bWithSuspensionOffset, bool bInWorldSpase) const
{
	FVector Location = RestingLocation;
	if (bWithSuspensionOffset)
	{
		Location += FVector(0, 0, SuspensionOffset);
	}

	if (bInWorldSpase)
	{
		return GetWheeledVehicle()->GetSimData().VehicleKinematic.Curr.GlobalPose.InverseTransformPositionNoScale(Location);
	}
	else
	{
		return Location;
	}
}

void USodaVehicleWheelComponent::PassTorque(float InTorque) 
{ 
	ReqTorq = InTorque; 
	if (bVerboseLog)
	{
		UE_LOG(LogSoda, Log, TEXT("USodaVehicleWheelComponent::PassTorque(); InTorque: %f"), InTorque);
	}
}

float USodaVehicleWheelComponent::ResolveAngularVelocity() const 
{ 
	if (bVerboseLog)
	{
		UE_LOG(LogSoda, Log, TEXT("USodaVehicleWheelComponent::ResolveAngularVelocity(); AngularVelocity: %f"), AngularVelocity);
	}
	return  AngularVelocity; 
}


