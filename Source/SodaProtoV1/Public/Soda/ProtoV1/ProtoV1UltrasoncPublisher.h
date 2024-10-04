// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "Soda/GenericPublishers/GenericUltrasoncPublisher.h"
#include "soda/sim/proto-v1/ultrasonic.hpp"
#include "Soda/Misc/UDPAsyncTask.h"
#include "Runtime/Sockets/Public/IPAddress.h"
#include "SocketSubsystem.h"
#include "Sockets.h"
#include "ProtoV1UltrasoncPublisher.generated.h"


/**
* UProtoV1UltrasoncHubPublisher
*/
UCLASS(ClassGroup = Soda, BlueprintType)
class SODAPROTOV1_API UProtoV1UltrasoncHubPublisher: public UGenericUltrasoncHubPublisher
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Publisher, SaveGame, meta = (EditInRuntime, ReactivateComponent))
	int Port = 8500;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Publisher, SaveGame, meta = (EditInRuntime, ReactivateComponent))
	FString Address = "127.0.0.1";

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Publisher, SaveGame, meta = (EditInRuntime, ReactivateComponent))
	bool bIsBroadcast = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Publisher, SaveGame, meta = (EditInRuntime))
	bool bAsync = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Publisher, SaveGame, meta = (EditInRuntime))
	int DeviceID = 0;

public:
	virtual bool Advertise(UVehicleBaseComponent* Parent) override;
	virtual void Shutdown() override;
	virtual bool IsOk() const override { return !!Socket; }
	virtual bool Publish(float DeltaTime, const FSensorDataHeader& Header, const TArray < FUltrasonicEchos >& EchoCollections) override;
	virtual FString GetRemark() const override;

protected:
	bool Publish(const soda::sim::proto_v1::UltrasonicsHub& Scan);

protected:
	TSharedPtr< FSocket > Socket;
	TSharedPtr< FInternetAddr > Addr;
	TSharedPtr <soda::FUDPFrontBackAsyncTask> AsyncTask;
	soda::sim::proto_v1::UltrasonicsHub Msg;
};


