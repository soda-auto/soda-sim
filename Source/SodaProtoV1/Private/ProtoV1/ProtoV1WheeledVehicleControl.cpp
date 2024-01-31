// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/ProtoV1/ProtoV1WheeledVehicleControl.h"
#include "Soda/VehicleComponents/Drivers/GenericVehicleDriverComponent.h"
#include "Soda/UnrealSoda.h"
#include "Soda/SodaApp.h"
#include "Common/UdpSocketBuilder.h"

bool UProtoV1WheeledVehicleControl::StartListen(UVehicleBaseComponent* Parent)
{
	FIPv4Endpoint Endpoint(FIPv4Address(0, 0, 0, 0), RecvPort);
	ListenSocket = FUdpSocketBuilder(TEXT("UProtoV1WheeledVehicleControl"))
		.AsNonBlocking()
		.AsReusable()
		.BoundToEndpoint(Endpoint)
		.WithReceiveBufferSize(0xFFFF);
	check(ListenSocket);
	FTimespan ThreadWaitTime = FTimespan::FromMilliseconds(100);
	UDPReceiver = new FUdpSocketReceiver(ListenSocket, ThreadWaitTime, TEXT("UProtoV1WheeledVehicleControl"));
	check(UDPReceiver);
	UDPReceiver->OnDataReceived().BindUObject(this, &UProtoV1WheeledVehicleControl::Recv);
	UDPReceiver->Start();

	return true;
}

void UProtoV1WheeledVehicleControl::StopListen()
{
	if (UDPReceiver)
	{
		UDPReceiver->Stop();
		delete UDPReceiver;
	}
	UDPReceiver = nullptr;

	if (ListenSocket)
	{
		ListenSocket->Close();
		delete ListenSocket;
	}
	ListenSocket = nullptr;
}

void UProtoV1WheeledVehicleControl::Recv(const FArrayReaderPtr& ArrayReaderPtr, const FIPv4Endpoint& EndPt)
{
	if (ArrayReaderPtr->Num() == sizeof(Msg))
	{
		::memcpy(&Msg, ArrayReaderPtr->GetData(), ArrayReaderPtr->Num());
		RecvTimestamp = SodaApp.GetRealtimeTimestamp();
	}
	else
	{
		UE_LOG(LogSoda, Error, TEXT("UProtoV1WheeledVehicleControl::Recv(); Got %i bytes, expected %i"), ArrayReaderPtr->Num(), sizeof(Msg));
	}
}

bool UProtoV1WheeledVehicleControl::GetControl(soda::FWheeledVehiclControl& Control) const
{
	Control.SteerReq = Msg.steer_req;
	Control.AccDecelReq = Msg.acc_decel_req;
	Control.GearReq = ENGear(Msg.gear_req);
	Control.RecvTimestamp = RecvTimestamp;
	return true;
}
