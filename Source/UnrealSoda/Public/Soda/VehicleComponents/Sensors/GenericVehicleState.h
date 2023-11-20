// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "Soda/VehicleComponents/Sensors/ImuSensor.h"
#include "Soda/Transport/GenericVehicleStatePublisher.h"
#include "GenericVehicleState.generated.h"

class UVehicleDriverComponent;
class ASodaWheeledVehicle;

UCLASS(ClassGroup = Soda, BlueprintType, meta = (BlueprintSpawnableComponent))
class UNREALSODA_API UGenericVehicleStateComponent : public UImuSensorComponent
{
	GENERATED_UCLASS_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sensor, SaveGame, meta = (EditInRuntime))
	FGenericVehicleStatePublisher Publisher;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Link, SaveGame, meta = (EditInRuntime, ReactivateComponent, AllowedClasses = "/Script/SodaSim.VehicleDriverComponent"))
	FSubobjectReference LinkToVehicleDriver { TEXT("VehicleDriver") };

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	virtual bool OnActivateVehicleComponent() override;
	virtual void OnDeactivateVehicleComponent() override;
	virtual void GetRemark(FString& Info) const override;
	virtual void PostPhysicSimulationDeferred(float DeltaTime, const FPhysBodyKinematic& VehicleKinematic, const TTimestamp& Timestamp) override;
	
protected:
	bool bIs4WDVehicle = false;

	UPROPERTY(BlueprintReadOnly, Category = Link)
	UVehicleDriverComponent * VehicleDriver = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = Link)
	ASodaWheeledVehicle * WheeledVehicle = nullptr;

};
