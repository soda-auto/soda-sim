// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/Misc/AsyncTaskManager.h"
#include "Soda/UnrealSoda.h"
#include "Misc/ScopeLock.h"

namespace soda
{

bool FAsyncTaskManager::Init(void)
{
	WorkEvent = FPlatformProcess::GetSynchEventFromPool();
	return WorkEvent != nullptr;
}

uint32 FAsyncTaskManager::Run(void)
{
	do
	{
		WorkEvent->Wait(PollingInterval);
		if (!bRequestingExit)
		{
			FScopeLock ScopeLock(&TickLock);
			Tick();
		}
	} while (!bRequestingExit);

	return 0;
}

void FAsyncTaskManager::Stop(void)
{
	bRequestingExit = true;
	Trigger();
}

void FAsyncTaskManager::Exit(void)
{
	FPlatformProcess::ReturnSynchEventToPool(WorkEvent);
	WorkEvent = nullptr;

	//FScopeLock ScopeLock(&TasksLock);
	//TaskQueue.Empty();
}

void FAsyncTaskManager::Tick()
{
	{
		// Tick all the parallel tasks - Tick unrelated tasks together. 
		TArray< TSharedPtr <FAsyncTask>> CopyTaskQueue;

		// Grab a copy of the parallel list
		{
			FScopeLock ScopeLock(&TasksLock);
			CopyTaskQueue = TaskQueue;
		}

		TSharedPtr <FAsyncTask> Task = nullptr;
		for (auto It = CopyTaskQueue.CreateIterator(); It; ++It)
		{
			Task = *It;

			if (!Task->IsDone())
			{
				//UE_LOG(LogSoda, Warning, TEXT("Start Tick %s %i"), *Task->ToString(), Task->IsDone());
				Task->Tick();
				//UE_LOG(LogSoda, Warning, TEXT("End Tick %s %i"), *Task->ToString(), Task->IsDone());
			}

			if (Task->IsDone())
			{
				if (Task->WasSuccessful())
				{
					UE_LOG(LogSoda, Verbose, TEXT("Async task '%s' succeeded in %f seconds"), *Task->ToString(), Task->GetElapsedTime());
				}
				else
				{
					UE_LOG(LogSoda, Warning, TEXT("Async task '%s' failed in %f seconds"), *Task->ToString(), Task->GetElapsedTime());
				}

				FScopeLock ScopeLock(&TasksLock);
				TaskQueue.Remove(Task);
			}
		}
	}
}

void FAsyncTaskManager::ClearQueue() 
{ 
	FScopeLock ScopeLock(&TickLock); 
	TaskQueue.Empty(); 
}

TSharedPtr<FAsyncTask> AsyncTaskLoop(FAsyncTaskManager& AsyncTaskManager, TUniqueFunction < void()> Function)
{
	class FFunctionAsyncTask : public FAsyncTask
	{
	public:
		FFunctionAsyncTask(TUniqueFunction < void()> InFunction) :
			Function(MoveTemp(InFunction))
		{
		}

		virtual FString ToString() const override { return "FFunctionAsyncTask"; }
		virtual bool IsDone() const override { return bIsDone; }
		virtual bool WasSuccessful() const override { return true; }
		virtual void Tick() override
		{
			Function();
		}

	protected:
		bool bIsDone = false;
		TUniqueFunction < void()> Function;
	};

	TSharedPtr<FFunctionAsyncTask> Task = MakeShared<FFunctionAsyncTask>(MoveTemp(Function));
	AsyncTaskManager.AddTask(Task);
	AsyncTaskManager.Trigger();
	return Task;
}


TFuture<bool> AsyncTask(FAsyncTaskManager& AsyncTaskManager, TUniqueFunction < bool()> Function, bool bSync)
{
	if (bSync)
	{
		TPromise<bool> Promise;
		Promise.SetValue(Function());
		return Promise.GetFuture();
	}
	else
	{
		class FTempAsyncTask : public FAsyncTask
		{
		public:
			FTempAsyncTask(TUniqueFunction < bool()> InFunction) :
				Promise(MakeShareable(new TPromise<bool>())),
				Function(MoveTemp(InFunction))
			{
			}
			virtual ~FTempAsyncTask()
			{
				if (!bIsDone) Promise->SetValue(false);
			}

			virtual FString ToString() const override { return "FTempAsyncTask"; }
			virtual bool IsDone() const override { return bIsDone; }
			virtual bool WasSuccessful() const override { return true; }
			virtual void Tick() override
			{
				Promise->SetValue(Function());
				bIsDone = true;
			}

			const TSharedRef<TPromise<bool>, ESPMode::ThreadSafe> Promise;

		protected:
			bool bIsDone = false;
			TUniqueFunction < bool ()> Function;

		};

		TSharedPtr<FTempAsyncTask> Task = MakeShared<FTempAsyncTask>(MoveTemp(Function));
		AsyncTaskManager.AddTask(Task);
		AsyncTaskManager.Trigger();
		return Task->Promise->GetFuture();
	}
}

} // namespace spda