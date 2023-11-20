// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "GameFramework/PawnMovementComponent.h"
#include "Soda/VehicleComponents/WheeledVehicleMovementBaseComponent.h"
#include "Curves/CurveFloat.h"
#include "Soda/Misc/BitStream.hpp"
#include <thread>
#include <mutex>
#include "Sockets.h"
#include "SocketSubsystem.h"
#include "Common/UdpSocketReceiver.h"
#include "Common/UdpSocketBuilder.h"
#include "Interfaces/IPv4/IPv4Address.h"
#include "Interfaces/IPv4/IPv4Endpoint.h"
#include "SodaSimProto/OXTS.hpp"
#include "NCOMWheeledVehicleMovement.generated.h"

class ALevelState;

/**
 * The VIL (Vehicle in the Loop) mode.
 */
UCLASS(ClassGroup = Soda,BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class UNREALSODA_API UNCOMWheeledVehicleMovement : public UWheeledVehicleMovementBaseComponent
{
	GENERATED_UCLASS_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = NCOM, SaveGame, meta = (EditInRuntime, ReactivateComponent))
	int NCOMPort = 8000;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = NCOM, SaveGame, meta = (EditInRuntime, ReactivateComponent))
	FVector NCOMOffest;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = NCOM, SaveGame, meta = (EditInRuntime, ReactivateComponent))
	FRotator NCOMRotation;

	/** Vehicle mass in kg */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ModelSetup, SaveGame, meta = (EditInRuntime, ReactivateComponent))
	float Mass = 1196.0;

	/** Wheels radius in cm */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ModelSetup, SaveGame, meta = (EditInRuntime, ReactivateComponent))
	float FrontWheelRadius = 35;

	/** Wheels radius in cm */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ModelSetup, SaveGame, meta = (EditInRuntime, ReactivateComponent))
	float RearWheelRadius = 35;

	/** Center of Mass */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ModelSetup, SaveGame, meta = (EditInRuntime, ReactivateComponent))
	FVector CenterOfMass;

	UPROPERTY()
	ALevelState * LevelState = nullptr;

public:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void InitializeComponent() override;
	virtual void UninitializeComponent() override;

protected:
	/* Overrides from  ISodaVehicleComponent */
	virtual bool OnActivateVehicleComponent() override;
	virtual void OnDeactivateVehicleComponent() override;
	virtual void OnPreActivateVehicleComponent() override;
	virtual void DrawDebug(UCanvas* Canvas, float& YL, float& YPos) override;

public:
	/* Overrides from  IWheeledVehicleMovementInterface  */
	virtual float GetVehicleMass() const override { return Mass; }
	virtual const FVehicleSimData& GetSimData() const { return VehicleSimData; }
	virtual bool SetVehiclePosition(const FVector& NewLocation, const FRotator& NewRotation);

public:
	UFUNCTION(Category = NCOM, meta = (CallInRuntime))
	void ResetPosition();

protected:
	FVehicleSimData VehicleSimData;
	FTransform InitTransform;
	std::mutex Mutex;

	soda::OxtsPacket OXTSPacket;
	FBitStream BitStream;
	FSocket* ListenSocket = nullptr;
	FUdpSocketReceiver* UDPReceiver = nullptr;
	TTimestamp LastPacketTimestamp;
	float LastDeltatime = 0.01;
	bool bIsConnected = false;
	bool bIsOXTSDataValid = false;
	void Recv(const FArrayReaderPtr& ArrayReaderPtr, const FIPv4Endpoint& EndPt);

	int OXTS_SatellitesNumber = 0;
	int OXTS_PositionMode = 0;
	int OXTS_VelocityMode = 0;
	int OXTS_OrientationMode = 0;
	int OXTS_GPSMinutes = 0;
	int OXTS_Miliseconds = 0;
};