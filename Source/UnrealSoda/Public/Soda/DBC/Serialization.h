// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Containers/UnrealString.h"
#include <variant>
#include  <memory>

namespace dbcppp
{
    class ISignal;
    class IMessage;
    class IAttribute;
    class IAttributeDefinition;
    class ISignalMultiplexerValue;
    class IValueEncodingDescription;
    class ISignalGroup;
}

namespace dbc
{
/*
class FSignal;
class FMessageSerializator;
class FAttribute;
class FAttributeDefinition;
class FSignalMultiplexerValue;
class FValueEncodingDescription;
class FSignalGroup;

 //---------------------------------------------------------------------------------------
class FSignalMultiplexerValue
{
public:
    struct FRange
    {
        std::size_t from;
        std::size_t to;
    };
        
    const FString& GetSwitchName() const;
    const FRange& GetValueRanges(std::size_t i) const;
    uint64_t GetNumValueRanges() const;

    //DBCPPP_MAKE_ITERABLE(ISignalMultiplexerValue, ValueRanges, Range);

    //bool operator==(const ISignalMultiplexerValue& rhs) const = 0;
    //bool operator!=(const ISignalMultiplexerValue& rhs) const = 0;

protected:
    TSharedPtr<dbcppp::ISignalMultiplexerValue> SignalMultiplexerValueImpl;
};

//---------------------------------------------------------------------------------------
class FValueEncodingDescription
{
public:
    int64_t Value() const;
    const FString& Description() const;

    //bool operator==(const IValueEncodingDescription& rhs) const = 0;
    //bool operator!=(const IValueEncodingDescription& rhs) const = 0;

protected:
    TSharedPtr<dbcppp::IValueEncodingDescription> ValueEncodingDescriptionImpl;
};


//---------------------------------------------------------------------------------------
class FAttributeDefinition
{
public:
    enum class EObjectType
    {
        Network,
        Node,
        Message,
        Signal,
        EnvironmentVariable,
    };
    struct ValueTypeInt
    {
        int64_t minimum;
        int64_t maximum;
    };
    struct ValueTypeHex
    {
        int64_t minimum;
        int64_t maximum;
    };
    struct ValueTypeFloat
    {
        double minimum;
        double maximum;
    };
    struct ValueTypeString
    {
    };
    struct ValueTypeEnum
    {
        std::vector<FString> values;
    };
    using value_type_t = std::variant<ValueTypeInt, ValueTypeHex, ValueTypeFloat, ValueTypeString, ValueTypeEnum>;

    EObjectType ObjectType() const;
    const FString& Name() const;
    const value_type_t& ValueType() const;

    //bool operator==(const IAttributeDefinition& rhs) const = 0;
    //bool operator!=(const IAttributeDefinition& rhs) const = 0;

protected:
    TSharedPtr<dbcppp::IAttributeDefinition> AttributeDefinitionImpl;
};

//---------------------------------------------------------------------------------------
class FSignal
{
public:
    enum class EErrorCode : uint64_t
    {
        NoError,
        MaschinesFloatEncodingNotSupported = 1,
        MaschinesDoubleEncodingNotSupported = 2,
        SignalExceedsMessageSize = 4,
        WrongBitSizeForExtendedDataType = 8
    };

    enum class EMultiplexer
    {
        NoMux, MuxSwitch, MuxValue
    };

    enum class EByteOrder
    {
        BigEndian = 0,
        LittleEndian = 1
    };

    enum class EValueType
    {
        Signed, Unsigned
    };

    enum class EExtendedValueType
    {
        Integer, Float, Double
    };

    const FString& Name() const;
    EMultiplexer MultiplexerIndicator() const;
    uint64_t MultiplexerSwitchValue() const;
    uint64_t StartBit() const;
    uint64_t BitSize() const;
    EByteOrder ByteOrder() const;
    EValueType ValueType() const;
    double Factor() const;
    double Offset() const;
    double Minimum() const;
    double Maximum() const;
    const FString& Unit() const;
    const FString& Receivers_Get(std::size_t i) const;
    uint64_t Receivers_Size() const;
    const FValueEncodingDescription& ValueEncodingDescriptions_Get(std::size_t i) const;
    uint64_t ValueEncodingDescriptions_Size() const;
    const FAttribute& AttributeValues_Get(std::size_t i) const;
    uint64_t AttributeValues_Size() const;
    const FString& Comment() const;
    EExtendedValueType ExtendedValueType() const;
    const FSignalMultiplexerValue& SignalMultiplexerValues_Get(std::size_t i) const;
    uint64_t SignalMultiplexerValues_Size() const;

    bool Error(EErrorCode code) const;

    //bool operator==(const FSignal& rhs) const;
    //bool operator!=(const FSignal& rhs) const;

    inline uint64_t Decode(const void* bytes) const noexcept;
    inline void Encode(uint64_t raw, void* buffer) const noexcept;

    inline double RawToPhys(uint64_t raw) const noexcept;
    inline uint64_t PhysToRaw(double phys) const noexcept;

protected:
    TSharedPtr<dbcppp::ISignal> SignalImpl;
};

//---------------------------------------------------------------------------------------
class FSignalGroup
{
    uint64_t MessageId() const;
    const FString& Name() const;
    uint64_t Repetitions() const;
    const FString& SignalNames_Get(std::size_t i) const;
    uint64_t SignalNames_Size() const;

    //DBCPPP_MAKE_ITERABLE(ISignalGroup, SignalNames, std::string);

    //bool operator==(const ISignalGroup& rhs) const = 0;
    //bool operator!=(const ISignalGroup& rhs) const = 0;

protected:
    TSharedPtr<dbcppp::ISignalGroup> SignalGroupImpl;
};

//---------------------------------------------------------------------------------------
class FAttribute
{
    using hex_value_t = int64_t;
    using value_t = std::variant<int64_t, double, FString>;

    const FString& Name() const;
    FAttributeDefinition::EObjectType ObjectType() const;
    const value_t& Value() const;

    //bool operator==(const FAttribute& rhs) const = 0;
    //bool operator!=(const FAttribute& rhs) const = 0;

protected:
    TSharedPtr<dbcppp::IAttribute> AttributeImpl;
};
*/

//---------------------------------------------------------------------------------------
class FMessageSerializator
{
public:

    FMessageSerializator(std::unique_ptr<dbcppp::IMessage> Impl);

    uint64_t GetID() const;
    const FString& GetName() const { return Name; }
	const int64 GetMessageSize() const;
    //const FString& GetTransmitter() const;
    //const FString& GetMessageTransmitters(std::size_t i) const;
    //uint64_t MessageTransmittersSize() const;
    //const FSignal& GetSignals(std::size_t i) const;
    //uint64_t GetNumSignals() const;
    //const FAttribute& GetAttributeValues(std::size_t i) const;
    //uint64_t GetNumAttributeValues() const;
    const FString& GetComment() const { return Comment; }
    //const FSignalGroup& GetSignalGroups(std::size_t i) const;
    uint64_t GetNumSignalGroups() const;
    //const FSignal* GetMuxSignal() const;

    //bool operator==(const FMessageSerializator& message) const = 0;
    //bool operator!=(const FMessageSerializator& message) const = 0;

protected:
    std::unique_ptr<dbcppp::IMessage> MessageImpl;

    FString Name;
    FString Comment;
};

bool LoadDBC(const FString& FileName, TMap<FString, TSharedPtr<dbc::FMessageSerializator>> & Map);

}
