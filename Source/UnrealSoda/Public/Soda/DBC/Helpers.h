// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Containers/UnrealString.h"
#include "Kismet/KismetStringLibrary.h"
#include "Soda/DBC/Common.h"
#include "Soda/DBC/Serialization.h"
#include "Soda/VehicleComponents/CANBus.h"
#include <vector>
#include <functional>

class UCANBusComponent;

#define DEFINE_DBC_MSG_STRUCT(MessageName, ...) \
    struct F ## MessageName: public dbc::FCANMessageDynamicWrapper { \
        F ## MessageName(dbc::FMessageMap * MessageMap, TAttribute<int64> Address, ECanFrameDir Dir, ECanFrameType Type) \
            : dbc::FCANMessageDynamicWrapper(MessageMap, #MessageName, Address, bIsRecv, bIsJ1939) { \
            Signals = {__VA_ARGS__};\
            InitSignales(TEXT(#__VA_ARGS__)); \
        } \
        dbc::FSignalGetSet __VA_ARGS__; \
    }; 

#define DEFINE_DBC_MSG_DYNAMIC(MessageName, Addr, Dir, Type, ...) \
    DEFINE_DBC_MSG_STRUCT(MessageName, __VA_ARGS__) \
    F ## MessageName MessageName{this, TAttribute<int64>::CreateLambda([&](){ return (Addr); }), Dir, Type};

#define DEFINE_DBC_MSG_STATIC(Namespace, MessageType, CANID, Dir, Type) \
    dbc::FCANMessageStaticWrapper< Namespace::F ## MessageType > MessageType{this, TAttribute<int64>::CreateLambda([&](){ return (CANID); }), Dir, Type};

enum ECanFrameDir
{
    Input,
    Output
};

enum ECanFrameType
{
    Std,
    J1939
};

namespace dbc
{
struct FCANMessageWrapper;

/**
 * FMessageMap
 */
class UNREALSODA_API FMessageMap
{
    friend FCANMessageWrapper;

public:
    FMessageMap() = default;
    ~FMessageMap();

    bool RegisterMessages(UCANBusComponent* CANBus);
    void UnregisterMessages();

    TArray<FCANMessageWrapper*> Messages;
    UCANBusComponent* CANBus = nullptr; // Don't use TWeakObjectPtr, becouse it is slow
    bool bSendImmediately = false;
};

/**
 * FSignalGetSet 
 * Helper for FCANMessageWrapper
 */
class UNREALSODA_API FSignalGetSet
{
public:
    void Initialize(const FString& InSignalName) { SignalName = InSignalName; }

    void Register(dbc::FCANMessageDynamic& Msg);

    /*
    void SetRaw(uint64 Val) { Signal->Encode(Val, CanFrame->Data); }

    uint64 GetRaw() { return Signal->Decode(CanFrame->Data); }

    template<typename T>
    void Set(T Val)
    {
        Signal->Encode(Signal->PhysToRaw(static_cast<T>(Val)), CanFrame->Data);
    }
     
    template<typename T>
    T Get()
    {
        return static_cast<T>(Signal->RawToPhys(Signal->Decode(CanFrame->Data)));
    }
    */

protected:
    //dbc::FSignal* Signal = nullptr; 
    TSharedPtr<dbcppp::ISignal> Signal;
    dbc::FCanFrame* CanFrame = nullptr;
    FString SignalName;
};

/**
 * FCANMessageWrapper
 * Common message wrapper
 */
struct UNREALSODA_API FCANMessageWrapper
{
    FCANMessageWrapper(dbc::FMessageMap* MessageMap, TAttribute<int64> Address, ECanFrameDir Dir, ECanFrameType Type);
    virtual ~FCANMessageWrapper();

    virtual void Destroy();

    int64 GetAddress() const { return Address.Get(); }
    ECanFrameDir GetDir() const { return CanFrameDir; }
    ECanFrameType GetType() const { return CanFrameType; }

    virtual bool RegisterMessage() = 0;
    virtual void UnregisterMessage() = 0;
    virtual bool SendMessage() = 0;

protected:
    dbc::FMessageMap* MessageMap;
    TAttribute<int64> Address;
    ECanFrameDir CanFrameDir;
    ECanFrameType CanFrameType;
};

/**
 * FCANMessageDynamicWrapper
 * Message wrapper for FCANMessageDynamic
 * WARNING!!! The implementation isn't finished
 * TODO: finish implementation
 */
struct UNREALSODA_API FCANMessageDynamicWrapper: public FCANMessageWrapper
{
    FCANMessageDynamicWrapper(dbc::FMessageMap* MessageMap, const FString& MessageName, TAttribute<int64> Address, ECanFrameDir Dir, ECanFrameType Type)
        : FCANMessageWrapper(MessageMap, Address, Dir, Type)
        , MessageName(MessageName)
    {}
    virtual ~FCANMessageDynamicWrapper() { UnregisterMessage(); }

    virtual bool RegisterMessage() override;
    virtual void UnregisterMessage() override;
    virtual bool SendMessage() override;

    TSharedPtr<dbc::FCANMessageDynamic> Msg;

protected:
    FString MessageName;
    std::vector<std::reference_wrapper<FSignalGetSet>> Signals;
    void InitSignales(const FString& SignalsName);
};

/**
 * FCANMessageStaticWrapper
 * Message wrapper for the code generated DBC ser/des
 */
template<class T>
struct FCANMessageStaticWrapper : public FCANMessageWrapper
{
    FCANMessageStaticWrapper(dbc::FMessageMap* MessageMap, TAttribute<int64> Address, ECanFrameDir Dir, ECanFrameType Type)
        : FCANMessageWrapper(MessageMap, Address, Dir, Type)
    {}
    virtual ~FCANMessageStaticWrapper() { UnregisterMessage(); }

    T* operator->() const
    {
        return Msg.Get();
    }

    virtual bool RegisterMessage() override
    {
        if (IsValid(MessageMap->CANBus) && MessageMap)
        {
            if (GetType() == ECanFrameType::J1939)
            {
                if (GetDir() == ECanFrameDir::Input)
                {
                    Msg = MessageMap->CANBus->RegRecvMsgJ1939<T>(GetAddress());
                }
                else
                {
                    Msg = MessageMap->CANBus->RegSendMsgJ1939<T>(GetAddress());
                }
            }
            else
            {
                if (GetDir() == ECanFrameDir::Input)
                {
                    Msg = MessageMap->CANBus->RegRecvMsg<T>(GetAddress());
                }
                else
                {
                    Msg = MessageMap->CANBus->RegSendMsg<T>(GetAddress());
                }
            }
        }

        return Msg.IsValid();
    }

    virtual void UnregisterMessage() override
    {
        if (MessageMap)
        {
            if (IsValid(MessageMap->CANBus))
            {
                if (GetDir() == ECanFrameDir::Input)
                {
                    MessageMap->CANBus->UnregRecvMsg(Msg->GetRegistredCANID());
                }
                else
                {
                    MessageMap->CANBus->UnregSendMsg(Msg->GetRegistredCANID());
                }
            }
        }
    }

    virtual bool SendMessage() override
    {
        if (GetDir() == ECanFrameDir::Output && MessageMap)
        {
            {
                UE::TScopeLock<UE::FSpinLock> ScopeLock(Msg->SpinLockFrame);
                Msg->OnPreSend();
            }
            if(MessageMap->bSendImmediately || !MessageMap->CANBus->bUseIntervaledSendingFrames) MessageMap->CANBus->SendFrame(Msg->Frame);
            return true;
        }
        return false;
    }

    template<typename FunctorType>
    bool SendMessage(FunctorType&& OnAfterSer)
    {
        if (GetDir() == ECanFrameDir::Output && MessageMap)
        {
            {
                UE::TScopeLock<UE::FSpinLock> ScopeLock(Msg->SpinLockFrame);
                Msg->OnPreSend();
                OnAfterSer(Msg->Frame);
            }
            if (MessageMap->bSendImmediately || !MessageMap->CANBus->bUseIntervaledSendingFrames) MessageMap->CANBus->SendFrame(Msg->Frame);
            return true;
        }
        return false;
    }

    template<typename FunctorType, typename... VarTypes>
    bool SendMessage(FunctorType&& OnAfterSer, VarTypes... Vars)
    {
        if (GetDir() == ECanFrameDir::Output && MessageMap)
        {
            {
                UE::TScopeLock<UE::FSpinLock> ScopeLock(Msg->SpinLockFrame);
                Msg->OnPreSend();
                OnAfterSer(Msg->Frame, Vars...);
            }
            if (MessageMap->bSendImmediately || !MessageMap->CANBus->bUseIntervaledSendingFrames) MessageMap->CANBus->SendFrame(Msg->Frame);
            return true;
        }
        return false;
    }

    TSharedPtr<T> Msg;
};

} // namespace dbc





