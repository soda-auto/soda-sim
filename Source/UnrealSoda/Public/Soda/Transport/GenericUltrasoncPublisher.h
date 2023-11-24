// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "SodaSimProto/Ultrasonic.hpp"
#include "Runtime/Sockets/Public/IPAddress.h"
#include "SocketSubsystem.h"
#include "Sockets.h"
#include "PublisherCommon.h"
#include "GenericUltrasoncPublisher.generated.h"


/***********************************************************************************************
	FGenericUltrasoncPublisher
***********************************************************************************************/
USTRUCT(BlueprintType)
struct UNREALSODA_API FGenericUltrasoncPublisher
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Publisher, SaveGame, meta = (EditInRuntime, ReactivateComponent))
	int Port = 8500;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Publisher, SaveGame, meta = (EditInRuntime, ReactivateComponent))
	bool bIsBroadcast = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Publisher, SaveGame, meta = (EditInRuntime, ReactivateComponent))
	FString Address = "239.0.0.230";

	void Advertise();
	void Shutdown();
	void Publish(const soda::UltrasonicsHub& Scan, bool bAsync = true);
	bool IsAdvertised() { return !!Socket; }

protected:
	TSharedPtr< FSocket > Socket;
	TSharedPtr< FInternetAddr > Addr;
	TSharedPtr <soda::FUDPFrontBackAsyncTask> AsyncTask;
};


