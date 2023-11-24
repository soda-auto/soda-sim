// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "HAL/UnrealMemory.h"
#include "Soda/Misc/Time.h"
#include <stdint.h>
#include <stdbool.h>
#include <float.h>
#include <chrono>

#define INT_DEFAULT (1000U)
#define CANID_EXTENDED_FLAG ((uint32_t)0x80000000U)
#define CANID_MASK          ((uint32_t)0x7fffffffU)
#define CAN_DOUBLE_SIGNAL_LENGTH (9)

namespace dbc
{

class FMessageSerializator;
class FCANMessage;

enum ECanFrameFlags : uint8
{
	CF_STANDARD = 0x00U,  // Frame is a CAN Standard Frame (11-bit identifier)
	CF_RTR      = 0x01U,  // Frame is a CAN Remote-Transfer-Request Frame
	CF_EXTENDED = 0x02U,  // Frame is a CAN Extended Frame (29-bit identifier)
	CF_FD       = 0x04U,  // Frame represents s FD frame in terms of CiA Specs
	CF_BRS      = 0x08U,  // Frame represents a FD bit rate switch (CAN data at a higher bit rate)
	CF_ESI      = 0x10U,  // Frame represents a FD error state indicator(CAN FD transmitter was error active)
	CF_ERRFRAME = 0x40U  // Frame represents an error frame
};

struct UNREALSODA_API FCanFrame
{
	uint32_t ID;
	uint8_t Length;
	uint8 Flags; // ECanFrameFlags
	uint8_t Data[64];

	FCanFrame() {}
	FCanFrame(uint32_t InID, uint8_t InLength, const uint8_t * InData) :
		ID(InID),
		Length(InLength)
	{
		check(InLength <= 64);
		FMemory::Memcpy(Data, InData, InLength);
	}
};

DECLARE_MULTICAST_DELEGATE_ThreeParams( FCanMsgDelegate, TTimestamp, FCANMessage*, uint32);
DECLARE_DELEGATE_OneParam(FCanDelegate, FCanFrame &);

class UNREALSODA_API FCANMessage
{
public:
	FCANMessage(int64 CANID) 
		: RegistredCANID(CANID)
	{}
	virtual ~FCANMessage() {}

	virtual const FString& GetName() const = 0;
	virtual const FString& GetNamespace() const = 0;
	virtual const uint32_t GetPGN() const { return 0; }
	virtual const uint32_t GetID() const = 0;
	virtual const uint8_t GetLength() const = 0; 	/** In bytes */
	virtual const uint32_t GetInterval() const = 0;

	virtual int64 GetRegistredCANID() const { return RegistredCANID; }
	virtual bool IsAlife(const TTimestamp& Timestamp) const { return (Timestamp - RecvTimestamp) < Timeout; }

	virtual void OnPreSend() {}
	virtual void OnAfterRecv() {}

public:
	int64 RegistredCANID;
	FCanFrame Frame;
	TSharedPtr<dbc::FMessageSerializator> Serializer;
	TTimestamp RecvTimestamp;
	TTimestamp SendTimestamp;
	std::chrono::nanoseconds Timeout{ 500000000ll };
	//FCanDelegate OnPreSend;
	//FCanDelegate OnAfterSerialize;
};

class UNREALSODA_API FCANMessageDynamic : public FCANMessage
{
public:
	FCANMessageDynamic(int64 CANID, TSharedPtr<dbc::FMessageSerializator> Serializer);

	virtual const FString& GetName() const;
	virtual const FString& GetNamespace() const;
	//virtual const uint32_t GetPGN() const { return 0; }
	virtual const uint32_t GetID() const;
	virtual const uint8_t GetLength() const; 	/** In bytes */
	virtual const uint32_t GetInterval() const;

	virtual void OnAfterRecv() override;
	virtual void OnPreSend() override;
};

class UNREALSODA_API FCANMessageStatic : public FCANMessage
{
public:
	FCANMessageStatic(int64 CANID)
		: FCANMessage(CANID)
	{}

	virtual bool Dser(uint32_t length, const void* in) = 0;
	virtual bool Ser(uint32_t length, void* out) const = 0;

	virtual void OnAfterRecv() override;
	virtual void OnPreSend() override;
};

} // namespace dbc
