// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "Soda/VehicleComponents/VehicleSensorComponent.h"
#include "Soda/GenericPublishers/GenericWheeledVehiclePublisher.h"
#include "Soda/VehicleComponents/VehicleDriverComponent.h"
#include "Soda/Vehicles/VehicleBaseTypes.h"
#include "GenericWheeledVehicleSensor.generated.h"

class ASodaWheeledVehicle;
class UVehicleGearBoxBaseComponent;
//class UVehicleBrakeSystemBaseComponent;
//class UVehicleEngineBaseComponent;
//class UVehicleSteeringRackBaseComponent;


struct UNREALSODA_API FWheeledVehicleSensorData
{
	FWheeledVehicleSensorData()
		: BodyKinematic(nullptr)
		, RelativeTransform(FTransform())
		, WheeledVehicle(nullptr)
		, GearBox(nullptr)
		, VehicleDriver(nullptr)
	{}

	FWheeledVehicleSensorData(
		const FPhysBodyKinematic * InBodyKinematic,
		const FTransform & InRelativeTransform,
		const ASodaWheeledVehicle* InWheeledVehicle,
		const UVehicleGearBoxBaseComponent* InGearBox,
		const UVehicleDriverComponent * InVehicleDriver)
		: BodyKinematic(InBodyKinematic)
		, RelativeTransform(InRelativeTransform)
		, WheeledVehicle(InWheeledVehicle)
		, GearBox(InGearBox)
		, VehicleDriver(InVehicleDriver)
	{}



	const FPhysBodyKinematic* BodyKinematic;
	FTransform RelativeTransform;
	const ASodaWheeledVehicle* WheeledVehicle;
	const UVehicleGearBoxBaseComponent* GearBox;
	const UVehicleDriverComponent* VehicleDriver;
};

UCLASS(ClassGroup = Soda, BlueprintType, meta = (BlueprintSpawnableComponent))
class UNREALSODA_API UGenericWheeledVehicleSensor : public USensorComponent
{
	GENERATED_UCLASS_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Publishing, SaveGame, meta = (EditInRuntime, ReactivateComponent))
	TSubclassOf<UGenericWheeledVehiclePublisher> PublisherClass;

	UPROPERTY(EditAnywhere, Instanced, Category = Publishing, SaveGame, meta = (EditInRuntime))
	TObjectPtr<UGenericWheeledVehiclePublisher> Publisher;

	//UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Link, SaveGame, meta = (EditInRuntime, ReactivateComponent, AllowedClasses = "/Script/UnrealSoda.VehicleEngineBaseComponent"))
	//FSubobjectReference LinkToEngine{ TEXT("Engine") };

	//UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Link, SaveGame, meta = (EditInRuntime, ReactivateComponent, AllowedClasses = "/Script/UnrealSoda.VehicleSteeringRackBaseComponent"))
	//FSubobjectReference LinkToSteering{ TEXT("SteeringRack") };

	//UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Link, SaveGame, meta = (EditInRuntime, ReactivateComponent, AllowedClasses = "/Script/UnrealSoda.VehicleBrakeSystemBaseComponent"))
	//FSubobjectReference LinkToBrakeSystem{ TEXT("BrakeSystem") };

	//UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Link, SaveGame, meta = (EditInRuntime, ReactivateComponent, AllowedClasses = "/Script/UnrealSoda.VehicleBrakeSystemBaseComponent"))
	//FSubobjectReference LinkToHandBrake{ TEXT("HandBrake") };

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Link, SaveGame, meta = (EditInRuntime, ReactivateComponent, AllowedClasses = "/Script/UnrealSoda.VehicleGearBoxBaseComponent"))
	FSubobjectReference LinkToGearBox{ TEXT("GearBox") };

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Link, SaveGame, meta = (EditInRuntime, ReactivateComponent, AllowedClasses = "/Script/UnrealSoda.VehicleDriverComponent"))
	FSubobjectReference LinkToVehicleDriver{ TEXT("VehicleDriver") };


	UVehicleGearBoxBaseComponent* GetGearBox() const { return GearBox; }
	UVehicleDriverComponent* GetVehicleDriver() const { return VehicleDriver; }
	ASodaWheeledVehicle* GetWheeledVehicle() const { return WheeledVehicle; }

protected:
	//UPROPERTY(BlueprintReadOnly, Category = Link)
	//UVehicleBrakeSystemBaseComponent* BrakeSystem = nullptr;

	//UPROPERTY(BlueprintReadOnly, Category = Link)
	//UVehicleEngineBaseComponent* Engine = nullptr;

	//UPROPERTY(BlueprintReadOnly, Category = Link)
	//UVehicleSteeringRackBaseComponent* SteeringRack = nullptr;

	//UPROPERTY(BlueprintReadOnly, Category = Link)
	//UVehicleBrakeSystemBaseComponent* HandBrake = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = Link)
	UVehicleGearBoxBaseComponent* GearBox = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = Link)
	ASodaWheeledVehicle* WheeledVehicle = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = Link)
	UVehicleDriverComponent* VehicleDriver = nullptr;

	const FWheeledVehicleSensorData& GetSensorData() const { return SensorData; }


protected:
	virtual bool PublishSensorData(float DeltaTime, const FSensorDataHeader& Header,const FWheeledVehicleSensorData& VehicleState);

protected:
	virtual void RuntimePostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent) override;
	virtual bool OnActivateVehicleComponent() override;
	virtual void OnDeactivateVehicleComponent() override;
	virtual bool IsVehicleComponentInitializing() const override;
	virtual FString GetRemark() const override;
	virtual void DrawDebug(UCanvas* Canvas, float& YL, float& YPos) override;
	virtual void PostPhysicSimulationDeferred(float DeltaTime, const FPhysBodyKinematic& VehicleKinematic, const TTimestamp& Timestamp) override;

protected:
	virtual void Serialize(FArchive& Ar) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
#if WITH_EDITOR
	virtual void PostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent) override;
	virtual void PostInitProperties() override;
#endif

protected:
	FGenericPublisherHelper<UGenericWheeledVehicleSensor, UGenericWheeledVehiclePublisher> PublisherHelper{ this, &UGenericWheeledVehicleSensor::PublisherClass, &UGenericWheeledVehicleSensor::Publisher };
	FWheeledVehicleSensorData SensorData;
};
