// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "SodaSimProto/VehicleState.hpp"
#include "Soda/Misc/BitStream.hpp"
#include "Runtime/Sockets/Public/IPAddress.h"
#include "SocketSubsystem.h"
#include "Sockets.h"
#include "PublisherCommon.h"
#include "GenericVehicleStatePublisher.generated.h"


/***********************************************************************************************
    FGenericLidarPublisher
***********************************************************************************************/
USTRUCT(BlueprintType)
struct UNREALSODA_API FGenericVehicleStatePublisher
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Publisher, SaveGame, meta = (EditInRuntime, ReactivateComponent))
	int Port = 7078;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Publisher, SaveGame, meta = (EditInRuntime, ReactivateComponent))
	bool bIsBroadcast = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Publisher, SaveGame, meta = (EditInRuntime, ReactivateComponent))
	FString Address = "127.0.0.1";

	void Advertise();
	void Shutdown();
	void Publish(const soda::GenericVehicleState& Msg, bool bAsync = true);
	bool IsAdvertised() { return !!Socket; }

protected:
	TSharedPtr< FSocket > Socket;
	TSharedPtr< FInternetAddr > Addr;
	int chan_index = 0;
	TSharedPtr <soda::FUDPFrontBackAsyncTask> AsyncTask;
	FBitStream BitStream;
};


