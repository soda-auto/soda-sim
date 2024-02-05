// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "Soda/VehicleComponents/GenericPublishers/GenericV2XPublisher.h"
#include "soda/sim/proto-v1/V2X.hpp"
#include "Soda/Misc/UDPAsyncTask.h"
#include "ProtoV1V2XPublisher.generated.h"


/**
* UProtoV1V2XPublisher
*/
UCLASS(ClassGroup = Soda, BlueprintType)
class SODAPROTOV1_API UProtoV1V2XPublisher : public UGenericV2XPublisher
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Publisher, SaveGame, meta = (EditInRuntime, ReactivateComponent))
	int Port = 5000;

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
	virtual bool Publish(float DeltaTime, const FSensorDataHeader& Header, const TArray<UV2XMarkerSensor*>& Transmitters) override;

protected:
	bool Publish(const soda::sim::proto_v1::V2XRenderObject& Msg);

protected:
	TSharedPtr< FSocket > Socket;
	TSharedPtr< FInternetAddr > Addr;
	TSharedPtr <soda::FUDPFrontBackAsyncTask> AsyncTask;
};


