// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "Soda/GenericPublishers/GenericNavPublisher.h"
#include "Soda/VehicleComponents/Sensors/Base/NavSensor.h"
#include "soda/sim/proto-v1/navigation.hpp"
#include "Soda/Misc/UDPAsyncTask.h"
#include "Runtime/Sockets/Public/IPAddress.h"
#include "SocketSubsystem.h"
#include "Sockets.h"
#include "ProtoV1NavPublisher.generated.h"

/**
* UProtoV1NavPublisher
*/
UCLASS(ClassGroup = Soda, BlueprintType)
class SODAPROTOV1_API UProtoV1NavPublisher : public UGenericNavPublisher
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Publisher, SaveGame, meta = (EditInRuntime, ReactivateComponent))
	int Port = 7080;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Publisher, SaveGame, meta = (EditInRuntime, ReactivateComponent))
	FString Address = "127.0.0.1";

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Publisher, SaveGame, meta = (EditInRuntime, ReactivateComponent))
	bool bIsBroadcast = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Publisher, SaveGame, meta = (EditInRuntime))
	bool bAsync = true;

public:
	virtual bool Advertise(UVehicleBaseComponent* InParent) override;
	virtual void Shutdown() override;
	virtual bool IsOk() const override { return !!Socket; }
	//virtual void DrawDebug(UCanvas* Canvas, float& YL, float& YPos) override;
	virtual bool Publish(float DeltaTime, const FSensorDataHeader& Header, const FTransform& RelativeTransform, const FPhysBodyKinematic& VehicleKinematic, const FImuNoiseParams& Covariance) override;
	virtual FString GetRemark() const override;

protected:
	bool Publish(const soda::sim::proto_v1::NavigationState& NavState);

protected:
	TSharedPtr< FSocket > Socket;
	TSharedPtr< FInternetAddr > Addr;
	TSharedPtr <soda::FUDPFrontBackAsyncTask> AsyncTask;
	UVehicleBaseComponent* Parent = nullptr;
};


