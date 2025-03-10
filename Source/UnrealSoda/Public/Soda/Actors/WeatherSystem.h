// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "Engine/SkyLight.h"
#include "Engine/ExponentialHeightFog.h"
#include "Engine/DirectionalLight.h"
#include "GameFramework/Actor.h"
#include "Soda/ISodaActor.h"
#include "Soda/ISodaDataset.h"
#include "WeatherSystem.generated.h"

DEFINE_LOG_CATEGORY_STATIC(WEATHER, Log, All);

class UMaterialInstanceDynamic;
class AVolumetricCloud;

UCLASS(ClassGroup = Soda, meta = (BlueprintSpawnableComponent))
class UNREALSODA_API AWeatherSystem 
	: public AActor
	, public ISodaActor
	, public IObjectDataset
{
	GENERATED_BODY()

public:
	AWeatherSystem();
	virtual void BeginPlay() override;
	virtual void PreInitializeComponents() override;
	virtual void Serialize(FArchive& Ar) override;
	
	/* Update weather in level*/
	UFUNCTION(BlueprintCallable, Category = Weather, meta = (CallInRuntime))
	void UpdateWeather();

public:
	UPROPERTY(EditAnywhere, Category = Weather)
	ADirectionalLight * Sun;

	UPROPERTY(EditAnywhere, Category = Weather)
	ASkyLight * SkyLight;

	UPROPERTY(EditAnywhere, Category = Weather)
	AExponentialHeightFog * ExponentialHeightFog;

	UPROPERTY(EditAnywhere, Category = Weather)
	AVolumetricCloud * VolumetricClouds;

	UPROPERTY(EditAnywhere, Category = Weather)
	UClass* FogClass;

	UPROPERTY(EditAnywhere, Category = Weather)
	UClass* RoadPlannerClass;

	UPROPERTY(EditAnywhere, Category = Weather, SaveGame, meta = (EditInRuntime))
	float DayTime = 12.f;

	// Enable fog on UPS_Depot map roads
	UPROPERTY(EditAnywhere, Category = Weather, SaveGame, meta = (EditInRuntime))
	bool RoadsFog = false;

	// Enable all user placed LocalFog actors
	UPROPERTY(EditAnywhere, Category = Weather, SaveGame, meta = (EditInRuntime))
	bool LocalFog = true;

	// Overall Mist Dencity, percent (0.0 - 100.0]
	UPROPERTY(EditAnywhere, Category = Weather, SaveGame, meta = (EditInRuntime))
	float OverallMistDensity = 2.f;

	UPROPERTY(EditAnywhere, Category = Weather, SaveGame, meta = (EditInRuntime))
	bool EnableVolumetricClouds = false;

	// Defines clouds opacity, percent [0 - 100]
	UPROPERTY(EditAnywhere, Category = Weather, SaveGame, meta = (EditInRuntime))
	float VolumetricCloudsDencity = 20.f;

	// Clouds Layout, [1.0 - 5.0]
	// 1 - Overcast,
	// 5 - Cumulus clouds
	UPROPERTY(EditAnywhere, Category = Weather, SaveGame, meta = (EditInRuntime))
	float VolumetricCloudsLayout = 3.f;

public:
	/* Override from ISodaActor */
	//virtual void OnSelect_Implementation() override;
	//virtual void OnUnselect_Implementation() override;
	virtual const FSodaActorDescriptor* GenerateActorDescriptor() const override;
	//virtual bool OnSetPinnedActor(bool bIsPinnedActor) override;
	//virtual bool IsPinnedActor() const override;

private:
	UMaterialInstanceDynamic* CloudsDynamicMaterial = nullptr;
};
