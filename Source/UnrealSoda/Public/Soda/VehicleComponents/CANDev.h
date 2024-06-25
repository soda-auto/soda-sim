// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "Soda/VehicleComponents/VehicleBaseComponent.h"
#include "Soda/DBC/Common.h"
#include "Soda/SodaTypes.h"
#include <unordered_map>
#include "CANDev.generated.h"

#define CANID_DEFAULT -1

DECLARE_MULTICAST_DELEGATE_TwoParams(FCanDevRecvFrameDelegate, TTimestamp, const dbc::FCanFrame &);

class UCANBusComponent;

/** ECanBitRates */
UENUM(BlueprintType)
enum class ECanBitRates : uint8
{
	CAN_BITRATE_1M,
	CAN_BITRATE_500K,
	CAN_BITRATE_250K,
	CAN_BITRATE_125K,
	CAN_BITRATE_100K,
	CAN_BITRATE_62K,
	CAN_BITRATE_50K,
};

/** ECANMode */
UENUM(BlueprintType)
enum class ECANMode : uint8
{
	Standart,
	FD
};

/**
 * UCANDevComponent
 * Abstract CAN device 
 */
UCLASS(abstract, ClassGroup = Soda, BlueprintType, Blueprintable)
class UNREALSODA_API UCANDevComponent : public UVehicleBaseComponent
{
	GENERATED_UCLASS_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Link, SaveGame, meta = (EditInRuntime, ReactivateActor, AllowedClasses = "/Script/UnrealSoda.CANBusComponent"))
	FSubobjectReference LinkToCANBus;

public:
	virtual bool IsCANFDSupported() const { return false; }
	virtual void ProcessRecvMessage(const TTimestamp& Timestamp, const dbc::FCanFrame& CanFrame);
	virtual int SendFrame(const dbc::FCanFrame& CanFrame);

public:
	virtual void OnPreActivateVehicleComponent() override;
	virtual bool OnActivateVehicleComponent() override;
	virtual void OnDeactivateVehicleComponent() override;

public:
	virtual FString GetRemark() const override;
	virtual void DrawDebug(UCanvas* Canvas, float& YL, float& YPos) override;

protected:
	UPROPERTY();
	UCANBusComponent* CANBus;
};
