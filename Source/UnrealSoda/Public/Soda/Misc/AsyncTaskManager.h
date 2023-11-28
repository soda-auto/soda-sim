// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "HAL/PlatformTime.h"
#include "HAL/PlatformProcess.h"
#include "HAL/Runnable.h"
#include "HAL/ThreadSafeBool.h"
#include "HAL/Event.h"
#include "Async/Future.h"
#include "Misc/SingleThreadRunnable.h"
#include "Templates/SharedPointer.h"

namespace soda 
{

class FAsyncTaskManager;

/* ************************************************************************************
 * FAsyncTask
 *************************************************************************************/
class FAsyncTask
{
public:

	FAsyncTask()
	{
		StartTime = FPlatformTime::Seconds();
	}

	double GetElapsedTime()
	{
		return FPlatformTime::Seconds() - StartTime;
	}

	virtual ~FAsyncTask() {}
	virtual FString ToString() const = 0;
	virtual void Initialize() {}
	virtual bool IsDone() const = 0;
	virtual bool WasSuccessful() const = 0;
	virtual void Tick() = 0;

protected:
	double StartTime;
};

/* ************************************************************************************
 * FDoubleBufferAsyncTask
 *************************************************************************************/
template <class T>
class FDoubleBufferAsyncTask : public FAsyncTask
{
public:
	FDoubleBufferAsyncTask()
	{
		FrontTask = MakeShareable(new T());
		BackTask = MakeShareable(new T());
	}

	virtual FString ToString() const override { return BackTask->ToString(); }
	virtual bool IsDone() const override { return bIsDone; }
	void Start() { bIsDone = false; }
	void Finish() { bIsDone = true; }
	virtual bool WasSuccessful() const override { return true; }

	virtual void Tick() override
	{
		if (!BackTask->IsDone())
		{
			BackTask->Tick();
		}

		if (BackTask->IsDone() && !FrontTask->IsDone())
		{
			FScopeLock ScopeLock(&TasksLock);
			std::swap(FrontTask, BackTask);
		}

		if (!BackTask->IsDone())
		{
			BackTask->Tick();
		}
	}

	TSharedRef <T> LockFrontTask()
	{
		TasksLock.Lock();
		return FrontTask.ToSharedRef();
	}

	void UnlockFrontTask() { TasksLock.Unlock(); }

	T& GetLockedFrontTask() { return *FrontTask; }


protected:
	FCriticalSection TasksLock;
	TSharedPtr<T> FrontTask;
	TSharedPtr<T> BackTask;
	bool bIsDone = false;
};

/* ************************************************************************************
 * FAsyncTaskManager
 *************************************************************************************/

class UNREALSODA_API FAsyncTaskManager : public FRunnable, FSingleThreadRunnable 
{
public:

	FAsyncTaskManager(uint32 InPollingInterval = 50):
		PollingInterval(InPollingInterval)
	{}
	virtual ~FAsyncTaskManager() 
	{}

	virtual bool Init() override;
	virtual uint32 Run() override;
	virtual void Stop() override;
	virtual void Exit() override;
	virtual class FSingleThreadRunnable* GetSingleThreadInterface() override { return this; }

	template<class T>
	bool AddTask(TSharedPtr <T>& NewTask)
	{
		TSharedPtr<FAsyncTask> TaskCast = StaticCastSharedPtr<FAsyncTask>(NewTask);
		if (TaskCast)
		{
			NewTask->Initialize();
			FScopeLock ScopeLock(&TasksLock);
			TaskQueue.Add(TaskCast);
			return true;
		}
		return false;
	}

	template<class T>
	bool RemoteTask(TSharedPtr <T>& Task, bool Sync = true)
	{
		TSharedPtr<FAsyncTask> TaskCast = StaticCastSharedPtr<FAsyncTask>(Task);
		int Removed = 0;
		if (Sync)
		{
			FScopeLock ScopeLock(&TickLock);
			Removed = TaskQueue.Remove(TaskCast);
		}
		else
		{
			FScopeLock ScopeLock(&TasksLock);
			Removed = TaskQueue.Remove(TaskCast);
		}
		/*
		if (Removed < 1)
		{
			UE_LOG(LogSoda, Warning, TEXT("FAsyncTaskManager::RemoteTask() Can't find task for remove"));
		}
		*/

		return Removed > 0;
	}

	void Trigger() { WorkEvent->Trigger(); }

	void ClearQueue();

	FCriticalSection TasksLock;
	FCriticalSection TickLock;

protected:
	virtual void Tick() override;

	FEvent* WorkEvent = nullptr;
	uint32 PollingInterval;
	FThreadSafeBool bRequestingExit = false;
	TArray< TSharedPtr <FAsyncTask>> TaskQueue; // TODO: Use TQueue without TasksLock
};

UNREALSODA_API TSharedPtr<FAsyncTask> AsyncTaskLoop(FAsyncTaskManager& AsyncTaskManager, TUniqueFunction < void()> Function);
UNREALSODA_API TFuture<bool> AsyncTask(FAsyncTaskManager& AsyncTaskManager, TUniqueFunction < bool()> Function, bool bSync = false);

} // namespace soda


