// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "HAL/UnrealMemory.h"

class FBitStream
{
public:
	TArray<uint8> Buf;

	inline void SetOffset(size_t BitOffsetIn)
	{
		Offset = BitOffsetIn;
	}

	inline size_t GetOffset() const
	{
		return Offset;
	}

	template<class T>
	inline void SetBits(T Data, size_t BitLengthIn)
	{
		SetBits(&Data, BitLengthIn);
	}

	template<class T>
	inline void SetBits(T* Data, size_t BitLengthIn)
	{
		if (BitLengthIn > 56)
		{
			check(BitLengthIn <= 64);
			SetBits56(Data, Offset, 56);
			SetBits56(&((uint8*)Data)[7], Offset, BitLengthIn - 56);
		}
		else
		{
			SetBits56(Data, Offset, BitLengthIn);
		}
	}

	template<class T>
	inline void SetBytes(T* Data, size_t Num)
	{
		check(Offset % 8 == 0);
		Buf.SetNum(Offset / 8 + Num + 1, false);
		FMemory::Memcpy(&Buf[Offset / 8], Data, Num);
		Offset += Num * 8;
	}

	template<class T>
	inline void SetBytes(T Data, size_t Num)
	{
		SetBytes(&Data, Num);
	}

	template<class T>
	inline T GetBits(size_t BitLengthIn)
	{
		if (BitLengthIn > 56)
		{
			check(BitLengthIn <= 64);
			uint64 Tmp = GetBits56<uint64>(Offset, 56) | (GetBits56<uint64>(Offset, BitLengthIn - 56) << 56);
			return *(T*)&Tmp;
		}
		else
		{
			return GetBits56<T>(Offset, BitLengthIn);
		}
	}

	inline void GetBytes(void* Dst, size_t Num)
	{
		check(Offset % 8 == 0);
		FMemory::Memcpy(Dst, &Buf[Offset / 8], Num);
		Offset += Num * 8;
	}

	template<class T>
	inline T GetBytes(size_t Num)
	{
		check(Offset % 8 == 0);
		T Ret = 0;
		FMemory::Memcpy(&Ret, &Buf[Offset / 8], Num);
		Offset += Num * 8;
		return Ret;
	}

	/*
	* The maximum size of the bit's field (BitLengthIn) is 56 bits.
	*/
	inline void SetBits56(void* Data, size_t BitOffsetIn, size_t BitLengthIn)
	{
		check(BitLengthIn <= 56);
		Buf.SetNum((BitOffsetIn + BitLengthIn) / 8 + 1, false);
		uint8 Off = BitOffsetIn % 8;
		uint64 Mask = 0xFFFFFFFFFFFFFFFF >> (64 - BitLengthIn - Off);
		uint64& Ref = *(uint64*)&Buf[BitOffsetIn / 8];
		Ref &= Mask; //Clear;
		Ref |= ((*(uint64*)Data) << Off) & Mask;
		Offset = BitOffsetIn + BitLengthIn;
	}

	/*
	* The maximum size of the bit's field (BitLengthIn) is 56 bits.
	*/
	template<class T>
	inline T GetBits56(size_t BitOffsetIn, size_t BitLengthIn)
	{
		check(BitLengthIn <= 56);
		uint8 Off = BitOffsetIn % 8;
		uint64 Mask = 0xFFFFFFFFFFFFFFFF >> (64 - BitLengthIn);
		uint64 Tmp = ((*(uint64*)&Buf[BitOffsetIn / 8]) >> Off)& Mask;
		Offset = BitOffsetIn + BitLengthIn;
		return *(T*)&Tmp;
	}

protected:
	volatile size_t Offset = 0;
/*
public:

	static bool CompareBits(void* A, void* B, size_t Len)
	{
		for (uint64 i = 0; i < Len; ++i)
		{
			if (((*(uint64*)A) & (1ull << i)) != ((*(uint64*)B) & (1ull << i))) return false;
		}

		return true;
	}

	static void BitTest()
	{
		FBitStream Stream;
		const int BitsNum = 64;
		uint64 Randbuf[BitsNum * 4];

		for (int i = 0; i < BitsNum; ++i)
		{
			Randbuf[i * 4 + 0] = 0xFFFFFFFFFFFFFFFF;
			Randbuf[i * 4 + 1] = 0X55AA55AA55AA55AA;
			Randbuf[i * 4 + 2] = 0XAA55AA55AA55AA55;
			Randbuf[i * 4 + 3] = 0x123456789ABCDEF0;
		}

		for (int i = 0; i < BitsNum; ++i)
		{
			Stream.SetBits(&Randbuf[i * 4 + 0], i + 1);
			Stream.SetBits(&Randbuf[i * 4 + 1], i + 1);
			Stream.SetBits(&Randbuf[i * 4 + 2], i + 1);
			Stream.SetBits(&Randbuf[i * 4 + 3], i + 1);
		}

		Stream.SetOffset(0);

		for (int i = 0; i < BitsNum; ++i)
		{
			uint64 Val = Stream.GetBits<uint64>(i + 1);
			UE_LOG(LogSoda, Warning, TEXT("BitTest[%i]: %i; 0x%llx  0x%llx"), i, CompareBits(&Val, &Randbuf[i * 4 + 0], i + 1), Val, Randbuf[i * 4 + 0]);

			Val = Stream.GetBits<uint64>(i + 1);
			UE_LOG(LogSoda, Warning, TEXT("BitTest[%i]: %i; 0x%llx  0x%llx"), i, CompareBits(&Val, &Randbuf[i * 4 + 1], i + 1), Val, Randbuf[i * 4 + 1]);

			Val = Stream.GetBits<uint64>(i + 1);
			UE_LOG(LogSoda, Warning, TEXT("BitTest[%i]: %i; 0x%llx  0x%llx"), i, CompareBits(&Val, &Randbuf[i * 4 + 2], i + 1), Val, Randbuf[i * 4 + 2]);

			Val = Stream.GetBits<uint64>(i + 1);
			UE_LOG(LogSoda, Warning, TEXT("BitTest[%i]: %i; 0x%llx  0x%llx"), i, CompareBits(&Val, &Randbuf[i * 4 + 3], i + 1), Val, Randbuf[i * 4 + 3]);
		}
	}
*/
};



