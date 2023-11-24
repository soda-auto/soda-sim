// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/Actors/WeatherSystem.h"
#include "Soda/UnrealSoda.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/VolumetricCloudComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Misc/OutputDeviceNull.h"
#include "UObject/UObjectIterator.h"

AWeatherSystem::AWeatherSystem()
{
}

void AWeatherSystem::PreInitializeComponents()
{
	Super::PreInitializeComponents();
}


void AWeatherSystem::BeginPlay()
{
	Super::BeginPlay();
	UpdateWeather();
}

void AWeatherSystem::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);

	if(Ar.IsLoading() && Ar.IsSaveGame() && HasActorBegunPlay())
	if (Ar.IsLoading())
	{
		UpdateWeather();
	}
}

void AWeatherSystem::UpdateWeather()
{
	if (Sun)
	{
		float SunAngle = FMath::Clamp(DayTime / 24.f, 0.f, 1.f) * 360.f + 90.f;
		FRotator Rotator(SunAngle, 0.f, 0.f);
		Sun->SetActorRotation(Rotator);
	}
	for (TObjectIterator<AActor> Itr; Itr; ++Itr)
	{
		if (Itr->IsA(FogClass))
		{
			AActor* FogActor = *Itr;
			if (FogActor->GetParentActor() && FogActor->GetParentActor()->IsA(RoadPlannerClass))
			{
				FogActor->SetActorHiddenInGame(!RoadsFog);
			}
			else
			{
				FogActor->SetActorHiddenInGame(!LocalFog);
			}
		}
	}
	if (ExponentialHeightFog)
	{
		OverallMistDensity = FMath::Clamp(OverallMistDensity, 0.01f, 100.f);
		ExponentialHeightFog->GetComponent()->SetFogDensity(OverallMistDensity * 0.01f);
	}
	if (VolumetricClouds)
	{
		VolumetricClouds->SetActorHiddenInGame(!EnableVolumetricClouds);
		if (!CloudsDynamicMaterial)
		{
			if (UVolumetricCloudComponent* VolumetricCloudComponent = VolumetricClouds->FindComponentByClass<UVolumetricCloudComponent>())
			{
				CloudsDynamicMaterial = UMaterialInstanceDynamic::Create(VolumetricCloudComponent->Material, VolumetricCloudComponent);
				VolumetricCloudComponent->SetMaterial(CloudsDynamicMaterial);
			}
		}
		if (CloudsDynamicMaterial)
		{
			VolumetricCloudsDencity = FMath::Clamp(VolumetricCloudsDencity, 0.f, 100.f);
			VolumetricCloudsLayout = FMath::Clamp(VolumetricCloudsLayout, 1.f, 5.f);
			CloudsDynamicMaterial->SetScalarParameterValue(TEXT("ExtinctionScale"), VolumetricCloudsDencity * 0.0002f);
			CloudsDynamicMaterial->SetScalarParameterValue(TEXT("WeatherUVScale"), VolumetricCloudsLayout * 0.0001);
		}
	}
	if (SkyLight)
	{
		SkyLight->GetLightComponent()->RecaptureSky();
	}
}


const FSodaActorDescriptor* AWeatherSystem::GenerateActorDescriptor() const
{
	static FSodaActorDescriptor Desc {
		TEXT("Weather System"), /*DisplayName*/
		TEXT("Scenario"), /*Category*/
		TEXT("Experimental"), /*SubCategory*/
		TEXT("SodaIcons.Weather"), /*Icon*/
		false, /*bAllowTransform*/
		true, /*bAllowSpawn*/
		FVector(0, 0, 0), /*SpawnOffset*/
	};
	return &Desc;
}