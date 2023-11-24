// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "SodaSimProto/OXTS.hpp"
#include "Soda/Misc/BitStream.hpp"
#include "Runtime/Sockets/Public/IPAddress.h"
#include "SocketSubsystem.h"
#include "Sockets.h"
#include "PublisherCommon.h"
#include "OXTSPublisher.generated.h"


/***********************************************************************************************
    FGenericLidarPublisher
***********************************************************************************************/
USTRUCT(BlueprintType)
struct UNREALSODA_API FOXTSPublisher
{
	GENERATED_BODY()

	FOXTSPublisher();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Publisher, SaveGame, meta = (EditInRuntime, ReactivateComponent))
	int Port = 8000;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Publisher, SaveGame, meta = (EditInRuntime, ReactivateComponent))
	bool bIsBroadcast = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Publisher, SaveGame, meta = (EditInRuntime, ReactivateComponent))
	FString Address = "127.0.0.1";

	void Advertise();
	void Shutdown();
	void Publish(const soda::OxtsPacket & Msg, bool bAsync = true);
	bool IsAdvertised() { return !!Socket; }
	int GetChanID() { return chan_index; }

	static std::uint8_t Checksum(soda::OxtsPacket const& Packet, std::size_t Start, std::size_t End) noexcept;

protected:
	TSharedPtr< FSocket > Socket;
	TSharedPtr< FInternetAddr > Addr;
	int chan_index = 0;
	TSharedPtr <soda::FUDPFrontBackAsyncTask> AsyncTask;
	FBitStream BitStream;
};


