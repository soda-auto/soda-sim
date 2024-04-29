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

#ifndef MODELTRAITS_HPP
#define MODELTRAITS_HPP

#include <utility>

namespace db
{
namespace simulink
{
////////////////////////////
// has_error_status
template <typename, typename = void>
struct has_error_status : std::false_type
{
};

template <typename T>
struct has_error_status<T, std::void_t<decltype(std::declval<T>().errorStatus)>> : std::true_type
{
};

template <typename T>
inline constexpr bool has_error_status_v = has_error_status<T>::value;

////////////////////////////
// has_datamapinfo
template <typename, typename = void>
struct has_datamapinfo : std::false_type
{
};

template <typename T>
struct has_datamapinfo<T, std::void_t<decltype(std::declval<T>().DataMapInfo)>> : std::true_type
{
};

template <typename T>
inline constexpr bool has_datamapinfo_v = has_datamapinfo<T>::value;

////////////////////////////
// has_childmmi
template <typename, typename = void>
struct has_childmmi : std::false_type
{
};

template <typename T>
struct has_childmmi<T, std::void_t<decltype(std::declval<T>().childMMI)>> : std::true_type
{
};

template <typename T>
inline constexpr bool has_childmmi_v = has_childmmi<T>::value;

////////////////////////////
// has_mmi
template <typename, typename = void>
struct has_mmi : std::false_type
{
};

template <typename T>
struct has_mmi<T, std::void_t<decltype(std::declval<T>().mmi)>> : std::true_type
{
};

template <typename T>
inline constexpr bool has_mmi_v = has_mmi<T>::value;

////////////////////////////
// has_block_io
template <typename, typename = void>
struct has_block_io : std::false_type
{
};

template <typename T>
struct has_block_io<T, std::void_t<decltype(std::declval<T>().blockIO)>> : std::true_type
{
};

template <typename T>
inline constexpr bool has_block_io_v = has_block_io<T>::value;

////////////////////////////
// has_instance_map
template <typename, typename = void>
struct has_instance_map : std::false_type
{
};

template <typename T>
struct has_instance_map<T, std::void_t<decltype(std::declval<T>().InstanceMap)>> : std::true_type
{
};

template <typename T>
inline constexpr bool has_instance_map_v = has_instance_map<T>::value;

}
}
#endif