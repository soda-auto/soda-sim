// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "Engine/DataAsset.h"
#include "Engine/AssetManagerTypes.h"
#include "Engine/EngineTypes.h"
#include "Soda/ISodaActor.h"
#include "Soda/SodaApp.h"
#include "SodaLibraryPrimaryAsset.generated.h"

class UActorComponent;
class AActor;

/*
USTRUCT(BlueprintType)
struct FActorGroundTruthRecord
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ActorGroundTruth)
	TSubclassOf<AActor> Class;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ActorGroundTruth, BlueprintAssignable)
	FOnGenerateActorGroundTruth OnGenerate;

};
*/

UCLASS()
class UNREALSODA_API USodaLibraryPrimaryAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "SodaLibraryPrimaryAsset", meta = (AssetBundles = "SodaActors"))
	TMap<TSoftClassPtr<AActor>, FSodaActorDescriptor> SodaActors;

	UPROPERTY(EditAnywhere, Category = "SodaLibraryPrimaryAsset", meta = (AssetBundles = "VehicleComponents"))
	TArray<TSoftClassPtr<UActorComponent>> VehicleComponents;

	//UPROPERTY(EditAnywhere, Category = "SodaLibraryPrimaryAsset", meta = (AssetBundles = "ActorGroundTruth"))
	//TArray<FActorGroundTruthRecord> ActorGroundTruthExporter;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override;
};