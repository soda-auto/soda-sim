// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "Soda/VehicleComponents/CANBus.h"
#include "Soda/Misc/WatingQueue.h"

namespace xcp
{

/**
 * Common XCP packet. 
 * Support only 8 byte data length.
 */
struct UNREALSODA_API FPacket
{
	FPacket() {}
	FPacket(uint8 CMD);
	FPacket(uint8 CMD, const uint8* InData, uint8 InLength);
	FPacket(const uint8* InData, uint8 InLength);
	FString ToString() const;

	uint8_t Length = 0;
	uint8_t Data[8];
};

/**
 * EConnResourceParameterBits
 */
enum class EConnResourceParameterBits : uint8
{
	CAL_PG = 0x1,	//Calibration and Paging: 0 = not available , 1 = avilable
	DAQ = 0x4,	//DAQ list supported: 0 = not avaliable, 1 = available
	STIM = 0x8,	//STIM - Data stimulation of a daq list: 0 = not available, 1 = available
	PGM = 0x10,	//Programming: 0 = flash programming not available, 1 = available
};

/**
 * EConnCommModeBasicBits
 */
enum class EConnCommModeBasicBits : uint8
{
	BYTE_ORDER_ = 0x1,	//Byte order for multibyte parameters. 0 = Little endian (Intel format), 1 = Big Endian (Motorola format)
	ADDRESS_GRANULARITY_0 = 0x2,	//The address granularity indicates the size of an element 
	ADDRESS_GRANULARITY_1 = 0x4,	//The address granularity indicates the size of an element: 00-byte, 01-word, 10-DWORD, 11-reserved
	ADDRESS_GRANULARITY_BOTH = 0x6,
	SLAVE_BLOCK_MODE = 0x40, //Inidicates if slave block mode is available
	OPTIONAL_ = 0x80, //The OPTIONAL flag indicates whether additional information on supported types of 	Communication mode is available.The master can get that additional information with GET_COMM_MODE_INFO.
};

/**
 * ECurrentSessionStatusBits
 */
enum class ECurrentSessionStatusBits : uint8
{
	STORE_CAL_REQ = 0x01, //Pending request to store data into non-volatile memory
	STORE_DAQ_REQ = 0x04, //Pending request to save daq list into non-volatile memory
	CLEAR_DAQ_REQ = 0x08, //pending request to clear all daq list in non-volatile memory
	DAQ_RUNNING = 0x40, //at least one daq list has been started and is in running mode
	RESUME = 0x80, //slave is in resume mode
};

/**
 * ECurrentResourceProtectionBits
 * The given resorce is protected with Seed&key. If a resource is protected, an attempt to exectue a command on it 
 * before a successful GET_SEED/UNLOCK sequence will result in ERR_ACCESS_LOCKED 
 */
enum class ECurrentResourceProtectionBits : uint8
{
	CAL_PG = 0x01,
	DAQ = 0x04,
	STIM = 0x08,
	PGM = 0x10,
};

/**
 * FConnectionInfo
 */
struct FConnectionInfo
{ 
	uint8 Resource; //See EConnResourceParameterBits
	uint8 CommModeBasic; //See EConnCommModeBasicBits
	uint8 MaxCto;
	uint16 MaxDto;
	uint8 ProtocolLayerVersion;
	uint8 TransportLayerVersion;
};

struct FConnectionStatus
{
	uint8 GetCurrentSessionStatus; //See ECurrentSessionStatusBits
	uint8 GetCurrentResourceProtection; //See ECurrentResourceProtectionBits
	uint8 GetStateNumber;
	uint16 GetSessionConfigurationId;
};

/**
 * FBaseTransport
 */
class UNREALSODA_API FBaseTransport
{
public:
	bool bIsDebug = false;
	int Timeout = 500; //[ms]
	TWatingQueue<FPacket> ResQueue;

protected:
	TTimestamp RecvTimestamp;
	//int CounterSend = 0;
	int CounterReceived = -1;

public:
	FBaseTransport()
	{
	}

	virtual ~FBaseTransport()
	{
	}

	virtual bool Send(const FPacket& Packet) = 0;
	virtual bool Request(const FPacket& SendPacket, FPacket & RecvPacket);
	virtual bool Request(const FPacket& SendPacket);
	virtual bool IsValid() const = 0;

protected:
	virtual void ProcessResponse(const FPacket& Packet, int Counter, TTimestamp InRecvTimestamp);
};

/**
 * FCANTransport
 */
class UNREALSODA_API FCANTransport: public FBaseTransport
{
public:
	TWeakObjectPtr<UCANBusComponent> CANDevObj;
	UCANBusComponent* CANBus = nullptr;
	uint32_t CAN_ID_MASTER = 0x9950F1FD;
	uint32_t CAN_ID_SLAVE = 0x9951FDF1;
	bool bIsJ1939 = true;

public:
	FCANTransport(UCANBusComponent* InCANDev);
	virtual bool Send(const FPacket& Packet) override;
	virtual bool IsValid() const override { return ::IsValid(CANDevObj.Get()); }

protected:
	void DataReceived(TTimestamp Timestamp, const dbc::FCanFrame& CanFrame);
};

/**
 * FMaster
 */
class UNREALSODA_API FMaster
{
public:
	TSharedPtr<FBaseTransport> Transport = nullptr;

public:
	FMaster(const TSharedPtr<FBaseTransport>& InTransport);
	virtual ~FMaster();

	/**
	 * Build up connection to an XCP slave.
	 * Before the actual XCP traffic starts a connection is required.
	 * Every XCP slave supports at most one connection,
	 * more attempts to connect are silently ignored.
	 */
	bool Connect(uint8 mode = 0x00);

	/**
	 * Releases the connection to the XCP slave.
	 * Thereafter, no further communication with the slave is possible
	 * If DISCONNECT is currently not possible, ERR_CMD_BUSY will be returned.
	 */
	bool Disconnect();

	/** Synchronize command execution after timeout conditions. */
	bool Synch();

	/**
	 * Set Memory Transfer Address in slave.
	 * The MTA is used by : meth:`buildChecksum`, : meth : `upload`, : meth : `download`, : meth : `downloadNext`,
	 * : meth : `downloadMax`, : meth : `modifyBits`, : meth : `programClear`, : meth : `program`, : meth : `programNext`
	 * and :meth:`programMax`.
	 */
	bool SetMta(uint32 Address, uint8 AddressExt = 0x00);

	/** 
	 * Transfer data from master to slave.
	 * Adress is set via : meth:`setMta`
	 */
	bool Download(const uint8* Data, uint8 Length);


	/**
	 * Transfer data from slave to master.
	 * Adress is set via : meth:`setMta` (Some services like : meth:`getID` also set the MTA).
	 */
	bool Upload(uint8* Data, uint8 Length);

	/**
	 * Get current status information of the slave device.
	 * This includes the status of the resource protection, pending store
	 * requests and the general status of data acquisition and stimulation.
	 */
	bool GetStatus(FConnectionStatus & ConnectionStatus);

	/**
	 * Get FConnectionInfo structure. Valid if only connected.
	 */
	const FConnectionInfo& GetConnectionInfo() const { return ConnectionInfo; }

protected:
	bool bIsLittleEndian = true;
	bool bIsConnected = false;
	FConnectionInfo ConnectionInfo;
	uint16 WordSwap(uint16 In) const;
	uint32 DwordSwap(uint32 In) const;
};

inline uint16 Swap16(uint16 Val)
{
	return (Val >> 8) | (Val << 8);
}

inline uint32 Swap32(uint32 Val) {
	Val = ((Val << 8) & 0xFF00FF00) | ((Val >> 8) & 0xFF00FF);
	return (Val << 16) | (Val >> 16);
}

inline uint64 Swap64(uint64 Val) {
	Val = ((Val & 0x00000000FFFFFFFFull) << 32) | ((Val & 0xFFFFFFFF00000000ull) >> 32);
	Val = ((Val & 0x0000FFFF0000FFFFull) << 16) | ((Val & 0xFFFF0000FFFF0000ull) >> 16);
	Val = ((Val & 0x00FF00FF00FF00FFull) << 8) | ((Val & 0xFF00FF00FF00FF00ull) >> 8);
	return Val;
}

} //xcp