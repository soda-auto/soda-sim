// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/VehicleComponents/Sensors/Generic/GenericRadarSensor.h"

UGenericRadarSensor::UGenericRadarSensor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	GUI.ComponentNameOverride = TEXT("Generic Radar");
	GUI.bIsPresentInAddMenu = true;

	FRadarParams& RadarParamsNearest = RadarParams.Add_GetRef(FRadarParams());
	RadarParamsNearest.Tag = "Nearest";
	RadarParamsNearest.FOV_HorizontMax = 60;
	RadarParamsNearest.FOV_HorizontFullDist = 40;
	RadarParamsNearest.FOV_Vertical = 20;
	RadarParamsNearest.DistanseMin = 0.2;
	RadarParamsNearest.DistanseMax = 70 * 0.18;
	RadarParamsNearest.DistanseOnMaxAngle = 40;
	RadarParamsNearest.HorizontalBestResolution = 1.0f;
	RadarParamsNearest.HorizontalResolutionFullDistAngle = 4.5f;
	RadarParamsNearest.HorizontalResolutionMaxAngle = 12.3f;
	RadarParamsNearest.MinSignalCoef = 0.002f;
	RadarParamsNearest.FOVSetup.MaxViewDistance = 600;

	FRadarParams& RadarParamsNear = RadarParams.Add_GetRef(FRadarParams());
	RadarParamsNear.Tag = "Near";
	RadarParamsNear.FOV_HorizontMax = 60;
	RadarParamsNear.FOV_HorizontFullDist = 40;
	RadarParamsNear.FOV_Vertical = 20;
	RadarParamsNear.DistanseMin = 70 * 0.18;
	RadarParamsNear.DistanseMax = 70;
	RadarParamsNear.DistanseOnMaxAngle = 40;
	RadarParamsNear.HorizontalBestResolution = 1.0f;
	RadarParamsNear.HorizontalResolutionFullDistAngle = 4.5f;
	RadarParamsNear.HorizontalResolutionMaxAngle = 12.3f;
	RadarParamsNear.MinSignalCoef = 0.002f;
	RadarParamsNear.FOVSetup.MaxViewDistance = 800;

	FRadarParams& RadarParamsFar = RadarParams.Add_GetRef(FRadarParams());
	RadarParamsFar.Tag = "Far";
	RadarParamsFar.FOV_HorizontMax = 9;
	RadarParamsFar.FOV_HorizontFullDist = 4;
	RadarParamsFar.FOV_Vertical = 20;
	RadarParamsFar.DistanseMin = 1;
	RadarParamsFar.DistanseMax = 250;
	RadarParamsFar.DistanseOnMaxAngle = 150;
	RadarParamsFar.HorizontalBestResolution = 1.0f;
	RadarParamsFar.HorizontalResolutionFullDistAngle = 4.5f;
	RadarParamsFar.HorizontalResolutionMaxAngle = 12.3f;
	RadarParamsFar.MinSignalCoef = 0.002f;
	RadarParamsFar.FOVSetup.MaxViewDistance = 1000;

}

bool UGenericRadarSensor::OnActivateVehicleComponent()
{
	if (Super::OnActivateVehicleComponent())
	{
		if (IsValid(Publisher))
		{
			return Publisher->AdvertiseAndSetHealth(this);
		}
	}
	return true;
}

void UGenericRadarSensor::OnDeactivateVehicleComponent()
{
	Super::OnDeactivateVehicleComponent();
	if (IsValid(Publisher))
	{
		Publisher->Shutdown();
	}
}

bool UGenericRadarSensor::IsVehicleComponentInitializing() const
{
	return IsValid(Publisher) && Publisher->IsInitializing();
}

void UGenericRadarSensor::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (IsValid(Publisher))
	{
		Publisher->CheckStatus(this);
	}
}

bool UGenericRadarSensor::PublishSensorData(float DeltaTime, const FSensorDataHeader& Header, const FRadarClusters& InClusters, const FRadarObjects& InObjects)
{
	if (Publisher && Publisher->IsOk())
	{
		return Publisher->Publish(DeltaTime, Header, GetRadarParams(), InClusters, InObjects);
	}
	return false;
}

void UGenericRadarSensor::DrawDebug(UCanvas* Canvas, float& YL, float& YPos)
{
	Super::DrawDebug(Canvas, YL, YPos);
	if (Publisher)
	{
		Publisher->DrawDebug(Canvas, YL, YPos);
	}
}

FString UGenericRadarSensor::GetRemark() const
{
	return Publisher ? Publisher->GetRemark() : "null";
}
