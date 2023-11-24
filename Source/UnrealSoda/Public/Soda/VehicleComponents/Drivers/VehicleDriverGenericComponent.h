// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "Soda/VehicleComponents/VehicleDriverComponent.h"
#include "Soda/Transport/GenericVehicleStatePublisher.h"
#include "Sockets.h"
#include "SocketSubsystem.h"
#include "Common/UdpSocketReceiver.h"
#include "Common/UdpSocketBuilder.h"
#include "Interfaces/IPv4/IPv4Address.h"
#include "Interfaces/IPv4/IPv4Endpoint.h"
#include "VehicleDriverGenericComponent.generated.h"

class UVehicleBrakeSystemBaseComponent;
class UVehicleEngineBaseComponent;
class UVehicleSteeringRackBaseComponent;
class UVehicleHandBrakeBaseComponent;
class UVehicleGearBoxBaseComponent;

UCLASS(ClassGroup = Soda, BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class UNREALSODA_API UVehicleDriverGenericComponent : public UVehicleDriverComponent
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Link, SaveGame, meta = (EditInRuntime, ReactivateComponent, AllowedClasses = "/Script/SodaSim.VehicleEngineBaseComponent"))
	FSubobjectReference LinkToEngine { TEXT("Engine") };

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Link, SaveGame, meta = (EditInRuntime, ReactivateComponent, AllowedClasses = "/Script/SodaSim.VehicleSteeringRackBaseComponent"))
	FSubobjectReference LinkToSteering { TEXT("SteeringRack") };

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Link, SaveGame, meta = (EditInRuntime, ReactivateComponent, AllowedClasses = "/Script/SodaSim.VehicleBrakeSystemBaseComponent"))
	FSubobjectReference LinkToBrakeSystem { TEXT("BrakeSystem") };

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Link, SaveGame, meta = (EditInRuntime, ReactivateComponent, AllowedClasses = "/Script/SodaSim.VehicleHandBrakeBaseComponent"))
	FSubobjectReference LinkToHandBrake { TEXT("HandBrake") };

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Link, SaveGame, meta = (EditInRuntime, ReactivateComponent, AllowedClasses = "/Script/SodaSim.VehicleGearBoxBaseComponent"))
	FSubobjectReference LinkToGearBox { TEXT("GearBox") };

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = VehicleDriver, SaveGame, meta = (EditInRuntime, ReactivateComponent))
	int RecvPort = 7077;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = VehicleDriver, SaveGame, meta = (EditInRuntime))
	FGenericVehicleStatePublisher Publisher;

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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = VehicleDriver)
	float EngineToWheelsRatio = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = VehicleDriver)
	float VehicleWheelRadius = 35.0f;

protected:
	virtual bool OnActivateVehicleComponent() override;
	virtual void OnDeactivateVehicleComponent() override;
	virtual void DrawDebug(UCanvas *Canvas, float &YL, float &YPos) override;
	virtual void PrePhysicSimulation(float DeltaTime, const FPhysBodyKinematic& VehicleKinematic, const TTimestamp& Timestamp) override;
	virtual void PostPhysicSimulationDeferred(float DeltaTime, const FPhysBodyKinematic& VehicleKinematic, const TTimestamp& Timestamp) override;

public:
	virtual ENGear GetGear() const override { return Gear; }
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
	void Shutdown();
	void Recv(const FArrayReaderPtr &ArrayReaderPtr, const FIPv4Endpoint &EndPt);

protected:
	FSocket *ListenSocket = nullptr;
	FUdpSocketReceiver *UDPReceiver = nullptr;

	FSocket *SendSocket = nullptr;
	TSharedPtr<FInternetAddr> SendSocketAddr = nullptr;

	soda::GenericVehicleControl MsgIn;

	bool bVapiPing = false;
	bool bIsADMode = false;
	ENGear Gear = ENGear::Park;
	TTimestamp RecvTimestamp;
};
