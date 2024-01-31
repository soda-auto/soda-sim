// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include <thread>
#include <chrono>

DECLARE_DELEGATE_TwoParams(FPrecisionTimerDelegate, const std::chrono::nanoseconds &, const  std::chrono::nanoseconds &);

class FPrecisionTimer
{
public:
	virtual ~FPrecisionTimer()
	{
		TimerStop();
	}

	template <class _Rep1, class _Period1, class _Rep2, class _Period2>
	void TimerStart(const std::chrono::duration<_Rep1, _Period1>& Deltatime, const std::chrono::duration<_Rep2, _Period2> & StartupDelay)
	{
		if (!TimerThread.joinable())
		{
			bIsTimerThreadWorking = true;
			TimerThread = std::thread([this, Deltatime, StartupDelay]() {
				std::this_thread::sleep_for(StartupDelay);
				auto SleepUntil = std::chrono::high_resolution_clock::now();
				while (bIsTimerThreadWorking)
				{
					SleepUntil += Deltatime;
					auto Elapsed = std::chrono::high_resolution_clock::now() - SleepUntil;
					if (Elapsed > std::chrono::nanoseconds::zero())
					{
						Elapsed = std::chrono::nanoseconds(
							int64(double(std::chrono::duration_cast<std::chrono::nanoseconds>(Elapsed).count()) * RealtimeSmoothFactor));
						SleepUntil += Elapsed;
					}
					else
					{
						std::this_thread::sleep_until(SleepUntil);
					}
					TimerTick(Deltatime, Elapsed);
				}
			});
		}
	}

	void TimerStop()
	{
		bIsTimerThreadWorking = false;
		if (TimerThread.joinable())
			TimerThread.join();
	}

	bool IsTimerWorking() const
	{
		return bIsTimerThreadWorking;
	}

	bool IsJoinable() const
	{
		return TimerThread.joinable();
	}

	virtual void TimerTick(const std::chrono::nanoseconds & Deltatime, const std::chrono::nanoseconds & Elapsed)
	{
		TimerDelegate.ExecuteIfBound(Deltatime, Elapsed);
	}


	FPrecisionTimerDelegate TimerDelegate;
	double RealtimeSmoothFactor = 1.0;

private:
	std::thread TimerThread;
	bool bIsTimerThreadWorking;
	std::chrono::high_resolution_clock::time_point PrevTimePoint;
};