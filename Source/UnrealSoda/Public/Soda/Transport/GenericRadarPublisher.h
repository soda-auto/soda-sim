// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "SodaSimProto/Radar.hpp"
#include "Runtime/Sockets/Public/IPAddress.h"
#include "SocketSubsystem.h"
#include "Sockets.h"
#include "PublisherCommon.h"
#include "GenericRadarPublisher.generated.h"

/***********************************************************************************************
    FGenericRadarPublisher
***********************************************************************************************/
USTRUCT(BlueprintType)
struct UNREALSODA_API FGenericRadarPublisher
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Publisher, SaveGame, meta = (EditInRuntime, ReactivateComponent))
	int Port = 8001;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Publisher, SaveGame, meta = (EditInRuntime, ReactivateComponent))
	FString Address = "239.0.0.230";

	void Advertise();
	void Shutdown();
	void Publish(const soda::RadarScan & Scan, bool bAsync = false);
	bool IsAdvertised() { return !!Socket; }

protected:
	TSharedPtr< FSocket > Socket;
	TSharedPtr< FInternetAddr > Addr;
	TSharedPtr <soda::FUDPFrontBackAsyncTask> AsyncTask;
};


