// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "Soda/GenericPublishers/GenericWheeledVehicleControl.h"
#include "soda/sim/proto-v1/vehicleState.hpp"
#include "Soda/Misc/UDPAsyncTask.h"
#include "Runtime/Sockets/Public/IPAddress.h"
#include "SocketSubsystem.h"
#include "Sockets.h"
#include "Common/UdpSocketReceiver.h"
#include "ProtoV1WheeledVehicleControl.generated.h"


/**
* UProtoV1WheeledVehicleControl
*/
UCLASS(ClassGroup = Soda, BlueprintType)
class SODAPROTOV1_API UProtoV1WheeledVehicleControl : public UGenericWheeledVehicleControlListener
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = VehicleDriver, SaveGame, meta = (EditInRuntime, ReactivateComponent))
	int RecvPort = 7077;

public:
	virtual bool StartListen(UVehicleBaseComponent* Parent) override;
	virtual void StopListen() override;
	virtual bool IsOk() const { return !!ListenSocket; }
	virtual void DrawDebug(UCanvas* Canvas, float& YL, float& YPos) override;
	virtual bool GetControl(soda::FGenericWheeledVehiclControl& Control) const override;
	virtual FString GetRemark() const override;

protected:
	void Recv(const FArrayReaderPtr& ArrayReaderPtr, const FIPv4Endpoint& EndPt);

protected:
	FSocket* ListenSocket = nullptr;
	FUdpSocketReceiver* UDPReceiver = nullptr;
	soda::sim::proto_v1::GenericVehicleControlMode1 Msg;
	TTimestamp RecvTimestamp;
};


