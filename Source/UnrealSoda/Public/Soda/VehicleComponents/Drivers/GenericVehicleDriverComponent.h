// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "Soda/VehicleComponents/VehicleDriverComponent.h"
#include "Soda/GenericPublishers/GenericWheeledVehiclePublisher.h"
#include "Soda/GenericPublishers/GenericWheeledVehicleControl.h"
#include "GenericVehicleDriverComponent.generated.h"

class UVehicleBrakeSystemBaseComponent;
class UVehicleEngineBaseComponent;
class UVehicleSteeringRackBaseComponent;
class UVehicleHandBrakeBaseComponent;
class UVehicleGearBoxBaseComponent;

namespace soda
{
	struct FWheeledVehiclControlMode1
	{
		float SteerReq;
		float AccDecelReq;
		EGearState GearStateReq;
		/**
		 * Desire gear number for the drive and revers gear;
		 * Values:
		 *   - 0        - automatic/undefined;
		 *   - others   - desire gear number;
		 */
		int8 GearNumReq;
		TTimestamp RecvTimestamp;
	};

} // namespace soda


UCLASS(ClassGroup = Soda, BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class UNREALSODA_API UGenericVehicleDriverComponentComponent : public UVehicleDriverComponent
{
	GENERATED_UCLASS_BODY()

	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Publishing, SaveGame, meta = (EditInRuntime))
	//TSubclassOf<UGenericWheeledVehiclePublisher> PublisherClass;

	//UPROPERTY(EditAnywhere, Instanced, Category = Publishing, SaveGame, meta = (EditInRuntime))
	//TObjectPtr<UGenericWheeledVehiclePublisher> Publisher;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Publishing, SaveGame, meta = (EditInRuntime))
	TSubclassOf<UGenericWheeledVehicleControlListener> VehicleControlClass;

	UPROPERTY(EditAnywhere, Instanced, Category = Publishing, SaveGame, meta = (EditInRuntime))
	TObjectPtr<UGenericWheeledVehicleControlListener> VehicleControl;


	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Link, SaveGame, meta = (EditInRuntime, ReactivateComponent, AllowedClasses = "/Script/UnrealSoda.VehicleEngineBaseComponent"))
	FSubobjectReference LinkToEngine { TEXT("Engine") };

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Link, SaveGame, meta = (EditInRuntime, ReactivateComponent, AllowedClasses = "/Script/UnrealSoda.VehicleSteeringRackBaseComponent"))
	FSubobjectReference LinkToSteering { TEXT("SteeringRack") };

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Link, SaveGame, meta = (EditInRuntime, ReactivateComponent, AllowedClasses = "/Script/UnrealSoda.VehicleBrakeSystemBaseComponent"))
	FSubobjectReference LinkToBrakeSystem { TEXT("BrakeSystem") };

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Link, SaveGame, meta = (EditInRuntime, ReactivateComponent, AllowedClasses = "/Script/UnrealSoda.VehicleHandBrakeBaseComponent"))
	FSubobjectReference LinkToHandBrake { TEXT("HandBrake") };

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Link, SaveGame, meta = (EditInRuntime, ReactivateComponent, AllowedClasses = "/Script/UnrealSoda.VehicleGearBoxBaseComponent"))
	FSubobjectReference LinkToGearBox { TEXT("GearBox") };

public:
	UPROPERTY(BlueprintReadOnly, Category = VehicleDriver)
	UVehicleBrakeSystemBaseComponent * BrakeSystem = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = VehicleDriver)
	UVehicleEngineBaseComponent * Engine = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = VehicleDriver)
	UVehicleSteeringRackBaseComponent * SteeringRack = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = VehicleDriver)
	UVehicleHandBrakeBaseComponent * HandBrake = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = VehicleDriver)
	UVehicleGearBoxBaseComponent * GearBox = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = VehicleDriver)
	FTransform IMURelativePosition;

	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = VehicleDriver)
	//float EngineToWheelsRatio = 1.f;

	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = VehicleDriver)
	//float VehicleWheelRadius = 35.0f;

protected:
	virtual void RuntimePostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent) override;
	virtual bool OnActivateVehicleComponent() override;
	virtual void OnDeactivateVehicleComponent() override;
	//virtual bool IsVehicleComponentInitializing() const override;
	virtual void DrawDebug(UCanvas *Canvas, float &YL, float &YPos) override;
	virtual FString GetRemark() const override;
	virtual void PrePhysicSimulation(float DeltaTime, const FPhysBodyKinematic& VehicleKinematic, const TTimestamp& Timestamp) override;
	//virtual void PostPhysicSimulationDeferred(float DeltaTime, const FPhysBodyKinematic& VehicleKinematic, const TTimestamp& Timestamp) override;

public:
	virtual EGearState GetGearState() const override { return GearState; }
	virtual bool IsADPing() const override { return bVapiPing; }
	virtual ESodaVehicleDriveMode GetDriveMode() const override;

	//virtual bool IsEnabledLeftTurningLights() const { return false; }
	//virtual bool IsEnabledRightTurningLights() const { return false; }
	//virtual bool IsEnabledBrakeLight() const;
	//virtual bool IsEnabledHeadlights() const { return false; }
	//virtual bool IsEnabledDaytimeRunningLights() const { return false; }
	//virtual bool IsEnabledReversLights() const;
	//virtual bool IsEnabledHorn() const { return false; }
	//virtual FColor GetLED() const;

protected:
	virtual void Serialize(FArchive& Ar) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
#if WITH_EDITOR
	virtual void PostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent) override;
	virtual void PostInitProperties() override;
#endif

protected:
	bool bVapiPing = false;
	bool bADModeEnbaled = false;
	bool bSafeStopEnbaled = false;
	EGearState GearState = EGearState::Neutral;
	int GearNum = 0;

	float WheelRadius;
	bool bWheelRadiusValid = false;

	//FGenericPublisherHelper<UGenericVehicleDriverComponentComponent, UGenericWheeledVehiclePublisher> PublisherHelper { this, &UGenericVehicleDriverComponentComponent::PublisherClass, &UGenericVehicleDriverComponentComponent::Publisher };
	FGenericListenerHelper<UGenericVehicleDriverComponentComponent, UGenericWheeledVehicleControlListener> ListenerHelper { this, &UGenericVehicleDriverComponentComponent::VehicleControlClass, &UGenericVehicleDriverComponentComponent::VehicleControl, "VehicleControlClass", "VehicleControl", "VehicleControlRecord" };
};
