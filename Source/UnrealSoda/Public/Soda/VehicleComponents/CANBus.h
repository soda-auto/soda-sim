// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "Soda/VehicleComponents/VehicleBaseComponent.h"
#include "Soda/DBC/Common.h"
#include "Soda/SodaTypes.h"
#include "Soda/Misc/PrecisionTimer.hpp"
#include <unordered_map>
#include <mutex>
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

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = CANBus, SaveGame, meta = (EditInRuntime, ReactivateActor))
	bool bUseIntervaledSendingFrames = false;

	/** [ms] */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = CANBus, SaveGame, meta = (EditInRuntime, ReactivateActor))
	int IntevalStep = 10;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Debug, SaveGame, meta = (EditInRuntime, ReactivateActor))
	bool bLogSendFrames = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Debug, SaveGame, meta = (EditInRuntime, ReactivateActor))
	bool bLogRecvFrames = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Debug, SaveGame, meta = (EditInRuntime, ReactivateActor))
	bool bShowRegMsgs = false;

	FCanDevRecvFrameDelegate RecvDelegate;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	virtual bool ProcessRecvMessage(const TTimestamp& Timestamp, const dbc::FCanFrame& CanFrame);
	virtual int SendFrame(const dbc::FCanFrame& CanFrame);

	virtual void RegisterCanDev(UCANDevComponent* CANDev);
	virtual bool UnregisterCanDev(UCANDevComponent* CANDev);

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
		std::lock_guard<std::mutex> Lock{ regMsgsMutex };
		RecvMessages[CAN_ID] = Msg;
		return Msg;
	}

	template<typename T>
	TSharedPtr<T> RegSendMsg(int64 CAN_ID = CANID_DEFAULT)
	{
		CAN_ID = (CAN_ID == CANID_DEFAULT ? T::Default_CANID : CAN_ID);
		auto Msg = MakeShared<T>(CAN_ID);
		std::lock_guard<std::mutex> Lock{ regMsgsMutex };
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
	UPROPERTY(Transient)
	TSet<UCANDevComponent*> RegistredCANDev;

	int PkgSent = 0;
	int PkgSentErr = 0;
	int PkgReceived = 0;
	int PkgDecoded = 0;

	std::unordered_map<std::uint64_t, TSharedPtr<dbc::FCANMessage>> RecvMessages;
	std::unordered_map<std::uint64_t, TSharedPtr<dbc::FCANMessage>> SendMessages;

	FPrecisionTimer PrecisionTimer;
	int IntervaledThreadCounter;

	std::mutex regMsgsMutex;
};
