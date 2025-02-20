// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/VehicleComponents/CANBus.h"
#include "Soda/VehicleComponents/CANDev.h"
#include "Soda/UnrealSoda.h"
#include "Soda/SodaApp.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Soda/DBC/Serialization.h"

static FString CANFrameToString(const dbc::FCanFrame& CanFrame)
{
	return FString::Printf(TEXT("#%08X [%i] "), CanFrame.ID, CanFrame.Length) + BytesToHex(CanFrame.Data, CanFrame.Length);
}


UCANBusComponent::UCANBusComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	GUI.Category = TEXT("CAN");
	GUI.IcanName = TEXT("SodaIcons.Net");
	GUI.ComponentNameOverride = TEXT("CAN Bus");
	GUI.bIsPresentInAddMenu = true;

	Common.bIsTopologyComponent = true;
	Common.Activation = EVehicleComponentActivation::OnStartScenario;
}

void UCANBusComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	SyncDataset();
}


void UCANBusComponent::OnPreActivateVehicleComponent()
{
	//RegistredCANDev.Empty();
}

bool UCANBusComponent::OnActivateVehicleComponent()
{
	//RecvMessages.clear();
	//SendMessages.clear();

	if (!Super::OnActivateVehicleComponent())
	{
		return false;
	}

	if (bUseIntervaledSendingFrames)
	{
		IntervaledThreadCounter = 0;
		PrecisionTimer.TimerDelegate.BindLambda([this](const std::chrono::nanoseconds& InDeltatime, const std::chrono::nanoseconds& Elapsed)
		{
			std::lock_guard<std::mutex> Lock{ regMsgsMutex };
			for (const auto& [Key, Value] : SendMessages)
			{
				if (Value->Frame.ID != 0)
				{
					int Step = FMath::Max(1, FMath::DivideAndRoundNearest<int>(Value->GetInterval(), IntevalStep));
					if (IntervaledThreadCounter % Step == 0)
					{
						Value->SpinLockFrame.Lock();
						auto Frame = Value->Frame;
						Value->SpinLockFrame.Unlock();
						SendFrame(Frame);

						//UE_LOG(LogSoda, Error, TEXT("UCANBusComponent::SendFrame(); ID:%lld; %lldms"), int64(Frame.ID), soda::RawTimestamp<std::chrono::milliseconds>(soda::Now()));
					}
				}
			}
			++IntervaledThreadCounter;
		});
		PrecisionTimer.RealtimeSmoothFactor = 0.5;
		PrecisionTimer.TimerStart(std::chrono::milliseconds(IntevalStep), std::chrono::milliseconds(0));
	}
	
	return true;
}



void UCANBusComponent::OnDeactivateVehicleComponent()
{
	Super::OnDeactivateVehicleComponent();

	PrecisionTimer.TimerStop();

	//RecvMessages.clear();
	//SendMessages.clear();

	//RegistredCANDev.Reset();
}

void UCANBusComponent::RegisterCanDev(UCANDevComponent* CANDev)
{
	if (IsValid(CANDev))
	{
		RegistredCANDev.Add(CANDev);
	}
}

bool UCANBusComponent::UnregisterCanDev(UCANDevComponent* CANDev)
{
	if (IsValid(CANDev))
	{
		return RegistredCANDev.Remove(CANDev) > 0;
	}
	return false;
}

int UCANBusComponent::SendFrame(const dbc::FCanFrame& CanFrame)
{ 
	if (bLoopFrames)
	{
		ProcessRecvMessage(SodaApp.GetRealtimeTimestamp(), CanFrame);
	}

	for (auto& It : RegistredCANDev)
	{
		if (It->SendFrame(CanFrame) < 0)
		{
			++PkgSentErr;
		}
	}

	++PkgSent;

	if (bLogSendFrames)
	{
		UE_LOG(LogSoda, Log, TEXT("UCANBusComponent::SendFrame(); %s"), *CANFrameToString(CanFrame));
	}

	return -1;
}

/*
int UCANBusComponent::SendFrame(const dbc::FCANMessage* Msg)
{
	dbc::FCanFrame Frame;
	Frame.Length = Msg->Serializer->GetMessageSize();
	Frame.ID = Msg->Serializer->GetID();

	
	//if(!Msg->Ser(Msg->GetLength(), Frame.Data))
	//{
	//	UE_LOG(LogSoda, Error, TEXT("UCANBusComponent::SendFrame(); Can't serialize CAN_ID == %i"), Msg->GetID());
	//	return -1;
	//}
	

	const_cast<dbc::FCANMessage*>(Msg)->SendTimestamp = SodaApp.GetRealtimeTimestamp();

	if (Msg->OnPreSend.IsBound())
	{
		Msg->OnPreSend.Execute(Frame);
	}

	return SendFrame(Frame);
}

int UCANBusComponent::SendFrameCustomID(const dbc::FCANMessage* Msg, int64 CustomID)
{
	dbc::FCanFrame Frame;
	Frame.Length = Msg->Serializer->GetMessageSize();
	Frame.ID = CustomID;

	
	//if(!Msg->Ser(Msg->GetLength(), Frame.Data))
	//{
	//	UE_LOG(LogSoda, Error, TEXT("UCANBusComponent::SendFrameCustomID(); Can't serialize CAN_ID == %i"), CustomID);
	//	return -1;
	//}
	

	const_cast<dbc::FCANMessage*>(Msg)->SendTimestamp = SodaApp.GetRealtimeTimestamp();

	if (Msg->OnPreSend.IsBound())
	{
		Msg->OnPreSend.Execute(Frame);
	}

	return SendFrame(Frame);
}

int UCANBusComponent::SendFrameJ1939(const dbc::FCANMessage* Msg, int J1939Addr)
{
	return SendFrameCustomID(Msg, (Msg->Serializer->GetID() & 0xFFFFFF00) | (J1939Addr & 0xFF));
}
*/


TSharedPtr<dbc::FCANMessage> UCANBusComponent::RegRecvMsg(const FString& MessageName, int64 CAN_ID)
{
	auto Serializer = SodaApp.FindDBCSerializator(MessageName);
	if (!Serializer)
	{
		return TSharedPtr<dbc::FCANMessageDynamic>();
	}

	CAN_ID = CAN_ID == CANID_DEFAULT ? Serializer->GetID() : CAN_ID;

	if (auto it = RecvMessages.find(CAN_ID); it != RecvMessages.end())
	{
		return it->second;
	}
	else
	{
		auto Msg = MakeShared<dbc::FCANMessageDynamic>(CAN_ID, Serializer);
		RecvMessages[CAN_ID] = Msg;
		return Msg;
	}
}

TSharedPtr<dbc::FCANMessage> UCANBusComponent::RegSendMsg(const FString& MessageName, int64 CAN_ID)
{
	auto Serializer = SodaApp.FindDBCSerializator(MessageName);
	if (!Serializer)
	{
		return TSharedPtr<dbc::FCANMessageDynamic>();
	}

	CAN_ID = CAN_ID == CANID_DEFAULT ? Serializer->GetID() : CAN_ID;

	if (auto it = SendMessages.find(CAN_ID); it != SendMessages.end())
	{
		return it->second;
	}
	else
	{
		auto Msg = MakeShared<dbc::FCANMessageDynamic>(CAN_ID, Serializer);
		SendMessages[CAN_ID] = Msg;
		return Msg;
	}
}

void UCANBusComponent::UnregRecvMsg(int64 CAN_ID)
{
	std::lock_guard<std::mutex> Lock{ regMsgsMutex };
	RecvMessages.erase(CAN_ID);
}

void UCANBusComponent::UnregSendMsg(int64 CAN_ID)
{
	std::lock_guard<std::mutex> Lock{ regMsgsMutex };
	SendMessages.erase(CAN_ID);
}

TSharedPtr<dbc::FCANMessage> UCANBusComponent::RegRecvMsgJ1939(const FString& MessageName, uint8 SourceAddress)
{ 
	if (auto Serializer = SodaApp.FindDBCSerializator(MessageName))
	{
		return RegRecvMsg(MessageName, (Serializer->GetID() & 0x3FFFF00) | SourceAddress);
	}
	else
	{
		return TSharedPtr<dbc::FCANMessageDynamic>();
	}
}

TSharedPtr<dbc::FCANMessage> UCANBusComponent::RegSendMsgJ1939(const FString& MessageName, uint8 SourceAddress)
{ 
	if (auto Serializer = SodaApp.FindDBCSerializator(MessageName))
	{
		return RegSendMsg(MessageName, (Serializer->GetID() & 0x3FFFF00) | SourceAddress);
	}
	else
	{
		return TSharedPtr<dbc::FCANMessageDynamic>();
	}
}

void UCANBusComponent::UnregRecvMsgJ1939(const FString& MessageName, uint8 SourceAddress)
{ 
	if (auto Serializer = SodaApp.FindDBCSerializator(MessageName))
	{
		UnregRecvMsg(Serializer->GetID() & (0x3FFFF00 | SourceAddress));
	}
}

void UCANBusComponent::UnregSendMsgJ1939(const FString& MessageName, uint8 SourceAddress)
{ 
	if (auto Serializer = SodaApp.FindDBCSerializator(MessageName))
	{
		UnregSendMsg((Serializer->GetID() & 0x3FFFF00) | SourceAddress);
	}
}

bool UCANBusComponent::ProcessRecvMessage(const TTimestamp& Timestamp, const dbc::FCanFrame & CanFrame)
{
	if (!HealthIsWorkable())
	{
		return false;
	}

	++PkgReceived;

	RecvDelegate.Broadcast(Timestamp, CanFrame);

	// Try to find msg by pure CAN ID
	auto It = RecvMessages.find(CanFrame.ID);

	// Try to find J1939 msg by PGN + source address
	if (It == RecvMessages.end())
	{
		It = RecvMessages.find(CanFrame.ID & 0x3FFFFFF);
	}

	// Try to find J1939 msg by broadcast PGN 
	if (It == RecvMessages.end())
	{
		It = RecvMessages.find((CanFrame.ID & 0x3FFFF00) | 0xFE);
	}

	if(It != RecvMessages.end())
	{
		It->second->RecvTimestamp = Timestamp;
		It->second->Frame = CanFrame;
		It->second->OnAfterRecv();
		++PkgDecoded;

		if (bLogRecvFrames)
		{
			UE_LOG(LogSoda, Log, TEXT("UCANBusComponent::ProcessRecvMessage(); %s -- found"), *CANFrameToString(CanFrame));
		}

		return true;
	}
	else
	{
		if (bLogRecvFrames)
		{
			UE_LOG(LogSoda, Log, TEXT("UCANBusComponent::ProcessRecvMessage(); %s -- not found"), *CANFrameToString(CanFrame));
		}

		return false;
	}
}

FString UCANBusComponent::GetRemark() const
{
	return "";
}

void UCANBusComponent::DrawDebug(UCanvas* Canvas, float& YL, float& YPos)
{
	Super::DrawDebug(Canvas, YL, YPos);

	if (Common.bDrawDebugCanvas)
	{
		UFont* RenderFont = GEngine->GetSmallFont();
		Canvas->SetDrawColor(FColor::White);
		YPos += Canvas->DrawText(RenderFont, FString::Printf(TEXT("Registred CAN Dev Num: %d"), RegistredCANDev.Num()), 16, YPos);
		YPos += Canvas->DrawText(RenderFont, FString::Printf(TEXT("Registred Recv Msgs: %d"), RecvMessages.size()), 16, YPos);
		if (bShowRegMsgs)
		{
			for (auto& [Key, Value] : RecvMessages)
			{
				YPos += Canvas->DrawText(RenderFont, FString::Printf(TEXT("    %s: #%08X"), *Value->GetName(), Value->GetRegistredCANID()), 16, YPos);
			}
		}
		YPos += Canvas->DrawText(RenderFont, FString::Printf(TEXT("Registred Send Msgs: %d"), SendMessages.size()), 16, YPos);
		if (bShowRegMsgs)
		{
			for (auto& [Key, Value] : SendMessages)
			{
				YPos += Canvas->DrawText(RenderFont, FString::Printf(TEXT("    %s: #%08X"), *Value->GetName(), Value->GetRegistredCANID()), 16, YPos);
			}
		}
		YPos += Canvas->DrawText(RenderFont, FString::Printf(TEXT("Pkg Sent: %d"), PkgSent), 16, YPos);
		YPos += Canvas->DrawText(RenderFont, FString::Printf(TEXT("Pkg Received: %d"), PkgReceived), 16, YPos);
		YPos += Canvas->DrawText(RenderFont, FString::Printf(TEXT("Pkg SentErr: %d"), PkgSentErr), 16, YPos);
		YPos += Canvas->DrawText(RenderFont, FString::Printf(TEXT("Pkg Decoded: %d"), PkgDecoded), 16, YPos);
	}
}

