// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "Soda/GenericPublishers/GenericRacingPublisher.h"
#include "soda/sim/proto-v1/racing.hpp"
#include "Soda/Misc/UDPAsyncTask.h"
#include "ProtoV1RacingPublisher.generated.h"


/**
* UProtoV1RacingPublisher
*/
UCLASS(ClassGroup = Soda, BlueprintType)
class SODAPROTOV1_API UProtoV1RacingPublisher : public UGenericRacingPublisher
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Publisher, SaveGame, meta = (EditInRuntime, ReactivateComponent))
	int Port = 5010;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Publisher, SaveGame, meta = (EditInRuntime, ReactivateComponent))
	FString Address = "239.0.0.230";

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Publisher, SaveGame, meta = (EditInRuntime, ReactivateComponent))
	bool bIsBroadcast = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Publisher, SaveGame, meta = (EditInRuntime))
	bool bAsync = true;

public:
	virtual bool Advertise(UVehicleBaseComponent* Parent) override;
	virtual void Shutdown() override;
	virtual bool IsOk() const override { return !!Socket; }
	virtual FString GetRemark() const override;
	virtual bool Publish(float DeltaTime, const FSensorDataHeader& Header, const soda::FRacingSensorData& SensorData) override;

protected:
	bool Publish(const soda::sim::proto_v1::RacingSensor& Msg);

protected:
	TSharedPtr< FSocket > Socket;
	TSharedPtr< FInternetAddr > Addr;
	TSharedPtr <soda::FUDPFrontBackAsyncTask> AsyncTask;
};


