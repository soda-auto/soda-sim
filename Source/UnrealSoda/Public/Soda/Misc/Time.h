// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include <chrono>
#include <string>

typedef std::chrono::time_point<std::chrono::system_clock> TTimestamp;

namespace soda 
{
	template <
		typename Clock = std::chrono::system_clock,
		typename Units = std::chrono::nanoseconds
	>
	inline auto ChronoTimestamp(typename Clock::rep u) noexcept
	{
		using namespace std::chrono;
		using TPoint = time_point<Clock>;
		using TDuration = typename Clock::duration;
		return TPoint(duration_cast<TDuration>(Units(u)));
	}
	template < typename Units = std::chrono::milliseconds, typename TimePoint = std::chrono::system_clock::time_point >
	inline auto RawTimestamp(TimePoint time)
	{
		return std::chrono::duration_cast<Units>(time.time_since_epoch()).count();
	}

	template < typename Clock = std::chrono::system_clock, typename Units = std::chrono::milliseconds >
	inline auto NowRaw()
	{
		return RawTimestamp< Units >(Clock::now());
	}
	template < typename Clock = std::chrono::system_clock>
	inline auto Now()
	{
		return Clock::now();
	}

	inline FString ToString(int64 Value)
	{
		return std::to_string(Value).c_str();
	}

	template < typename Units = std::chrono::milliseconds >
	inline FString ToString(const TTimestamp& Timestamp)
	{
		return soda::ToString(int64(soda::RawTimestamp<Units>(Timestamp)));
	}
}