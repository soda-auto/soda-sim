// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "Soda/VehicleComponents/VehicleBaseComponent.h"
#include "Soda/DBC/Common.h"
#include "Soda/SodaTypes.h"
#include <unordered_map>
#include "CANBus.generated.h"

#define CANID_DEFAULT -1

DECLARE_MULTICAST_DELEGATE_TwoParams(FCanDevRecvFrameDelegate, TTimestamp, const dbc::FCanFrame &);

class UCANDevComponent;

/**
 * UCANBusComponent
 * CAN bus or network imitation
 */
UCLASS(ClassGroup = Soda, BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class UNREALSODA_API UCANBusComponent : public UVehicleBaseComponent
{
	GENERATED_UCLASS_BODY()

public:
	/** Whether to put the sent frame to the receive queue. 
	 * This can be useful if multiple components communicate with each other via this bus.
	 * Beware, setting this flag will require more performance from the vehicle simulation thread, 
	 * as all messages sent to this bus will be deserialized on the vehicle simulation thread.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = CANBus, SaveGame, meta = (EditInRuntime, ReactivateActor))
	bool bLoopFrames = false; 

	FCanDevRecvFrameDelegate RecvDelegate;

public:
	virtual bool ProcessRecvMessage(const TTimestamp& Timestamp, const dbc::FCanFrame& CanFrame);
	virtual int SendFrame(const dbc::FCanFrame& CanFrame);
	virtual void RegisterCanDev(UCANDevComponent* CANDev);

public:
	virtual void OnPreActivateVehicleComponent() override;
	virtual bool OnActivateVehicleComponent() override;
	virtual void OnDeactivateVehicleComponent() override;

public:
	TSharedPtr<dbc::FCANMessage> RegRecvMsg(const FString & MessageName, int64 CAN_ID = CANID_DEFAULT);
	TSharedPtr<dbc::FCANMessage> RegSendMsg(const FString& MessageName, int64 CAN_ID = CANID_DEFAULT);
	TSharedPtr<dbc::FCANMessage> RegRecvMsgJ1939(const FString& MessageName, uint8 SourceAddress = 0xFE);
	TSharedPtr<dbc::FCANMessage> RegSendMsgJ1939(const FString& MessageName, uint8 SourceAddress = 0xFE);

	template<typename T>
	TSharedPtr<T> RegRecvMsg(int64 CAN_ID = CANID_DEFAULT)
	{
		CAN_ID = (CAN_ID == CANID_DEFAULT ? T::Default_CANID : CAN_ID);
		auto Msg = MakeShared<T>(CAN_ID);
		RecvMessages[CAN_ID] = Msg;
		return Msg;
	}

	template<typename T>
	TSharedPtr<T> RegSendMsg(int64 CAN_ID = CANID_DEFAULT)
	{
		CAN_ID = (CAN_ID == CANID_DEFAULT ? T::Default_CANID : CAN_ID);
		auto Msg = MakeShared<T>(CAN_ID);
		SendMessages[CAN_ID] = Msg;
		return Msg;
	}

	template<typename T>
	TSharedPtr<T> RegRecvMsgJ1939(uint8 SourceAddress = 0xFE)
	{
		auto Msg = RegRecvMsg<T>(T::Default_CANID & 0x3FFFF00 | SourceAddress);
		return Msg;
	}

	template<typename T>
	TSharedPtr<T> RegSendMsgJ1939(uint8 SourceAddress = 0xFE)
	{
		auto Msg = RegSendMsg<T>(T::Default_CANID & 0x3FFFF00 | SourceAddress);
		return Msg;
	}

	void UnregRecvMsg(int64 CAN_ID);
	void UnregSendMsg(int64 CAN_ID);
	void UnregRecvMsgJ1939(const FString& MessageName, uint8 SourceAddress = 0xFE);
	void UnregSendMsgJ1939(const FString& MessageName, uint8 SourceAddress = 0xFE);

public:
	virtual FString GetRemark() const override;
	virtual void DrawDebug(UCanvas* Canvas, float& YL, float& YPos) override;

protected:
	UPROPERTY()
	TSet<UCANDevComponent*> RegistredCANDev;

	int PkgSent = 0;
	int PkgSentErr = 0;
	int PkgReceived = 0;
	int PkgDecoded = 0;

	std::unordered_map<std::uint64_t, TSharedPtr<dbc::FCANMessage>> RecvMessages;
	std::unordered_map<std::uint64_t, TSharedPtr<dbc::FCANMessage>> SendMessages;
};
