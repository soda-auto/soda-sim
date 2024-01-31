// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/DBC/Common.h"
#include "Soda/DBC/Serialization.h"
#include "Soda/SodaApp.h"
#include "Soda/UnrealSoda.h"

namespace dbc
{

FCANMessageDynamic::FCANMessageDynamic(int64 CANID, TSharedPtr<dbc::FMessageSerializator> InSerializer)
	: FCANMessage(CANID)
{
	Serializer = InSerializer;
	check(Serializer);
}

const FString& FCANMessageDynamic::GetName() const
{
	return Serializer->GetName();
}

const FString& FCANMessageDynamic::GetNamespace() const
{
	static FString Dummy;
	return Dummy;
}

//const uint32_t GetPGN() const { return 0; }

const uint32_t FCANMessageDynamic::GetID() const
{
	return Serializer->GetID();
}

const uint8_t FCANMessageDynamic::GetLength() const
{
	return Serializer->GetMessageSize();
}

const uint32_t FCANMessageDynamic::GetInterval() const
{
	return INT_DEFAULT;
}

void FCANMessageDynamic::OnAfterRecv()
{
}

void FCANMessageDynamic::OnPreSend()
{
	Frame.Length = Serializer->GetMessageSize();
	Frame.ID = Serializer->GetID();
	SendTimestamp = SodaApp.GetRealtimeTimestamp();
}

//------------------------------------------------------------------------------------------------------------------------------------
void FCANMessageStatic::OnAfterRecv()
{ 
	if (!Dser(Frame.Length, Frame.Data))
	{
		UE_LOG(LogSoda, Error, TEXT("FCANMessageStatic::OnRecv(); Can't deserialize CAN_ID == %i"), Frame.ID);
	}
}
void FCANMessageStatic::OnPreSend()
{ 
	Frame.Length = GetLength();
	Frame.ID = GetRegistredCANID();
	SendTimestamp = SodaApp.GetRealtimeTimestamp();
	if(!Ser(GetLength(), Frame.Data))
	{
		UE_LOG(LogSoda, Error, TEXT("FCANMessageStatic::OnPreSend(); Can't serialize CAN_ID == %i"), Frame.ID);
	}
}

}