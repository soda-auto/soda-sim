// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/SodaApp.h"
#include "Soda/UnrealSoda.h"
#include "Soda/SodaSubsystem.h"
#include "Soda/Vehicles/SodaVehicle.h"
#include "Soda/SodaUserSettings.h"
#include "Soda/DBC/Serialization.h"
#include "HAL/RunnableThread.h"
#include "PhysicsEngine/PhysicsSettings.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "UI/SOutputLog.h"
#include "Misc/App.h"
#include "UObject/Package.h"

// Http server
#include "HttpServerModule.h"
#include "IHttpRouter.h"
#include "HttpServerRequest.h"
#include "HttpRequestHandler.h"
#include "HttpServerConstants.h"
#include "HttpServerResponse.h"
#include "IWebRemoteControlModule.h"
#include "RemoteControlSettings.h"

#include <thread>

FSodaApp::FSodaApp()
{
}

FSodaApp::~FSodaApp()
{
	Release();
}

void FSodaApp::Initialize()
{
	check(!bInitialized);

	if (!ZmqCtx) ZmqCtx = new zmq::context_t();

	OnPreTickHandle = FWorldDelegates::OnWorldTickStart.AddRaw(this, &FSodaApp::OnPreTick);
	OnPostTickHandle = FWorldDelegates::OnWorldPostActorTick.AddRaw(this, &FSodaApp::OnPostTick);
	//FWorldDelegates::OnPostWorldInitialization.AddRaw(this, &FSodaApp::PostWorldInitialization);

	CamTaskManagerThread = FRunnableThread::Create(&CamTaskManager, TEXT("CamTaskManager"));
	if (CamTaskManagerThread == nullptr)
	{
		UE_LOG(LogSoda, Fatal, TEXT("Can't create TaskManagerThread"));
	}

	EthTaskManagerThread = FRunnableThread::Create(&EthTaskManager, TEXT("EthTaskManager"));
	if (EthTaskManagerThread == nullptr)
	{
		UE_LOG(LogSoda, Fatal, TEXT("Can't create EthTaskManager"));
	}

	int HttpServerPort = GetDefault<URemoteControlSettings>()->RemoteControlHttpServerPort;
	if (!HttpRouter)
	{
		HttpRouter = FHttpServerModule::Get().GetHttpRouter(HttpServerPort);
	}

	/*
	if (TSharedPtr<IHttpRouter> HttpRouter = FHttpServerModule::Get().GetHttpRouter(30010))
	{
		HttpRouter->BindRoute(
			FHttpPath(TEXT("/test")),
			EHttpServerRequestVerbs::VERB_PUT,
			[this](const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete)
			{
				UE_LOG(LogSoda, Error, TEXT("*******1"));
				std::this_thread::sleep_for(std::chrono::milliseconds(5000));
				OnComplete(FHttpServerResponse::Ok());
				UE_LOG(LogSoda, Error, TEXT("*******2"));
				return true;
			}
		);
		FHttpServerModule::Get().StartAllListeners();
	}
	*/

	IWebRemoteControlModule& WebRemoteControlModule = FModuleManager::LoadModuleChecked< IWebRemoteControlModule >("WebRemoteControl");
	OnHttpServerStartedHandle = WebRemoteControlModule.OnHttpServerStarted().AddRaw(this, &FSodaApp::OnHttpServerStarted);
	OnHttpServerStoppedHandle = WebRemoteControlModule.OnHttpServerStopped().AddRaw(this, &FSodaApp::OnHttpServerStopped);

	OutputLogHistory = MakeShareable(new soda::FOutputLogHistory);

	bInitialized = true;
}

void FSodaApp::Release()
{
	if (bInitialized)
	{
		bInitialized = false;

		if (IWebRemoteControlModule* WebRemoteControlModule = FModuleManager::GetModulePtr< IWebRemoteControlModule >("WebRemoteControlModule"))
		{
			if (OnHttpServerStartedHandle.IsValid()) WebRemoteControlModule->OnHttpServerStarted().Remove(OnHttpServerStartedHandle);
			if (OnHttpServerStoppedHandle.IsValid()) WebRemoteControlModule->OnHttpServerStopped().Remove(OnHttpServerStoppedHandle);
		}

		FWorldDelegates::OnWorldTickStart.Remove(OnPreTickHandle);
		FWorldDelegates::OnWorldPostActorTick.Remove(OnPostTickHandle);

		CamTaskManagerThread->Kill();
		delete CamTaskManagerThread;

		EthTaskManagerThread->Kill();
		delete EthTaskManagerThread;

		if (ZmqCtx) delete ZmqCtx;

		OutputLogHistory.Reset();
	}
}

void FSodaApp::OnPreTick(UWorld* World, ELevelTick TickType, float DeltaSeconds)
{
	if (GameWorld == World && TickType == ELevelTick::LEVELTICK_All)
	{
		++FrameIndex;
		if(bSynchronousMode)
		{
			TotalTime += DeltaSeconds;
			SimulationTimestamp = soda::ChronoTimestamp<std::chrono::system_clock, std::chrono::duration<double>>(TotalTime);

			while (bSynchronousMode && TickCuesReceived <= 0)
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(1));
				FHttpServerModule::Get().Tick(0.1);
			} 
			--TickCuesReceived;
			RealtimeTimestamp = std::chrono::system_clock::now();
		}
		else
		{
			RealtimeTimestamp = SimulationTimestamp = std::chrono::system_clock::now();
		}
	}
}

void FSodaApp::OnPostTick(UWorld* /*World*/, ELevelTick TickType, float DeltaSeconds)
{
}

void FSodaApp::SetSynchronousMode(bool bEnable, double DeltaSeconds)
{
	FApp::SetBenchmarking(bEnable);
	FApp::SetFixedDeltaTime(DeltaSeconds);
	FixedDeltaSeconds = DeltaSeconds;
	UPhysicsSettings* PhysSett = UPhysicsSettings::Get();
	PhysSett->bSubstepping = !bEnable;
	bSynchronousMode = bEnable;

	if (IsValid(GameWorld))
	{
		TArray<AActor*> FoundVehicles;
		UGameplayStatics::GetAllActorsOfClass(GameWorld, ASodaVehicle::StaticClass(), FoundVehicles);

		for (auto & it : FoundVehicles)
		{
			ASodaVehicle* Vehicle = Cast<ASodaVehicle>(it);
			if (IsValid(Vehicle))
			{
				Vehicle->RespawnVehcile();
			}
		}
	}
	else
	{
		UE_LOG(LogSoda, Error, TEXT("FSodaApp::SetSynchronousMode(), forgot to call FSodaApp::NotifyInitGame() ?"));
	}
}

void FSodaApp::NotifyInitGame(USodaSubsystem* InSodaSubsystem)
{
	check(IsValid(InSodaSubsystem) && IsValid(InSodaSubsystem->GetWorld()));
	GameWorld = InSodaSubsystem->GetWorld();
	SetSodaSubsystem(InSodaSubsystem);

}

void FSodaApp::NotifyEndGame()
{
	ResetSodaSubsystem();
	GameWorld = nullptr;
}

bool FSodaApp::SynchTick(int64& TimestampMs, int& InFramIndex)
{
	if (bSynchronousMode && TickCuesReceived <= 0)
	{
		++TickCuesReceived;
		TimestampMs = soda::RawTimestamp<std::chrono::milliseconds>(SimulationTimestamp);
		InFramIndex = FrameIndex;
		return true;
	}
	else
	{
		return false;
	}
}

void FSodaApp::OnHttpServerStarted(uint32 Port)
{
	HttpRouter = FHttpServerModule::Get().GetHttpRouter(Port);
}

void FSodaApp::OnHttpServerStopped()
{
	HttpRouter.Reset();
}


USodaSubsystem* FSodaApp::GetSodaSubsystem() const
{
	check(IsInGameThread());
	return SodaSubsystem.Get();
}

USodaSubsystem* FSodaApp::GetSodaSubsystemChecked() const
{
	check(IsInGameThread());
	check(SodaSubsystem.IsValid());
	return SodaSubsystem.Get();
}

void FSodaApp::SetSodaSubsystem(USodaSubsystem* InSodaSubsystem)
{
	check(IsInGameThread());
	SodaSubsystem = InSodaSubsystem;
}

void FSodaApp::ResetSodaSubsystem()
{
	check(IsInGameThread());
	SodaSubsystem = nullptr;
}

const USodaUserSettings* FSodaApp::GetSodaUserSettings() const
{
	if (SodaUserSettings == NULL)
	{
		FSodaApp* ConstThis = const_cast<FSodaApp*>(this);	// Hack because Header Generator doesn't yet support mutable keyword
		ConstThis->CreateSodaUserSettings();
	}
	return SodaUserSettings.Get();
}

USodaUserSettings* FSodaApp::GetSodaUserSettings()
{
	if (SodaUserSettings == NULL)
	{
		CreateSodaUserSettings();
	}
	return SodaUserSettings.Get();
}

void FSodaApp::CreateSodaUserSettings()
{
	USodaUserSettings::LoadConfigIni();
	SodaUserSettings = NewObject<USodaUserSettings>(GetTransientPackage());
	SodaUserSettings->LoadSettings();
	SodaUserSettings->AddToRoot();
}

TSharedPtr<dbc::FMessageSerializator> FSodaApp::FindDBCSerializator(const FString& MessageName, const FString& Namespace)
{
	if (Namespace.IsEmpty())
	{
		for (auto& NamespaceIt : DBCPool)
		{
			if (auto MessageIt = NamespaceIt.Value.Find(MessageName))
			{
				return *MessageIt;
			}
		}
	}
	else
	{
		if (auto NamespaceIt = DBCPool.Find(Namespace))
		{
			if (auto MessageIt = NamespaceIt->Find(MessageName))
			{
				return *MessageIt;
			}
		}

	}

	return TSharedPtr<dbc::FMessageSerializator>();
}


bool FSodaApp::RegisterDBC(const FString& Namespace, const FString& FileName)
{
	UE_LOG(LogSoda, Log, TEXT("FSodaApp::RegisterDBC(\"%s\")"), *FileName);
	auto & It = DBCPool.FindOrAdd(Namespace);
	bool bRet =  dbc::LoadDBC(FileName, It);
	if (!bRet)
	{
		UE_LOG(LogSoda, Warning, TEXT("FSodaApp::RegisterDBC(); Can't load DBC: \"%s\""), *FileName);
	}
	return bRet;
}

void FSodaApp::RegisterVehicleExporter(TSharedPtr<ISodaVehicleExporter> Exporter)
{
	check(Exporter.IsValid());
	VehicleExporters.FindOrAdd(Exporter->GetExporterName()) = Exporter;
}

void FSodaApp::UnregisterVehicleExporter(const FString& ExporteName)
{
	VehicleExporters.Remove(ExporteName);
}

FSodaApp SodaApp;
