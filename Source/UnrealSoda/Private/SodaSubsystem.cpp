// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/SodaSubsystem.h"
#include "Soda/UnrealSoda.h"
#include "Soda/SodaApp.h"
#include "Soda/LevelState.h"
#include "Soda/FileDatabaseManager.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "GameFramework/GameModeBase.h"
#include "Kismet/KismetMathLibrary.h"
#include "Misc/FileHelper.h"
#include "EngineUtils.h"
#include "Soda/SodaSpectator.h"
#include "TimerManager.h"
#include "Engine/LevelStreamingPersistent.h"
#include "Soda/Vehicles/SodaVehicle.h"
#include "Soda/SodaStatics.h"
#include "Components/PanelWidget.h"
#include "Engine/AssetManager.h"
#include "Soda/Misc/SerializationHelpers.h"
#include "Soda/UI/SMessageBox.h"
#include "Soda/UI/SMenuWindow.h"
#include "Soda/UI/SSodaViewport.h"
#include "UI/Wnds/SSaveAllWindow.h"
#include "UI/Wnds/SQuickStartWindow.h"
#include "Soda/UI/SToolBox.h"
#include "Soda/UI/SWaitingPanel.h"
#include "Soda/SodaGameViewportClient.h"
#include "Soda/ISodaActor.h"
#include "Soda/SodaLibraryPrimaryAsset.h"
#include "RuntimeMetaData.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "Soda/SodaCommonSettings.h"
#include "UObject/UObjectIterator.h"
#include "Async/Async.h"
#include "Soda/SodaActorFactory.h"
#include "Soda/Editor/SodaSelection.h"
#include "Engine/GameInstance.h"
#include "Engine/Engine.h"

USodaSubsystem::FLevelTransientData USodaSubsystem::LevelTransientData {};
FGuid USodaSubsystem::SlotToLoad{};

USodaSubsystem* USodaSubsystem::Get()
{
	return SodaApp.GetSodaSubsystem();
}

USodaSubsystem* USodaSubsystem::GetChecked()
{
	return SodaApp.GetSodaSubsystemChecked();
}

USodaSubsystem::USodaSubsystem()
{
	DefaultLevelStateClass = ALevelState::StaticClass();
	SpectatorClass = ASodalSpectator::StaticClass();
}

TStatId USodaSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(USodaSubsystem, STATGROUP_Tickables);
}

void USodaSubsystem::PostInitialize()
{
	Super::PostInitialize();

	InitGameHandle = FGameModeEvents::GameModeInitializedEvent.AddUObject(this, &USodaSubsystem::InitGame);
	PreEndGameHandle = FWorldDelegates::OnWorldBeginTearDown.AddUObject(this, &USodaSubsystem::PreEndGame);

	ActorSpawnedDelegateHandle = GetWorldRef().AddOnActorSpawnedHandler(FOnActorSpawned::FDelegate::CreateUObject(this, &USodaSubsystem::OnActorSpawned));
	ActorDestroyedDelegateHandle = GetWorldRef().AddOnActorDestroyedHandler(FOnActorSpawned::FDelegate::CreateUObject(this, &USodaSubsystem::OnActorDestroyed));

	for (TActorIterator<AActor> It(&GetWorldRef()); It; ++It)
	{
		if (It->GetClass()->ImplementsInterface(USodaActor::StaticClass()))
		{
			SodaActors.Add(*It);
		}
	}
}

void USodaSubsystem::Deinitialize()
{
	Super::Deinitialize();

	SodaApp.NotifyEndGame();

	GetWorldRef().RemoveOnActorSpawnedHandler(ActorSpawnedDelegateHandle);
	GetWorldRef().RemoveOnActorDestroyededHandler(ActorDestroyedDelegateHandle);
}

void USodaSubsystem::InitGame(AGameModeBase* GameMode)
{
	UWorld* World = GetWorld();
	check(World);

	if (World->WorldType != EWorldType::Game && World->WorldType != EWorldType::PIE)
	{
		UE_LOG(LogSoda, Warning, TEXT("USodaSubsystem::InitGame(); Wrong WorldType"));
		return;
	}

	// Registre loadad SodaActors
	for (TObjectIterator<UClass> It; It; ++It)
	{
		if (It->HasAnyClassFlags(CLASS_Abstract) ||
			It->GetName().StartsWith(TEXT("SKEL_"), ESearchCase::CaseSensitive) ||
			It->GetName().StartsWith(TEXT("REINST_"), ESearchCase::CaseSensitive) ||
			It->HasAnyClassFlags(CLASS_Abstract) ||
			!It->IsChildOf<AActor>() ||
			!It->ImplementsInterface(USodaActor::StaticClass())
			) continue;

		if (ISodaActor* SodaActor = Cast<ISodaActor>(It->GetDefaultObject()))
		{
			if (const FSodaActorDescriptor* Desc = SodaActor->GenerateActorDescriptor())
			{
				SodaActorDescriptors.FindOrAdd(*It) = *Desc;
			}
		}
	}

	// Registre Metadata
	FRuntimeMetaData::RegisterMetadataPrimaryAssets();

	// Registre unloadad SodaActors
	UAssetManager& AssetManager = UAssetManager::Get();
	TArray<FPrimaryAssetId> PrimaryAssetIdList;
	AssetManager.GetPrimaryAssetIdList(FPrimaryAssetType("SodaLibraryPrimaryAsset"), PrimaryAssetIdList);
	for (auto& AssetId : PrimaryAssetIdList)
	{
		AssetManager.LoadPrimaryAsset(AssetId, TArray<FName>{TEXT("VehicleComponents")}, FStreamableDelegate::CreateLambda([this, AssetId]()
		{
			UAssetManager& AssetManager = UAssetManager::Get();
			UClass* AssetObjectClass = AssetManager.GetPrimaryAssetObjectClass<USodaLibraryPrimaryAsset>(AssetId);
			if (AssetObjectClass)
			{
				UE_LOG(LogSoda, Log, TEXT("USodaSubsystem::InitGame(); Loaded asset %s"), *AssetObjectClass->GetName());
				for (auto& It : CastChecked<USodaLibraryPrimaryAsset>(AssetObjectClass->GetDefaultObject())->SodaActors)
				{
					SodaActorDescriptors.FindOrAdd(It.Key) = It.Value;
					UE_LOG(LogSoda, Log, TEXT("USodaSubsystem::InitGame(); Registre \"%s\" [%s]"), *It.Key.ToString(), It.Key.Get() ? TEXT("Loaded") : TEXT("Unloaded"));
				}
			}
			else
			{
				UE_LOG(LogSoda, Error, TEXT("USodaSubsystem::InitGame(); Primary asset \"%s\" load faild"), *AssetId.ToString());
			}
		}));
	}

	SodaApp.NotifyInitGame(this);

	if (GetDefault<USodaCommonSettings>()->bTagActorsAtBeginPlay)
	{
		USodaStatics::TagActorsInLevel(this, true);
		USodaStatics::AddV2XMarkerToAllVehiclesInLevel(this, ASodaVehicle::StaticClass()->GetClass());
	}

	/* Initialize LevelState */
	InitLevelState();
	SlotToLoad = FGuid{};

	/* Initialize level default pinned tool actors */
	/*
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		if (IToolActor* ToolActor = Cast<IToolActor>(*It))
		{
			if (ToolActor->IsPinnedActor())
			{
				ToolActor->DeserializePinnedActor();
			}
		}
	}
	*/
	
	if (UGameInstance* Instance = UGameplayStatics::GetGameInstance(this))
	{
		UGameViewportClient* ViewPortClient = Instance->GetGameViewportClient();
		if (ViewPortClient)
		{
			ViewPortClient->OnWindowCloseRequested().BindUObject(this, &USodaSubsystem::OnWindowCloseRequested);
		}
	}
}

void USodaSubsystem::InitLevelState()
{
	const FString CurrentLevelName = UGameplayStatics::GetCurrentLevelName(this, true);
	ULevelSaveGame* LevelSaveGame = nullptr;
	soda::FFileDatabaseSlotInfo SlotInfo;
	//bool bSlotInfoIsValid = false;

	if (LevelTransientData.IsValid()) // Load transient level data
	{
		LevelSaveGame = Cast<ULevelSaveGame>(UGameplayStatics::LoadGameFromMemory(LevelTransientData.Memory));
		if (!LevelSaveGame)
		{
			UE_LOG(LogSoda, Error, TEXT("USodaSubsystem::InitLevelState(); Can't load SavaGame from memory"));
		}
	}
	else if (SlotToLoad.IsValid())
	{
		LevelSaveGame = ALevelState::LoadSaveGameFromSlot(SlotToLoad, SlotInfo);
		if (LevelSaveGame)
		{
			//bSlotInfoIsValid = true;
		}
		else
		{
			UE_LOG(LogSoda, Error, TEXT("USodaSubsystem::InitLevelState(); Can't load SavaGame from Slot"));
		}
	}

	if (LevelSaveGame)
	{
		if (LevelSaveGame->LevelName != CurrentLevelName)
		{
			UE_LOG(LogSoda, Error, TEXT("USodaSubsystem::InitLevelState(); Transient slot LevelName mismatch, loaded name \"%s\", current name \"%s\""), *LevelSaveGame->LevelName, *CurrentLevelName);
			LevelSaveGame = nullptr;
			LevelTransientData = FLevelTransientData{};
		}
	}

	auto It = TActorIterator<ALevelState>(GetWorld());
	LevelState = It ? *It : nullptr;

	if (LevelSaveGame)
	{
		if (LevelState)
		{
			LevelSaveGame->LevelDataRecord.DeserializeActor(LevelState, false);
		}
		else
		{
			LevelState = LevelSaveGame->LevelDataRecord.SpawnActor<ALevelState>(GetWorld(), false);
		}

		/*
		if (bSlotInfoIsValid)
		{
			LevelState->SlotGuid = SlotInfo.GUID;
			LevelState->SlotLable = SlotInfo.Lable;
		}
		*/
	}

	if (!LevelState)
	{
		if (DefaultLevelStateClass)
		{
			LevelState = GetWorld()->SpawnActor<ALevelState>(DefaultLevelStateClass);
		}
		else
		{
			LevelState = GetWorld()->SpawnActor<ALevelState>();
		}
	}

	check(LevelState);

	LevelState->SpawnSavedActors();
}

void USodaSubsystem::PreEndGame(UWorld* World)
{
	if (World->WorldType != EWorldType::Game && World->WorldType != EWorldType::PIE)
	{
		UE_LOG(LogSoda, Warning, TEXT("USodaSubsystem::PreEndGame(); Wrong WorldType"));
		return;
	}

	ScenarioStop(EScenarioStopReason::QuitApplication, EScenarioStopMode::StopSiganalOnly);

	if (InitGameHandle.IsValid())
	{
		FGameModeEvents::GameModeInitializedEvent.Remove(InitGameHandle);
	}

	if (PreEndGameHandle.IsValid())
	{
		FWorldDelegates::OnWorldBeginTearDown.Remove(PreEndGameHandle);
	}

	/*
	for (auto& It : DatasetManagersForScenario)
	{
		It->ClearDatasetsQueue();
	}
	*/
}

bool USodaSubsystem::OnWindowCloseRequested()
{
	RequestQuit();
	return false;
}


void USodaSubsystem::OnWorldBeginPlay(UWorld& World)
{
	Super::OnWorldBeginPlay(World);

	check(this == USodaSubsystem::Get());
	APlayerController* PlayerController = World.GetFirstPlayerController();
	check(PlayerController);
	FInputModeGameAndUI Mode;
	Mode.SetHideCursorDuringCapture(true).SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	PlayerController->SetInputMode(Mode);
	PlayerController->bShowMouseCursor = true;

	ViewportClient = Cast<USodaGameViewportClient>(World.GetGameViewport());
	if (ViewportClient)
	{
		SAssignNew(SodaViewport, soda::SSodaViewport, &World);
		ViewportClient->InitUI(SodaViewport);
	}

	RestoreLevelTransientData();

	static bool bWasShown = false;
	if (GetDefault<USodaCommonSettings>()->bShowQuickStartAtStartUp && !bWasShown)
	{
		bWasShown = true;
		OpenWindow("Quick Start", SNew(soda::SQuickStartWindow));
	}
}


void USodaSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	/*
	UWorld* World = GetWorld();
	check(World);

	if (IsScenarioRunning())
	{
		if (soda::FDBGateway::Instance().IsDatasetRecording() && soda::FDBGateway::Instance().IsDatasetRecordingStalled())
		{
			ScenarioStop(EScenarioStopReason::InnerError, EScenarioStopMode::RestartLevel, "Scenario Recording Failed. " + soda::FDBGateway::Instance().GetLastError());
		}
	}
	*/

	for (auto& [Name, Manager] : SodaApp.GetDatasetManagers())
	{
		Manager->Tick(DeltaTime);
	}
}

void USodaSubsystem::PushToolBox(TSharedRef<soda::SToolBox> Widget, bool InstedPrev)
{
	if (SodaViewport.IsValid())
	{
		SodaViewport->PushToolBox(Widget, InstedPrev);;
	}
}

bool USodaSubsystem::PopToolBox()
{
	if (SodaViewport.IsValid())
	{
		return SodaViewport->PopToolBox();
	}
	else
	{
		return false;
	}
}

bool USodaSubsystem::CloseWindow(bool bCloseAllWindows)
{
	if (SodaViewport.IsValid())
	{
		return SodaViewport->CloseWindow(bCloseAllWindows);
	}
	else
	{
		return false;
	}
}

void USodaSubsystem::SetSpectatorMode(bool bSpectatorMode)
{
	UWorld* World = GetWorld();
	check(World);

	APlayerController* PlayerController = World->GetFirstPlayerController();
	if (!PlayerController)
	{
		return;
	}

	ASodaVehicle* Vehicle = PlayerController->GetPawn<ASodaVehicle>();

	if (bSpectatorMode)
	{
		if (IsValid(SpectatorActor))
		{
			PlayerController->Possess(SpectatorActor);
		}
		else if (SpectatorClass.Get() )
		{
			FTransform Transform;
			if (IsValid(LevelState) && LevelState->SpectatorActorRecord.IsRecordValid())
			{
				Transform = LevelState->SpectatorActorRecord.Transform;
			}
			else
			{
				FVector ViewLocation;
				FRotator ViewRotation;
				PlayerController->GetPlayerViewPoint(ViewLocation, ViewRotation);
				Transform = FTransform(ViewRotation, ViewLocation);
			}
			if (IsValid(Vehicle))
			{
				UnpossesVehicle = Vehicle;
			}
			SpectatorActor = World->SpawnActorDeferred<ASodalSpectator>(
				SpectatorClass.Get(),
				Transform, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
			check(SpectatorActor);
			if (IsValid(LevelState)) LevelState->SpectatorActorRecord.DeserializeActor(SpectatorActor, false, true);
			SpectatorActor->FinishSpawning(Transform);
			PlayerController->Possess(SpectatorActor);
		}
	}
	else
	{
		ASodaVehicle* PossesVehicle = UnpossesVehicle;
		if (!IsValid(PossesVehicle)) // Try to find anouther vehicle
		{
			TActorIterator<ASodaVehicle> It(GetWorld());
			if (It) PossesVehicle = *It;
		}

		if (IsValid(PossesVehicle))
		{
			if (IsValid(SpectatorActor))
			{
				if (AController* Controller = SpectatorActor->GetController())
				{
					SpectatorActor->SetActorRotation(Controller->GetControlRotation());
				}
				LevelState->SpectatorActorRecord.SerializeActor(SpectatorActor, false);
				SpectatorActor->Destroy();
				SpectatorActor = nullptr;
			}
			PlayerController->Possess(PossesVehicle);
		}

		UnpossesVehicle = nullptr;
	}
}

ASodaVehicle* USodaSubsystem::GetActiveVehicle()
{
	UWorld* World = GetWorld();
	check(World);

	APlayerController* PlayerController = World->GetFirstPlayerController();

	if (IsValid(PlayerController))
	{
		ASodaVehicle* Vehicle = PlayerController->GetPawn<ASodaVehicle>();

		if (IsValid(Vehicle))
		{
			return Vehicle;
		}
	}

	if (IsValid(UnpossesVehicle))
	{
		return UnpossesVehicle;
	}

	TActorIterator<ASodaVehicle> FirstVehicleIt(World);
	if (FirstVehicleIt)
	{
		return *FirstVehicleIt;
	}

	return nullptr;
}

void USodaSubsystem::SetActiveVehicle(ASodaVehicle* Vehicle)
{
	UWorld* World = GetWorld();
	check(World);

	APlayerController* PlayerController = World->GetFirstPlayerController();
	check(PlayerController);

	if (IsValid(Vehicle))
	{
		SetSpectatorMode(false);
		PlayerController->Possess(Vehicle);
	}
}

bool USodaSubsystem::ScenarioPlay()
{
	if (bIsScenarioRunning)
	{
		return false;
	}

	// Filling LevelTransientData
	LevelTransientData = FLevelTransientData{};
	if (!LevelState->SaveToToMemory(LevelTransientData.Memory))
	{
		UE_LOG(LogSoda, Error, TEXT("USodaSubsystem::ScenarioPlay(); Can't LevelState->SaveToToMemory()"));
		ShowMessageBox(soda::EMessageBoxType::OK, "Scenario Play Failed", "Can't save level to memory");
		return false;
	}

	if (APlayerController* PlayerController = GetWorld()->GetFirstPlayerController())
	{
		if (APawn * Pawn = PlayerController->GetPawn())
		{
			LevelTransientData.PossesdActor = Pawn->GetName();
		}
	}
	if (ViewportClient)
	{
		LevelTransientData.Mode = SodaViewport->GetUIMode();
		if (const AActor* Actor = ViewportClient->Selection->GetSelectedActor())
		{
			LevelTransientData.SelectedActor = Actor->GetName();
		}
	}
	for (auto& It : LevelState->ActorFactory->GetActors())
	{
		if (ISodaActor* SodaActor = Cast<ISodaActor>(It))
		{
			if (SodaActor->IsDirty())
			{
				LevelTransientData.ActorsDirty.Add(It->GetName());
			}
		}
	}

	// Call ScenarioBegin() for ISodaActors
	bIsScenarioRunning = true;
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		if (It->GetClass()->ImplementsInterface(USodaActor::StaticClass()))
		{
			if (ISodaActor* SodaActor = Cast<ISodaActor>(*It))
			{
				SodaActor->ScenarioBegin();
				if (SodaActor->GetActorHiddenInScenario())
				{
					It->SetActorHiddenInGame(true);
				}
			}
			else
			{
				ISodaActor::Execute_ReceiveScenarioBegin(*It);
			}
		}
	}

	OnScenarioPlay.Broadcast();

	// Start record datasets
	ActiveDatasets.Reset();
	for (auto It = GetMutableDefault<USodaCommonSettings>()->DatasetNamesToUse.CreateIterator(); It; ++It)
	{
		if (auto* Found = SodaApp.GetDatasetManagers().Find(*It))
		{
			if ((*Found)->BeginRecording())
			{
				ActiveDatasets.Add(*Found);
			}
			else
			{
				//ShowMessageBox(soda::EMessageBoxType::OK, "Scenario Play Failed", (*Found)->GetLastError());
				//return false;
			}
		}
		else
		{
			// If somehow a dataset name ended up here that is not registered, delete it
			It.RemoveCurrent();
			GetMutableDefault<USodaCommonSettings>()->SaveConfig();
		}
	}

	return true;

}

void USodaSubsystem::ScenarioStop(EScenarioStopReason Reason, EScenarioStopMode Mode, const FString& UserMessage)
{
	if (!bIsScenarioRunning)
	{
		return;
	}

	bIsScenarioRunning = false;
	LevelTransientData.UserMessage = UserMessage;

	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		if (It->GetClass()->ImplementsInterface(USodaActor::StaticClass()))
		{
			if (ISodaActor* SodaActor = Cast<ISodaActor>(*It))
			{
				SodaActor->ScenarioEnd();
			}
			else
			{
				ISodaActor::Execute_ReceiveScenarioEnd(*It);
			}
		}
	}

	OnScenarioStop.Broadcast(Reason);

	for (auto& Manager : ActiveDatasets)
	{
		Manager->EndRecording(Reason, false); // TODO: bImmediately = (EScenarioStopMode==RestartLevel)
	}
	ActiveDatasets.Reset();

	for (TObjectIterator<UObject> It; It; ++It)
	{
		if (It->GetWorld() == GetWorld() && (!It->GetName().StartsWith(TEXT("SKEL_")) || !It->GetName().StartsWith(TEXT("REINST_"))))
		{
			if (IObjectDataset* DatasetObject = Cast<IObjectDataset>(*It))
			{
				DatasetObject->ReleaseDatasetHandlers();
			}
		}
	}

	if (Mode == EScenarioStopMode::RestartLevel)
	{
		if (APlayerController* PlayerController = GetWorld()->GetFirstPlayerController())
		{
			PlayerController->RestartLevel();
		}
	}
	else if (Mode == EScenarioStopMode::ResetSodaActorsOnly)
	{
		LevelState->ClearLevel();
		GEngine->ForceGarbageCollection(true);
		FCoreUObjectDelegates::GetPostGarbageCollect().RemoveAll(this);
		FCoreUObjectDelegates::GetPostGarbageCollect().AddUObject(this, &USodaSubsystem::OnPostGarbageCollect, 0.0f);
	}
	else // (Mode == EScenarioStopMode::StopSiganalOnly)
	{
		// Do not anything
	}
	
}

void USodaSubsystem::RestoreLevelTransientData()
{
	SetSpectatorMode(true);

	if (LevelTransientData.IsValid())
	{
		LevelTransientData.Memory.Reset();

		for (auto& It : LevelTransientData.ActorsDirty)
		{
			AActor* const* Actor = LevelState->ActorFactory->GetActors().FindByPredicate([&It](const AActor* Actor) { return Actor->GetName() == It; });
			if (ISodaActor* SodaActor = Cast<ISodaActor>(*Actor))
			{
				SodaActor->MarkAsDirty();
			}
		}

		APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
		check(PlayerController);

		SodaViewport->SetUIMode(LevelTransientData.Mode);
		if (ViewportClient && !LevelTransientData.SelectedActor.IsEmpty())
		{
			if (AActor* const* Actor = LevelState->ActorFactory->GetActors().FindByPredicate([this](const AActor* Actor) { return Actor->GetName() == LevelTransientData.SelectedActor; }))
			{
				ViewportClient->Selection->SelectActor(*Actor, nullptr);
			}
		}

		if (!LevelTransientData.PossesdActor.IsEmpty() && SpectatorActor->GetName() != LevelTransientData.PossesdActor)
		{
			for (TActorIterator<APawn> It(GetWorld()); It; ++It)
			{
				if (It->GetName() == LevelTransientData.PossesdActor)
				{
					PlayerController->Possess(*It);
					break;
				}
			}
		}

		if (!LevelTransientData.UserMessage.IsEmpty())
		{
			SodaApp.GetSodaSubsystem()->ShowMessageBox(soda::EMessageBoxType::OK, "Scenarip stoped", LevelTransientData.UserMessage);
		}
	}
}

void USodaSubsystem::OnPostGarbageCollect(float Delay /*= 0.0f*/)
{
	FCoreUObjectDelegates::GetPostGarbageCollect().RemoveAll(this);
	LevelState->SpawnSavedActors();
	RestoreLevelTransientData();
}

void USodaSubsystem::RequestQuit(bool bForce)
{
	ScenarioStop(EScenarioStopReason::QuitApplication, EScenarioStopMode::StopSiganalOnly);

	CloseWindow(true);

	if (!bForce && SodaViewport.IsValid())
	{
		OpenWindow("Quit", SNew(soda::SSaveAllWindow, soda::ESaveAllWindowMode::Quit));
	}
	else
	{
		//Delay before exiting to end the current scenario if it was running
		GetWorld()->GetTimerManager().SetTimer(TimerHandle, [this]()
		{
			UKismetSystemLibrary::QuitGame(this, nullptr, EQuitPreference::Quit, false);
		}, 0.2, false);
	}

}

void USodaSubsystem::RequestRestartLevel(bool bForce)
{
	ScenarioStop(EScenarioStopReason::QuitApplication, EScenarioStopMode::StopSiganalOnly);

	CloseWindow(true);

	if (!bForce && SodaViewport.IsValid())
	{
		OpenWindow("Restart Level", SNew(soda::SSaveAllWindow, soda::ESaveAllWindowMode::Restart));
	}
	else
	{
		//Delay before exiting to end the current scenario if it was running
		GetWorld()->GetTimerManager().SetTimer(TimerHandle, [this]()
		{
			SlotToLoad = LevelState->GetSlotGuid();
			LevelTransientData.Memory.Reset();
			UGameplayStatics::OpenLevel(this, *UGameplayStatics::GetCurrentLevelName(this, true), false);
		}, 0.2, false);
	}
}

TSharedPtr<soda::SMessageBox> USodaSubsystem::ShowMessageBox(soda::EMessageBoxType Type, const FString& Caption, const FString& Text)
{
	if (SodaViewport.IsValid())
	{
		return SodaViewport->ShowMessageBox(Type, Caption, Text);
	}
	else
	{
		return TSharedPtr<soda::SMessageBox>();
	}
}

TSharedPtr<soda::SMenuWindow> USodaSubsystem::OpenWindow(const FString& Caption, TSharedRef<soda::SMenuWindowContent> Content)
{
	if (SodaViewport.IsValid())
	{
		return SodaViewport->OpenWindow(Caption, Content);
	}
	else
	{
		return TSharedPtr<soda::SMenuWindow>();
	}
}

bool USodaSubsystem::CloseWindow(soda::SMenuWindow * Wnd)
{
	if (SodaViewport.IsValid())
	{
		return SodaViewport->CloseWindow(Wnd);
	}
	else
	{
		return false;
	}
}

ASodaActorFactory* USodaSubsystem::GetActorFactory()
{
	check(LevelState);
	return LevelState->ActorFactory;
}

const FSodaActorDescriptor& USodaSubsystem::GetSodaActorDescriptor(TSoftClassPtr<AActor> Class) const
{
	if (const FSodaActorDescriptor* Desc = SodaActorDescriptors.Find(Class))
	{
		return *Desc;
	}

	static FSodaActorDescriptor Default;
	return Default;
}

const TMap<TSoftClassPtr<AActor>, FSodaActorDescriptor>& USodaSubsystem::GetSodaActorDescriptors() const
{
	return SodaActorDescriptors;
}

TSharedPtr<soda::SWaitingPanel> USodaSubsystem::ShowWaitingPanel(const FString& Caption, const FString& SubCaption)
{
	if (SodaViewport.IsValid())
	{
		return SodaViewport->ShowWaitingPanel(Caption, SubCaption);
	}
	else
	{
		return TSharedPtr<soda::SWaitingPanel>();
	}
}

bool USodaSubsystem::CloseWaitingPanel(soda::SWaitingPanel* InWaitingPanel = nullptr)
{
	if (SodaViewport.IsValid())
	{
		return SodaViewport->CloseWaitingPanel(InWaitingPanel);
	}
	else
	{
		return false;
	}
}

bool USodaSubsystem::CloseWaitingPanel(bool bCloseAll)
{
	if (SodaViewport.IsValid())
	{
		return SodaViewport->CloseWaitingPanel(bCloseAll);
	}
	else
	{
		return false;
	}
}

void USodaSubsystem::OnActorSpawned(AActor* InActor)
{
	if (InActor->GetClass()->ImplementsInterface(USodaActor::StaticClass()))
	{
		SodaActors.Add(InActor);
	}
}

void USodaSubsystem::OnActorDestroyed(AActor* InActor)
{
	if (InActor->GetClass()->ImplementsInterface(USodaActor::StaticClass()))
	{
		SodaActors.Remove(InActor);
	}
}

bool USodaSubsystem::LoadEmptyLevel(const FString& InLevelName)
{
	SlotToLoad = FGuid{};
	LevelTransientData.Memory.Reset();

	FString LevelName = InLevelName.IsEmpty() ? UGameplayStatics::GetCurrentLevelName(this, true) : InLevelName;
	UGameplayStatics::OpenLevel(this, *LevelName, false);
	return true;
}

bool USodaSubsystem::LoadLevelFromSlot(const FGuid& Guid)
{
	if (!Guid.IsValid())
	{
		UE_LOG(LogSoda, Error, TEXT("USodaSubsystem::LoadLevelFromSlot(); Slot isn't valid "));
		return false;
	}

	auto& Database = SodaApp.GetFileDatabaseManager();
	soda::FFileDatabaseSlotInfo SlotInfo;
	if (!Database.GetSlot(Guid, SlotInfo))
	{
		UE_LOG(LogSoda, Error, TEXT("USodaSubsystem::LoadLevelFromSlot(); Can't find slot "));
		return false;
	}

	FString LevelName;
	if (!ALevelState::DeserializeSlotDescriptor(SlotInfo.JsonDescription, LevelName))
	{
		UE_LOG(LogSoda, Error, TEXT("ALevelState::LoadLevelFromSlot(); DeserializeSlotDescriptor() faild"));
		return false;
	}

	SlotToLoad = Guid;
	LevelTransientData.Memory.Reset();

	UGameplayStatics::OpenLevel(this, *LevelName, false);
	return true;
}