// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "Soda/VehicleComponents/VehicleSensorComponent.h"
#include "Soda/VehicleComponents/GenericPublishers/GenericWheeledVehiclePublisher.h"
#include "Soda/VehicleComponents/VehicleDriverComponent.h"
#include "Soda/Vehicles/VehicleBaseTypes.h"
#include "GenericWheeledVehicleSensor.generated.h"

class ASodaWheeledVehicle;
class UVehicleGearBoxBaseComponent;
//class UVehicleBrakeSystemBaseComponent;
//class UVehicleEngineBaseComponent;
//class UVehicleSteeringRackBaseComponent;
//class UVehicleHandBrakeBaseComponent;


struct FWheeledVehicleStateExtra
{
	FPhysBodyKinematic BodyKinematic{};
	FTransform RelativeTransform{};
	ENGear Gear{};
	ESodaVehicleDriveMode DriveMode{};
	FWheeledVehicleWheelState WheelStates[4];
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

	//UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Link, SaveGame, meta = (EditInRuntime, ReactivateComponent, AllowedClasses = "/Script/UnrealSoda.VehicleHandBrakeBaseComponent"))
	//FSubobjectReference LinkToHandBrake{ TEXT("HandBrake") };

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Link, SaveGame, meta = (EditInRuntime, ReactivateComponent, AllowedClasses = "/Script/UnrealSoda.VehicleGearBoxBaseComponent"))
	FSubobjectReference LinkToGearBox{ TEXT("GearBox") };

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Link, SaveGame, meta = (EditInRuntime, ReactivateComponent, AllowedClasses = "/Script/UnrealSoda.VehicleDriverComponent"))
	FSubobjectReference LinkToVehicleDriver{ TEXT("VehicleDriver") };


protected:
	//UPROPERTY(BlueprintReadOnly, Category = Link)
	//UVehicleBrakeSystemBaseComponent* BrakeSystem = nullptr;

	//UPROPERTY(BlueprintReadOnly, Category = Link)
	//UVehicleEngineBaseComponent* Engine = nullptr;

	//UPROPERTY(BlueprintReadOnly, Category = Link)
	//UVehicleSteeringRackBaseComponent* SteeringRack = nullptr;

	//UPROPERTY(BlueprintReadOnly, Category = Link)
	//UVehicleHandBrakeBaseComponent* HandBrake = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = Link)
	UVehicleGearBoxBaseComponent* GearBox = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = Link)
	ASodaWheeledVehicle* WheeledVehicle = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = Link)
	UVehicleDriverComponent* VehicleDriver = nullptr;


protected:
	virtual bool PublishSensorData(float DeltaTime, const FSensorDataHeader& Header,const FWheeledVehicleStateExtra& VehicleState);

protected:
	virtual void RuntimePostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent) override;
	virtual bool OnActivateVehicleComponent() override;
	virtual void OnDeactivateVehicleComponent() override;
	virtual bool IsVehicleComponentInitializing() const override;
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
};