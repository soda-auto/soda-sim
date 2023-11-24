// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/VehicleComponents/Sensors/RadarGenericSensor.h"
#include "Soda/UnrealSoda.h"
#include "Soda/SodaApp.h"
#include "Soda/SodaStatics.h"


UGeneralRadarSensorComponent::UGeneralRadarSensorComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	GUI.ComponentNameOverride = TEXT("GeneralRadar");
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

bool UGeneralRadarSensorComponent::OnActivateVehicleComponent()
{
	if (!Super::OnActivateVehicleComponent())
	{
		return false;
	}

	PointCloudPublisher.Advertise();
	if (!PointCloudPublisher.IsAdvertised())
	{
		SetHealth(EVehicleComponentHealth::Error, TEXT("Can't advertise publisher"));
		return false;
	}

	return true;
}

void UGeneralRadarSensorComponent::OnDeactivateVehicleComponent()
{
	Super::OnDeactivateVehicleComponent();
	PointCloudPublisher.Shutdown();
}

void UGeneralRadarSensorComponent::PublishResults()
{
	Scan.items.resize(Clusters.Clusters.Num());
	Scan.device_timestamp = soda::RawTimestamp<std::chrono::milliseconds>( SodaApp.GetSimulationTimestamp());

	FVector Loc = GetComponentLocation();
	FRotator Rot = GetComponentRotation();

	switch (GetRadarMode())
	{
	case ERadarMode::ClusterMode:
		for (int i = 0; i < Clusters.Clusters.Num(); ++i)
		{
			soda::RadarCluster& Pt = Scan.items[i];
			FVector TracedPoint = Rot.UnrotateVector(Clusters.Clusters[i].HitPosition - Loc) / 100.0;
			Pt.id = i;
			Pt.coords.x = TracedPoint.X;
			Pt.coords.y = -TracedPoint.Y;
			Pt.coords.z = 0;
			Pt.velocity.lat = Clusters.Clusters[i].Lat;
			Pt.velocity.lon = Clusters.Clusters[i].Lon;
			Pt.property = soda::RadarCluster::property_t::moving;
			Pt.RCS = Clusters.Clusters[i].RCS;
		}
		{
			//SCOPE_CYCLE_COUNTER(STAT_RadarPublishResults);
			PointCloudPublisher.Publish(Scan);
		}
		break;

	case ERadarMode::ObjectMode:
		break;
	}
}

void UGeneralRadarSensorComponent::GetRemark(FString& Info) const
{
	Info = PointCloudPublisher.Address + ":" + FString::FromInt(PointCloudPublisher.Port);
}

