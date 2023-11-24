// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "PublisherCommon.h"
#include "SodaSimProto/V2X.hpp"
#include "GenericV2XPublisher.generated.h"


/***********************************************************************************************
    FGenericV2XPublisher
***********************************************************************************************/
USTRUCT(BlueprintType)
struct UNREALSODA_API FGenericV2XPublisher
{
	GENERATED_BODY()

	FGenericV2XPublisher();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Publisher, SaveGame, meta = (EditInRuntime, ReactivateComponent))
	int Port = 5000;

	void Advertise();
	void Shutdown();
	void Publish(const soda::V2VSimulatedObject& Msg, bool bAsync = true);
	void Publish(const soda::V2XRenderObject& Msg, bool bAsync = true);
	bool IsAdvertised() { return !!Socket; }

protected:
	TSharedPtr< FSocket > Socket;
	TSharedPtr< FInternetAddr > Addr;
	TSharedPtr <soda::FUDPFrontBackAsyncTask> AsyncTask;
};


