// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/ProtoV1/ProtoV1WheeledVehicleControl.h"
#include "Soda/VehicleComponents/Drivers/GenericVehicleDriverComponent.h"
#include "Soda/UnrealSoda.h"
#include "Soda/SodaApp.h"
#include "Common/UdpSocketBuilder.h"
#include "Engine/Engine.h"
#include "Engine/Canvas.h"

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

bool UProtoV1WheeledVehicleControl::GetControl(soda::FWheeledVehiclControlMode1& Control) const
{
	Control.SteerReq = Msg.steer_req;
	Control.AccDecelReq = Msg.acc_decel_req;
	Control.GearStateReq = EGearState(Msg.gear_state_req);
	Control.GearNumReq = Msg.gear_num_req;
	Control.RecvTimestamp = RecvTimestamp;
	return true;
}

FString UProtoV1WheeledVehicleControl::GetRemark() const
{
	return  "udp://*:" + FString::FromInt(RecvPort);
}

void UProtoV1WheeledVehicleControl::DrawDebug(UCanvas* Canvas, float& YL, float& YPos)
{
	Super::DrawDebug(Canvas, YL, YPos);

	UFont* RenderFont = GEngine->GetSmallFont();

	const uint64 dt = std::chrono::duration_cast<std::chrono::milliseconds>(SodaApp.GetRealtimeTimestamp() - RecvTimestamp).count();

	Canvas->SetDrawColor(FColor::White);
	YPos += Canvas->DrawText(RenderFont, FString::Printf(TEXT("steer_req: %.2f"), Msg.steer_req), 16, YPos);
	YPos += Canvas->DrawText(RenderFont, FString::Printf(TEXT("acc_decel_req: %.1f"), Msg.acc_decel_req), 16, YPos);
	YPos += Canvas->DrawText(RenderFont, FString::Printf(TEXT("gear_state_req: %d"), (int)Msg.gear_state_req), 16, YPos);
	YPos += Canvas->DrawText(RenderFont, FString::Printf(TEXT("gear_num_req: %d"), (int)Msg.gear_num_req), 16, YPos);

	if (dt > 10000)
	{
		YPos += Canvas->DrawText(RenderFont, FString::Printf(TEXT("dtime: timeout")), 16, YPos);
	}
	else
	{
		YPos += Canvas->DrawText(RenderFont, FString::Printf(TEXT("dtime: %dms"), dt), 16, YPos);
	}
}