// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "Soda/VehicleComponents/VehicleSensorComponent.h"
#include "V2XSensor.generated.h"

/*
struct FV2XObject
{
	uint64 ID;
	FTransform Transform;
	FVector AngularVelocity;
	FVector WorldVelocity;
	FVector LocalVelocity;
	FExtent Extent;
	TTimestamp Timestamp;
	ESegmObjectLabel Label;
	TWeakObjectPtr<AActor> Actor;
};
*/

/**
 * UV2XMarkerSensor
 */
UCLASS(ClassGroup = Soda, BlueprintType, meta = (BlueprintSpawnableComponent))
class UNREALSODA_API UV2XMarkerSensor : public USensorComponent
{
	GENERATED_UCLASS_BODY()

public:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	virtual bool OnActivateVehicleComponent() override;
	virtual void OnDeactivateVehicleComponent() override;
	virtual void GetRemark(FString& Info) const override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = V2X, SaveGame, meta = (EditInRuntime))
	FBox Bound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = V2X, SaveGame, meta = (EditInRuntime))
	int ID = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = V2X, SaveGame, meta = (EditInRuntime))
	bool bAssignUniqueIdAtStartup = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = V2X, SaveGame, meta = (EditInRuntime))
	bool bAutoCalculateBoundingBox = true;

public:
	UFUNCTION(BlueprintCallable, Category = V2X)
	virtual void AssignUniqueId() { ID = ++IDsCounter; }

	UFUNCTION(BlueprintCallable, Category = V2X)
	virtual void CalculateBoundingBox();

	virtual FTransform GetV2XTransform() const;
	virtual FVector GetV2XWorldVelocity() const;
	virtual FVector GetV2XWorldAngVelocity() const;

protected:
	static int IDsCounter;
};

/**
 * UV2XReceiverSensor 
 */
UCLASS(abstract, ClassGroup = Soda, BlueprintType, meta = (BlueprintSpawnableComponent))
class UNREALSODA_API UV2XReceiverSensor : public USensorComponent
{
	GENERATED_UCLASS_BODY()

	/** [cm] */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = V2X, SaveGame, meta = (EditInRuntime))
	float Radius = 10000; 

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = V2X, SaveGame, meta = (EditInRuntime))
	bool bRecalculateBoundingBoxEveryTick = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = V2X, SaveGame, meta = (EditInRuntime))
	bool bDrawDebugBoxes = false;

protected:
	virtual bool PublishSensorData(float DeltaTime, const FSensorDataHeader& Header, const TArray<UV2XMarkerSensor*>& InTransmitters) { return false; }

protected:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	TArray<UV2XMarkerSensor*> Transmitters;
};
