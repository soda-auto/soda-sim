// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "Soda/VehicleComponents/VehicleDriverComponent.h"
#include "Soda/GenericPublishers/GenericWheeledVehiclePublisher.h"
#include "Soda/GenericPublishers/GenericWheeledVehicleControl.h"
#include "GenericVehicleDriverComponent.generated.h"

class UVehicleBrakeSystemBaseComponent;
class UVehicleEngineBaseComponent;
class UVehicleSteeringRackBaseComponent;
class UVehicleGearBoxBaseComponent;

namespace soda
{
	struct FGenericWheeledVehiclControl
	{
		union
		{
			float ByAngle; // [rad]
			float ByRatio; // [-1..1]
		} SteerReq;

		/** Zero value means change speed as quickly as possible to TargetSpeed */
		union
		{
			float ByAcc; // [cm/s^2]
			float ByRatio; // [-1..1]
		} DriveEffortReq;

		/** [rad/s] Zero value means change speed as quickly as possible to SteerReq.ByAngle */
		float SteeringAngleVelocity;

		/** [cm/s] */
		float TargetSpeedReq;

		EGearState GearStateReq;
		/**
		 * Desire gear number for the DRIVE and REVERSE gear only;
		 * Values:
		 *   - 0        - automatic/undefined;
		 *   - others
		 */
		int8 GearNumReq;

		enum class ESteerReqMode: uint8
		{
			ByRatio,
			ByAngle
		};

		enum class EDriveEffortReqMode : uint8
		{
			ByRatio,
			ByAcc
		};
		bool bTargetSpeedIsSet;
		bool bGearIsSet;
		bool bSteeringAngleVelocitySet;
		ESteerReqMode SteerReqMode;
		EDriveEffortReqMode DriveEffortReqMode;

		TTimestamp Timestamp;
	};

} // namespace soda

/**
 * TODO: Support brake by Engine
 * TODO: Support revers by Engine, without change gear (for electric engines)
 * TODO: Support FGenericWheeledVehiclControl::bSteeringAngleVelocitySet
 */

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

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Link, SaveGame, meta = (EditInRuntime, ReactivateComponent, AllowedClasses = "/Script/UnrealSoda.VehicleBrakeSystemBaseComponent"))
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
	UVehicleBrakeSystemBaseComponent* HandBrake = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = VehicleDriver)
	UVehicleGearBoxBaseComponent * GearBox = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = VehicleDriver)
	FTransform IMURelativePosition;

	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = VehicleDriver)
	//float EngineToWheelsRatio = 1.f;

	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = VehicleDriver)
	//float VehicleWheelRadius = 35.0f;

	/** [cm/s] */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = VehicleDriver)
	float TargetSpeedDelta = 10;

protected:
	virtual void RuntimePostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent) override;
	virtual bool OnActivateVehicleComponent() override;
	virtual void OnPostActivateVehicleComponent() override;
	virtual void OnDeactivateVehicleComponent() override;
	//virtual bool IsVehicleComponentInitializing() const override;
	virtual void DrawDebug(UCanvas *Canvas, float &YL, float &YPos) override;
	virtual FString GetRemark() const override;
	virtual void PrePhysicSimulation(float DeltaTime, const FPhysBodyKinematic& VehicleKinematic, const TTimestamp& Timestamp) override;
	//virtual void PostPhysicSimulationDeferred(float DeltaTime, const FPhysBodyKinematic& VehicleKinematic, const TTimestamp& Timestamp) override;
	virtual void OnPushDataset(soda::FActorDatasetData& Dataset) const override;

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

	soda::FGenericWheeledVehiclControl Control;

	//FGenericPublisherHelper<UGenericVehicleDriverComponentComponent, UGenericWheeledVehiclePublisher> PublisherHelper { this, &UGenericVehicleDriverComponentComponent::PublisherClass, &UGenericVehicleDriverComponentComponent::Publisher };
	FGenericListenerHelper<UGenericVehicleDriverComponentComponent, UGenericWheeledVehicleControlListener> ListenerHelper { this, &UGenericVehicleDriverComponentComponent::VehicleControlClass, &UGenericVehicleDriverComponentComponent::VehicleControl, "VehicleControlClass", "VehicleControl", "VehicleControlRecord" };
};
