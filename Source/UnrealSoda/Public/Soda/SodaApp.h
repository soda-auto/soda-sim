// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "CoreGlobals.h"
#if PLATFORM_WINDOWS
#include "Windows/AllowWindowsPlatformTypes.h"
#endif
#include <zmq.hpp>
#if PLATFORM_WINDOWS
#include "Windows/HideWindowsPlatformTypes.h"
#endif
#include "Engine/EngineBaseTypes.h"
#include "GameFramework/WorldSettings.h"
#include "Soda/Misc/LLConverter.h"
#include "Soda/Misc/AsyncTaskManager.h"
#include "Soda/Misc/Utils.h"
#include "Soda/Misc/Time.h"
#include "Soda/Vehicles/ISodaVehicleExporter.h"
#include "JsonObjectWrapper.h"
#include <cmath>

class IHttpRouter;
class USodaSubsystem;
class USodaUserSettings;

namespace dbc
{
	class FMessageSerializator;
}

namespace soda
{
	class FOutputLogHistory;
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

	TSharedPtr<IHttpRouter> & GetHttpRouter() { return HttpRouter; }

	USodaSubsystem* GetSodaSubsystem() const;
	USodaSubsystem* GetSodaSubsystemChecked() const;

	const USodaUserSettings* GetSodaUserSettings() const;
	USodaUserSettings* GetSodaUserSettings();

	TSharedPtr<dbc::FMessageSerializator> FindDBCSerializator(const FString& MessageName, const FString& Namespace = FString());
	bool RegisterDBC(const FString& Namespace, const FString& FileName);

	void RegisterVehicleExporter(TSharedPtr<ISodaVehicleExporter> Exporter);
	void UnregisterVehicleExporter(const FString& ExporteName);
	const TMap<FString, TSharedPtr<ISodaVehicleExporter>>& GetVehicleExporters() const { return VehicleExporters; }

	TSharedPtr<soda::FOutputLogHistory> GetOutputLogHistory() { return OutputLogHistory; }

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

	void CreateSodaUserSettings();

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

	TWeakObjectPtr<USodaUserSettings> SodaUserSettings;

	TMap<FString, TMap<FString, TSharedPtr<dbc::FMessageSerializator>>> DBCPool;

	TMap<FString, TSharedPtr<ISodaVehicleExporter>> VehicleExporters;

	TSharedPtr<soda::FOutputLogHistory> OutputLogHistory;
};

extern UNREALSODA_API class FSodaApp SodaApp;