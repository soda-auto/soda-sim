// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "CoreGlobals.h"
#include "Engine/EngineBaseTypes.h"
#include "Soda/Misc/AsyncTaskManager.h"
#include "Soda/Misc/Time.h"
#include "Templates/IsValidVariadicFunctionArg.h"

class IHttpRouter;
class USodaSubsystem;

namespace dbc
{
	class FMessageSerializator;
}

namespace soda
{
	class FOutputLogHistory;
	class FFileDatabaseManager;
	class IDatasetManager;
	class ISodaVehicleExporter;
}

namespace zmq
{
	class context_t;
}

class UNREALSODA_API FSodaApp
{
public:
	FSodaApp();
	~FSodaApp();

	void Initialize();
	void Release();
	bool IsInitialized() const { return bInitialized; }

public:
	inline zmq::context_t* GetZmqContext() { return ZmqCtx; }
	inline TTimestamp GetSimulationTimestamp() const { return SimulationTimestamp; }
	inline TTimestamp GetRealtimeTimestamp() const { return RealtimeTimestamp; }

	void SetSynchronousMode(bool bEnable, double DeltaSeconds);
	void NotifyInitGame(USodaSubsystem* InSodaSubsystem);
	void NotifyEndGame();
	inline bool IsSynchronousMode() const { return bSynchronousMode; }
	inline int GetFrameIndex() const { return FrameIndex; }

	/** One tick simulation in the synchronous mode */
	bool SynchTick(int64& TimestampMs, int& FramIndex);

	TSharedPtr<IHttpRouter>& GetHttpRouter() { return HttpRouter; }

	USodaSubsystem* GetSodaSubsystem() const;
	USodaSubsystem* GetSodaSubsystemChecked() const;

	TSharedPtr<dbc::FMessageSerializator> FindDBCSerializator(const FString& MessageName, const FString& Namespace = FString());
	bool RegisterDBC(const FString& Namespace, const FString& FileName);

	void RegisterVehicleExporter(TSharedRef<soda::ISodaVehicleExporter> Exporter);
	void UnregisterVehicleExporter(const FName& ExporteName);
	const TMap<FName, TSharedPtr<soda::ISodaVehicleExporter>>& GetVehicleExporters() const { return VehicleExporters; }

	void RegisterDatasetManager(FName DatasetName, TSharedRef<soda::IDatasetManager> DatasetManager);
	void UnregisterDatasetManager(FName DatasetName);
	const TMap<FName, TSharedPtr<soda::IDatasetManager>>& GetDatasetManagers() const { return DatasetManagers; }

	soda::FOutputLogHistory & GetOutputLogHistory() { return *OutputLogHistory.Get(); }
	soda::FFileDatabaseManager & GetFileDatabaseManager() { return *FileDatabaseManager.Get(); }

public:
	soda::FAsyncTaskManager CamTaskManager;
	soda::FAsyncTaskManager EthTaskManager;

protected:
	// TODO: User ThreadPool insted
	FRunnableThread* CamTaskManagerThread = nullptr;

	// TODO: User ThreadPool insted
	FRunnableThread* EthTaskManagerThread = nullptr;

	FDelegateHandle OnPreTickHandle;
	FDelegateHandle OnPostTickHandle;

	FDelegateHandle OnHttpServerStartedHandle;
	FDelegateHandle OnHttpServerStoppedHandle;

	void OnPreTick(UWorld* World, ELevelTick TickType, float DeltaSeconds);
	void OnPostTick(UWorld* World, ELevelTick TickType, float DeltaSeconds);

	void OnHttpServerStarted(uint32 Port);
	void OnHttpServerStopped();

	void SetSodaSubsystem(USodaSubsystem* InSodaSubsystem);
	void ResetSodaSubsystem();

	zmq::context_t* ZmqCtx = nullptr;

	// Current GameWorld
	UWorld* GameWorld = nullptr;

	//TArray<TSharedPtr<FSyncClient>> SyncClients;
	bool bSynchronousMode = false;
	float FixedDeltaSeconds = 0;
	int TickCuesReceived = 0;
	int FrameIndex = 0;
	TTimestamp RealtimeTimestamp;
	TTimestamp SimulationTimestamp;
	double TotalTime = 0;

	bool bWaitTick = false;

	/* HTTP */
	TSharedPtr<IHttpRouter> HttpRouter;

	TWeakObjectPtr<USodaSubsystem> SodaSubsystem;
	bool bInitialized = false;

	TMap<FString, TMap<FString, TSharedPtr<dbc::FMessageSerializator>>> DBCPool;

	TMap<FName, TSharedPtr<soda::ISodaVehicleExporter>> VehicleExporters;
	TMap<FName, TSharedPtr<soda::IDatasetManager>> DatasetManagers;

	TSharedPtr<soda::FOutputLogHistory> OutputLogHistory;
	TSharedPtr<soda::FFileDatabaseManager> FileDatabaseManager;
};

enum class ENotificationLevel
{
	Success,
	Warning,
	Error
};

namespace soda
{
	UNREALSODA_API void ShowNotificationImpl(ENotificationLevel Level, double Duration, bool bLog, const TCHAR* FunctionName, const TCHAR* Fmt, ...);

	template <typename... Types>
	void ShowNotification(ENotificationLevel Level, double Duration, const TCHAR * Fmt, Types... Args)
	{
		static_assert((TIsValidVariadicFunctionArg<Types>::Value && ...), "Invalid argument(s) passed to Printf");
		ShowNotificationImpl(Level, Duration, false, TEXT(""), Fmt, Args...);
	}

	template <typename... Types>
	void ShowNotificationAndLog(ENotificationLevel Level, double Duration, const TCHAR* FunctionName, const TCHAR* Fmt, Types... Args)
	{
		static_assert((TIsValidVariadicFunctionArg<Types>::Value && ...), "Invalid argument(s) passed to Printf");
		ShowNotificationImpl(Level, Duration, true, FunctionName, Fmt, Args...);
	}
}

extern UNREALSODA_API class FSodaApp SodaApp;