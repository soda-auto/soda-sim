// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include <inttypes.h>
#include <string.h>
#include <math.h>

#define J1939_MASK_8        0xFFU
#define J1939_FLAG_8_ERR    0xFEU
#define J1939_FLAG_8_NA     0xFFU
#define J1939_VALUE_8_ERR   0xFEU
#define J1939_VALUE_8_NA    0xFFU

#define J1939_MASK_10       0x3FFU
#define J1939_FLAG_10_ERR   0x3FEU
#define J1939_FLAG_10_NA    0x3FFU
#define J1939_VALUE_10_ERR  0x3FEU
#define J1939_VALUE_10_NA   0x3FFU

#define J1939_MASK_11       0x7FFU
#define J1939_FLAG_11_ERR   0x7FEU
#define J1939_FLAG_11_NA    0x7FFU
#define J1939_VALUE_11_ERR  0x7FEU
#define J1939_VALUE_11_NA   0x7FFU

#define J1939_MASK_12       0xFFFU
#define J1939_FLAG_12_ERR   0xFFEU
#define J1939_FLAG_12_NA    0xFFFU
#define J1939_VALUE_12_ERR  0xFFEU
#define J1939_VALUE_12_NA   0xFFFU

#define J1939_MASK_13       0x1FFFU
#define J1939_FLAG_13_ERR   0x1FFEU
#define J1939_FLAG_13_NA    0x1FFFU
#define J1939_VALUE_13_ERR  0x1FFEU
#define J1939_VALUE_13_NA   0x1FFFU

#define J1939_MASK_16       0xFF00U
#define J1939_FLAG_16_ERR   0xFE00U
#define J1939_FLAG_16_NA    0xFF00U
#define J1939_VALUE_16_ERR  0xFEFFU
#define J1939_VALUE_16_NA   0xFFFFU

#define J1939_MASK_24       0xFF0000U
#define J1939_FLAG_24_ERR   0xFE0000U
#define J1939_FLAG_24_NA    0xFF0000U
#define J1939_VALUE_24_ERR  0xFEFFFFU
#define J1939_VALUE_24_NA   0xFFFFFFU

#define J1939_MASK_32       0xFF000000U
#define J1939_FLAG_32_ERR   0xFE000000U
#define J1939_FLAG_32_NA    0xFF000000U
#define J1939_VALUE_32_ERR  0xFEFFFFFFU
#define J1939_VALUE_32_NA   0xFFFFFFFFU

#define J1939_MASK_64       0xFF00000000000000LLU
#define J1939_FLAG_64_ERR   0xFE00000000000000LLU
#define J1939_FLAG_64_NA    0xFF00000000000000LLU
#define J1939_VALUE_64_ERR  0xFEFFFFFFFFFFFFFFLLU
#define J1939_VALUE_64_NA   0xFFFFFFFFFFFFFFFFLLU

#define BIT_ONE 1
#define BIT_ZERO 0

#define TO_BOOL(X) ((X)?true:false)

#define CHECK_STRUCT_SIZE(expected_size, struct_size) static_assert(expected_size <= struct_size, "Invalid structure size " #expected_size " == " #struct_size);

#if defined(__GLIBC__)
#  include <endian.h>
#  if __BYTE_ORDER == __LITTLE_ENDIAN
#    define INTEL_ORDER 1
#  endif
#endif

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32)
#  define INTEL_ORDER 1
#endif


#ifndef INTEL_ORDER
#  error Wrong byte order!
#endif

namespace Utils
{
	/*
	 * The maximum size of the bit's field (BitLength) is 56 bits.
	 */

	inline void SetBits56(void* Dest, const void* Src, size_t BitOffset, size_t BitLength)
	{
		uint8_t Off = BitOffset % 8;
		uint64_t Mask = ((0xFFFFFFFFFFFFFFFF >> (64 - BitLength))) << Off;
		uint64_t& Ref = *(uint64_t*)&((uint8_t*)Dest)[BitOffset / 8];
		Ref &= ~Mask;
		Ref |= ((*(uint64_t*)Src) << Off) & Mask;
	}

	/*
	* The maximum size of the bit's field (BitLength) is 56 bits.
	*/
	template<class T>
	inline T GetBits56(const void* Src, size_t BitOffset, size_t BitLength)
	{
		uint8_t Off = BitOffset % 8;
		uint64_t Mask = 0xFFFFFFFFFFFFFFFF >> (64 - BitLength);
		uint64_t Tmp = ((*(uint64_t*)&((uint8_t*)Src)[BitOffset / 8]) >> Off) & Mask;
		return *(T*)&Tmp;
	}

	inline void SetBits(void* Dest, const void* Src, size_t BitOffset, size_t BitLength)
	{
		if (BitOffset % 8 == 0 && BitLength % 8 == 0)
		{
			FMemory::Memcpy(&((uint8_t*)Dest)[BitOffset / 8], Src, BitLength / 8);
			return;
		}

		if (BitLength > 56)
		{
			check(BitLength <= 64);
			SetBits56(Dest, Src, BitOffset, 56);
			SetBits56(Dest, &((uint8_t*)Src)[7], BitOffset + 56, BitLength - 56);
		}
		else
		{
			SetBits56(Dest, Src, BitOffset, BitLength);
		}
	}

	template<class T>
	inline T GetBits(const void* Src, size_t BitOffset, size_t BitLength)
	{
		if (BitOffset % 8 == 0 && BitLength % 8 == 0)
		{
			T Ret{};
			FMemory::Memcpy(&Ret, &((uint8_t*)Src)[BitOffset / 8], BitLength / 8);
			return Ret;
		}

		if (BitLength > 56)
		{
			check(BitLength <= 64);
			uint64_t Tmp = GetBits56<uint64_t>(Src, BitOffset, 56) | (GetBits56<uint64_t>(Src, BitOffset + 56, BitLength - 56) << 56);
			return *(T*)&Tmp;
		}
		else
		{
			return GetBits56<T>(Src, BitOffset, BitLength);
		}
	}
}