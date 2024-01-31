// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/DBC/Helpers.h"

namespace dbc
{

//--------------------------------------------------------------------------------------------------------------------------------------------
void FSignalGetSet::Register(dbc::FCANMessageDynamic& Msg)
{
    check(0); // TODO
}

//--------------------------------------------------------------------------------------------------------------------------------------------
FCANMessageWrapper::FCANMessageWrapper(dbc::FMessageMap* MessageMap, TAttribute<int64> Address, ECanFrameDir Dir, ECanFrameType Type)
    : MessageMap(MessageMap)
    , Address(Address)
    , CanFrameDir(Dir)
    , CanFrameType(Type)
{
    check(MessageMap);
    MessageMap->Messages.Add(this);
}

FCANMessageWrapper::~FCANMessageWrapper() 
{
    if (MessageMap)
    {
        MessageMap->Messages.Remove(this);
    }
}

void FCANMessageWrapper::Destroy()
{
    MessageMap = nullptr;
}

//--------------------------------------------------------------------------------------------------------------------------------------------
bool FCANMessageDynamicWrapper::RegisterMessage()
{
    if (MessageMap)
    {
        if (GetType() == ECanFrameType::J1939)
        {
            if (GetDir() == ECanFrameDir::Input)
            {
                Msg = StaticCastSharedPtr<FCANMessageDynamic>(MessageMap->CANBus->RegRecvMsgJ1939(MessageName, GetAddress()));
            }
            else
            {
                Msg = StaticCastSharedPtr<FCANMessageDynamic>(MessageMap->CANBus->RegSendMsgJ1939(MessageName, GetAddress()));
            }
        }
        else
        {
            if (GetDir() == ECanFrameDir::Input)
            {
                Msg = StaticCastSharedPtr<FCANMessageDynamic>(MessageMap->CANBus->RegRecvMsg(MessageName, GetAddress()));
            }
            else
            {
                Msg = StaticCastSharedPtr<FCANMessageDynamic>(MessageMap->CANBus->RegSendMsg(MessageName, GetAddress()));
            }
        }

        if (Msg.IsValid())
        {
            for (auto& It : Signals)
            {
                It.get().Register(*Msg);
            }
        }
    }

    return Msg.IsValid();
}

void FCANMessageDynamicWrapper::UnregisterMessage()
{
    if (MessageMap)
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

void FCANMessageDynamicWrapper::InitSignales(const FString& SignalsName)
{
    TArray<FString> SignalsNameArray;
    SignalsName.ParseIntoArray(SignalsNameArray, TEXT(","));
    check(SignalsNameArray.Num() == Signals.size());

    for (int i = 0; i < Signals.size(); ++i)
    {
        Signals[i].get().Initialize(SignalsNameArray[i]);
    }
}

bool FCANMessageDynamicWrapper::SendMessage()
{
    return false;
}

//--------------------------------------------------------------------------------------------------------------------------------------------
FMessageMap::~FMessageMap() 
{ 
    UnregisterMessages(); 
    for (auto& it : Messages)
    {
        it->Destroy();
    }
}

bool FMessageMap::RegisterMessages(UCANBusComponent* InCANDev)
{
    UnregisterMessages();

    check(IsValid(InCANDev));
    CANBus = InCANDev;

    bool bRes = true;

    for (auto& it : Messages)
    {
        if (!it->RegisterMessage())
        {
            bRes = false;
        }
    }

    return bRes;
}

void FMessageMap::UnregisterMessages()
{
    if (IsValid(CANBus))
    {
        for (auto& it : Messages)
        {
            it->UnregisterMessage();
        }
    }

    CANBus = nullptr;
}

}
