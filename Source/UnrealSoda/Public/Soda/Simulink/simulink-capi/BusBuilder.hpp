// Copyright (c) 2020-2021, Danish Belal.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice, this
//    list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
// 3. Neither the name of the copyright holder nor the names of its contributors
//    may be used to endorse or promote products derived from this software without
//    specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
// IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
// INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
// NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
// WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

#ifndef BUS_BUILDER_HPP
#define BUS_BUILDER_HPP

#include "AccessorHelper.hpp"
#include "CapiAccessor.hpp"
#include "CapiError.hpp"
#include "ModelTraits.hpp"

#include "rtw_capi.h"

#include <algorithm>
#include <string>

namespace db
{
namespace simulink
{

template <typename CapiElement>
class BusBuilder;

using BlockParameterBusBuilder = BusBuilder<rtwCAPI_BlockParameters>;
using ModelParameterBusBuilder = BusBuilder<rtwCAPI_ModelParameters>;
using StateBusBuilder = BusBuilder<rtwCAPI_States>;
using SignalBusBuilder = BusBuilder<rtwCAPI_Signals>;

template <typename CapiElement>
class BusBuilder
{
    static_assert(is_capi_element<CapiElement>(), "Invalid C-API Element!");

public:
    BusBuilder(rtwCAPI_ModelMappingInfo& MMI, const std::string& PathAndName)
        : mElementMap(GetRawData<rtwCAPI_ElementMap>(MMI))
        , mDataTypeMap(GetRawData<rtwCAPI_DataTypeMap>(MMI))
        , mElement(nullptr)
    {
        // Get the actual element.
        CapiAccessor<CapiElement> Accessor(MMI);
        mData = Accessor.template ptr<void>(PathAndName);

        // Find the C-API Element. It contains required meta-information.
        const auto NumElements { db::simulink::GetCount<CapiElement>(MMI) };
        auto Data { db::simulink::GetRawData<CapiElement>(MMI) };

        // Search for the Element.
        mElement = std::find_if(Data, Data + NumElements, [&](const CapiElement& Element) {
            std::string Name { db::simulink::GetName<CapiElement>(MMI, Element) };
            return Name == PathAndName;
        });

        // Element found?
        if (mElement == (Data + NumElements))
        {
            // CapiError Error { CapiError::NotFound };
            throw std::runtime_error("Element not found.");
        }
    }

    template <typename T>
    T* ptr(const std::string MemberName)
    {
        T* Result {};

        auto Element { std::find_if(begin(), end(), [MemberName](auto& e) {
            return MemberName == e.elementName;
        }) };

        if (Element != end())
        {
            auto Offset { Element->elementOffset };
            unsigned char* Pointer { reinterpret_cast<unsigned char*>(mData) };
            Pointer += Offset;
            Result = reinterpret_cast<T*>(Pointer);
        }

        return Result;
    }

    template <typename T>
    T& get(const std::string MemberName)
    {
        auto Result { ptr<T>(MemberName) };
        if (Result == nullptr)
        {
            // CapiError Error { CapiError::NotFound };
            throw std::runtime_error("Element not found.");
        }
        return *Result;
    }

    const rtwCAPI_ElementMap* begin()
    {
        auto DataTypeIndex { mElement->dataTypeIndex };
        rtwCAPI_DataTypeMap DataTypeEntry { mDataTypeMap[DataTypeIndex] };

        auto ElementMapIndex { DataTypeEntry.elemMapIndex };

        const rtwCAPI_ElementMap* Members { &mElementMap[ElementMapIndex] };

        return Members;
    }

    const rtwCAPI_ElementMap* end()
    {
        auto DataTypeIndex { mElement->dataTypeIndex };
        rtwCAPI_DataTypeMap DataTypeEntry { mDataTypeMap[DataTypeIndex] };

        auto ElementMapIndex { DataTypeEntry.elemMapIndex };

        auto NumMembers { DataTypeEntry.numElements };
        const rtwCAPI_ElementMap* Members { &mElementMap[ElementMapIndex] };

        return Members + NumMembers;
    }

private:
    void* mData;
    const rtwCAPI_ElementMap* mElementMap;
    const rtwCAPI_DataTypeMap* mDataTypeMap;
    const CapiElement* mElement;
};
} // namespace simulink
} // namespace db
#endif