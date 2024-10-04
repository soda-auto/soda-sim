// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/DBC/Serialization.h"
#include <dbcppp/Network.h>
#include <fstream>

namespace dbc
{
    FMessageSerializator::FMessageSerializator(std::unique_ptr<dbcppp::IMessage> Impl)
        : MessageImpl(std::move(Impl))
    {
        Name = FString(MessageImpl->Name().c_str());
        Comment = FString(MessageImpl->Comment().c_str());
    }

    uint64_t FMessageSerializator::GetID() const
    {
        return MessageImpl->Id();
    }

    const int64 FMessageSerializator::GetMessageSize() const
    {
        return MessageImpl->MessageSize();
    }

    /*
    const FSignal& FMessageSerializator::GetSignals(std::size_t i) const
    {
        return MessageImpl->Signals_Get(i);
    }

    uint64_t FMessageSerializator::GetNumSignals() const
    {
        return MessageImpl->Signals_Size();
    }

    const FAttribute& FMessageSerializator::GetAttributeValues(std::size_t i) const
    {
        return MessageImpl->AttributeValues_Get(i);
    }

    uint64_t FMessageSerializator::GetNumAttributeValues() const
    {
        return MessageImpl->AttributeValues_Size();
    }

    const FSignalGroup& FMessageSerializator::GetSignalGroups(std::size_t i) const
    {
        return MessageImpl->SignalGroups_Get(i);
    }

    uint64_t FMessageSerializator::GetNumSignalGroups() const
    {
        return MessageImpl->SignalGroups_Size();
    }

    const FSignal* FMessageSerializator::GetMuxSignal() const
    {
        return MessageImpl->MuxSignal();
    }
    */

    bool LoadDBC(const FString& FileName, TMap<FString, TSharedPtr<dbc::FMessageSerializator>>& Map)
    {
        std::unique_ptr<dbcppp::INetwork> Net;
        std::ifstream idbc(TCHAR_TO_UTF8(*FileName));
        if (!idbc.is_open())
        {
            return false;
        }
        Net = dbcppp::INetwork::LoadDBCFromIs(idbc);
        if (!Net)
        {
            return false;
        }
        for (const dbcppp::IMessage& Msg : Net->Messages())
        {
            Map.Add(UTF8_TO_TCHAR(Msg.Name().c_str()), MakeShared<FMessageSerializator>(Msg.Clone()));
        }

        return true;
    }
}