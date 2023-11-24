// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/Transport/XCP.h"
#include "Soda/UnrealSoda.h"

namespace xcp
{

enum SlaveToMasterPID: uint8
{
	SERV = 0xFC,
	EV = 0xFD,
	ERR = 0xFE,
    RES = 0xFF,
};

enum MasterToSlavePID : uint8
{
    //Standard Commands:
    CONNECT = 0xFF,
    DISCONNECT = 0xFE,
    GET_STATUS = 0xFD,
    SYNCH = 0xFC,
    GET_COMM_MODE_INFO = 0xFB,
    GET_ID = 0xFA,
    SET_REQUEST = 0xF9,
    GET_SEED = 0xF8,
    UNLOCK = 0xF7,
    SET_MTA = 0xF6,
    UPLOAD = 0xF5,
    SHORT_UPLOAD = 0xF4,
    BUILD_CHECKSUM = 0xF3,
    TRANSPORT_LAYER_CMD = 0xF2,
    USER_CMD = 0xF1,

    //Calibration commands:
	DOWNLOAD = 0xF0,
	DOWNLOAD_NEXT = 0xEF,
	DOWNLOAD_MAX = 0xEE,
	SHORT_DOWNLOAD = 0xED,
	MODIFY_BITS = 0xEC,

    //Page switching commands:

    //Basic data acquisition and stimulation commands:
    SET_DAQ_PTR = 0xE2,
    WRITE_DAQ = 0xE1,
    SET_DAQ_LIST_MODE = 0xE0,
    START_STOP_DAQ_LIST = 0xDE,
    START_STOP_SYNCH = 0xDD,
    WRITE_DAQ_MULTIPLE = 0xC7,
    READ_DAQ = 0xDB,
    GET_DAQ_CLOCK = 0xDC,
    GET_DAQ_PROCESSOR_INFO = 0xDA,
    GET_DAQ_RESOLUTION_INFO = 0xD9,
    GET_DAQ_LIST_MODE = 0xDF,
    GET_DAQ_EVENT_INFO = 0xD7,
    DTO_CTR_PROPERTIES = 0xC5,

    //Static data acquisition and stim commands:
    CLEAR_DAQ_LIST = 0xE3,
    GET_DAQ_LIST_INFO = 0xD8,

    //Dynamic data acquisition and stim commands:
    FREE_DAQ = 0xD6,
    ALLOC_DAQ = 0xD5,
    ALLOC_ODT = 0xD4,
    ALLOC_ODT_ENTRY = 0xD3,

    //Non-volatile memory programming commands:

    //Time sync commands:
};

FPacket::FPacket(uint8 CMD)
{
	Data[0] = CMD;
	Length = 1;
}

FPacket::FPacket(uint8 CMD, const uint8* InData, uint8 InLength)
{
	check(InLength <= 7);
	Data[0] = CMD;
	FMemory::Memcpy(&Data[1], InData, InLength);
	Length = InLength + 1;
}

FPacket::FPacket(const uint8* InData, uint8 InLength)
{
	check(InLength <= 8);
	*(uint64*)Data = *(uint64*)InData;
	Length = InLength;
}

FString FPacket::ToString() const
{
	return  BytesToHex(Data, Length);
}

bool FBaseTransport::Request(const FPacket& SendPacket, FPacket & RecvPacket)
{
	if (!Send(SendPacket))
	{
		return false;
	}
	if (!ResQueue.PopBack(RecvPacket, Timeout))
	{
		return false;
	}
	if (RecvPacket.Data[0] == SlaveToMasterPID::ERR && SendPacket.Data[0] != MasterToSlavePID::SYNCH)
	{
		UE_LOG(LogSoda, Error, TEXT("FBaseTransport::Request(); Received error status: [%s]"), *RecvPacket.ToString());
		return false;
	}
	return true;
}

inline bool FBaseTransport::Request(const FPacket& SendPacket)
{
	FPacket RecvPacket;
	return Request(SendPacket, RecvPacket);
}

void FBaseTransport::ProcessResponse(const FPacket& Packet, int Counter, TTimestamp InRecvTimestamp)
{
	if (bIsDebug)
	{
		UE_LOG(LogSoda, Log, TEXT("FBaseTransport::ProcessResponse(); Received [%s]"), *Packet.ToString());
	}

	if (Counter == CounterReceived)
	{
		UE_LOG(LogSoda, Warning, TEXT("FBaseTransport::ProcessResponse(); Duplicate message counter %i"), Counter);
		return;
	}

	CounterReceived = Counter;
	uint8 PID = Packet.Data[0];
	if (PID == SlaveToMasterPID::ERR || PID == SlaveToMasterPID::RES)
	{
		ResQueue.Push(Packet);
		RecvTimestamp = InRecvTimestamp;
	}
}

FCANTransport::FCANTransport(UCANBusComponent* InCANDev)
{
	check(::IsValid(InCANDev));
	CANBus = InCANDev;
	CANDevObj = InCANDev;
	CANBus->RecvDelegate.AddRaw(this , &FCANTransport::DataReceived);
}

bool FCANTransport::Send(const FPacket& Packet)
{
	return CANBus->SendFrame(dbc::FCanFrame(CAN_ID_MASTER, Packet.Length, Packet.Data)) >= 0;
}

void FCANTransport::DataReceived(TTimestamp Timestamp, const dbc::FCanFrame& CanFrame)
{
	if ((CanFrame.ID & 0x3FFFFFF) == (CAN_ID_SLAVE & 0x3FFFFFF))
	{
		ProcessResponse(FPacket(CanFrame.Data, CanFrame.Length), CounterReceived + 1, Timestamp);
	}
}

FMaster::FMaster(const TSharedPtr<FBaseTransport>& InTransport)
{
	check(InTransport);
	Transport = InTransport;
}

FMaster::~FMaster()
{
	if(Transport->IsValid()) Disconnect();
}

bool FMaster::Connect(uint8 mode)
{
	bIsConnected = false;


	if (!Transport->IsValid())
	{
		return false;
	}

	FPacket Response;
	if (!Transport->Request(FPacket(MasterToSlavePID::CONNECT, &mode, 1), Response))
	{
		return false;
	}

	if (Response.Length < 8)
	{
		return false;
	}

	ConnectionInfo.Resource = Response.Data[0];
	ConnectionInfo.CommModeBasic = Response.Data[1];
	ConnectionInfo.MaxCto = Response.Data[2];
	ConnectionInfo.MaxDto = *(uint16*)&Response.Data[3];
	ConnectionInfo.ProtocolLayerVersion = Response.Data[5];
	ConnectionInfo.TransportLayerVersion = Response.Data[6];

	bIsLittleEndian = ((ConnectionInfo.CommModeBasic & (uint8)EConnCommModeBasicBits::BYTE_ORDER_) == 0);
	ConnectionInfo.MaxDto = WordSwap(ConnectionInfo.MaxDto);

	UE_LOG(LogSoda, Log, TEXT("FMaster::Connect(), Resource %i, CommModeBasic %i, MaxCto %i, MaxDto %i, ProtocolLayerVersion %i, TransportLayerVersion %i"),
		ConnectionInfo.Resource,
		ConnectionInfo.CommModeBasic,
		ConnectionInfo.MaxCto,
		ConnectionInfo.MaxDto,
		ConnectionInfo.ProtocolLayerVersion,
		ConnectionInfo.TransportLayerVersion
	);

	bIsConnected = true;

	return true;
}

bool FMaster::Disconnect()
{
	bIsConnected = false;
	if (!Transport->IsValid())
	{
		return false;
	}

	if (!bIsConnected)
	{
		return true;
	}
	return Transport->Request(FPacket(MasterToSlavePID::DISCONNECT));;
}

bool FMaster::GetStatus(FConnectionStatus& ConnectionStatus)
{
	FPacket Response;
	if (Transport->Request(FPacket(MasterToSlavePID::GET_STATUS), Response))
	{
		return false;
	}

	if (Response.Length < 6)
	{
		return false;
	}

	ConnectionStatus.GetCurrentSessionStatus = Response.Data[0];
	ConnectionStatus.GetCurrentResourceProtection = Response.Data[1];
	ConnectionStatus.GetStateNumber = Response.Data[2];
	ConnectionStatus.GetSessionConfigurationId = WordSwap(*(uint16*)&Response.Data[3]);

	return true;
}

bool FMaster::Synch()
{
	return Transport->Request(FPacket(MasterToSlavePID::SYNCH));
}

bool FMaster::SetMta(uint32 Address, uint8 AddressExt)
{
	FPacket Packet;
	Packet.Length = 8;
	Packet.Data[0] = MasterToSlavePID::SET_MTA;
	Packet.Data[1] = 0;
	Packet.Data[2] = 0;
	Packet.Data[3] = AddressExt;
	*(uint32*)&Packet.Data[4] = DwordSwap(Address);
	return Transport->Request(Packet);
}

bool FMaster::Download(const uint8* Data, uint8 Length)
{
	check(Length <= 6);
	FPacket Packet;
	Packet.Length = Length + 2;
	Packet.Data[0] = MasterToSlavePID::DOWNLOAD;
	Packet.Data[1] = Length;
	FMemory::Memcpy(&Packet.Data[2], Data, Length);
	return Transport->Request(Packet);
}

bool FMaster::Upload(uint8* Data, uint8 Length)
{
	FPacket Response;
	if (!Transport->Request(FPacket(MasterToSlavePID::UPLOAD, &Length, 1), Response))
	{
		return false;
	}

	if (Response.Length <= 1)
	{
		return false;
	}

	int Num = Response.Length - 1;
	if (Num > Length) Num = Length;
	FMemory::Memcpy(Data , &Response.Data[1], Num);


	// larger sizes will send in multiple CAN messages
	// each valid message will start with 0xFF followed by the upload bytes
	// the last message might be padded to the required DLC
	int Got = Num;
	while (Got < Length)
	{
		if (Transport->ResQueue.PopBack(Response, Transport->Timeout) && Response.Data[0] == SlaveToMasterPID::RES && Response.Length > 1)
		{
			Num = Response.Length - 1;
			if ((Num + Got) > Length) Num = Length - Got;
			FMemory::Memcpy(&Data[Got], &Response.Data[1], Num);
			Got += Num;
		}
		else
		{
			return false;
		}	
	}
	return true;
}

inline uint16 FMaster::WordSwap(uint16 In) const
{
	return bIsLittleEndian ? In : Swap16(In);
}

inline uint32 FMaster::DwordSwap(uint32 In) const
{
	return bIsLittleEndian ? In : Swap32(In);
}

} // xcp