// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/SodaGameMode.h"
#include "Soda/UnrealSoda.h"
#include "Soda/SodaApp.h"
#include "Soda/LevelState.h"
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
#include "Soda/UI/SToolBox.h"
#include "Soda/UI/SWaitingPanel.h"
#include "Soda/SodaGameViewportClient.h"
#include "Soda/IToolActor.h"
#include "Soda/SodaLibraryPrimaryAsset.h"
#include "RuntimeMetaData.h"
#include "Soda/DBGateway.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "Soda/SodaUserSettings.h"
#include "UObject/UObjectIterator.h"
#include "Async/Async.h"
#include "Soda/SodaActorFactory.h"
#include "Soda/Editor/SodaSelection.h"
#include "Engine/GameInstance.h"
#include "Engine/Engine.h"

#define SLOT_GAMEMODE "SodaGameMode"

USodaGameModeComponent::FScenarioLevelSavedData USodaGameModeComponent::ScenarioLevelSavedData {};

USodaGameModeComponent* USodaGameModeComponent::Get()
{
	return SodaApp.GetGameMode();
}

USodaGameModeComponent* USodaGameModeComponent::GetChecked()
{
	return SodaApp.GetGameModeChecked();
}

USodaGameModeComponent::USodaGameModeComponent()
{
	//bWantsInitializeComponent = true;
	PrimaryComponentTick.bCanEverTick = true;
	bTickInEditor = false;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;

	DefaultLevelStateClass = ALevelState::StaticClass();
	SpectatorClass = ASodalSpectator::StaticClass();
}

void USodaGameModeComponent::OnComponentCreated()
{
	Super::OnComponentCreated();

	InitGameHandle = FGameModeEvents::GameModeInitializedEvent.AddUObject(this, &USodaGameModeComponent::InitGame);
	PreEndGameHandle = FWorldDelegates::OnWorldBeginTearDown.AddUObject(this, &USodaGameModeComponent::PreEndGame);
}

void USodaGameModeComponent::OnComponentDestroyed(bool bDestroyingHierarchy)
{
	Super::OnComponentDestroyed(bDestroyingHierarchy);

	SodaApp.NotifyEndGame();
}

void USodaGameModeComponent::InitGame(AGameModeBase* GameMode)
{
	UWorld* World = GetWorld();
	check(World);

	if (World->WorldType != EWorldType::Game && World->WorldType != EWorldType::PIE)
	{
		UE_LOG(LogSoda, Warning, TEXT("USodaGameModeComponent::InitGame(); Wrong WorldType"));
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
				UE_LOG(LogSoda, Log, TEXT("USodaGameModeComponent::InitGame(); Loaded asset %s"), *AssetObjectClass->GetName());
				for (auto& It : CastChecked<USodaLibraryPrimaryAsset>(AssetObjectClass->GetDefaultObject())->SodaActors)
				{
					SodaActorDescriptors.FindOrAdd(It.Key) = It.Value;
					UE_LOG(LogSoda, Log, TEXT("USodaGameModeComponent::InitGame(); Registre \"%s\" [%s]"), *It.Key.ToString(), It.Key.Get() ? TEXT("Loaded") : TEXT("Unloaded"));
				}
			}
			else
			{
				UE_LOG(LogSoda, Error, TEXT("USodaGameModeComponent::InitGame(); Primary asset \"%s\" load faild"), *AssetId.ToString());
			}
		}));
	}

	SodaApp.NotifyInitGame(this);

	if (Cast<AGameModeBase>(GetOwner()) == nullptr)
	{
		UE_LOG(LogSoda, Fatal, TEXT("The USodaGameModeComponent must be child of AGameModeBase"));
	}

	if (SodaApp.GetSodaUserSettings()->bTagActorsAtBeginPlay)
	{
		USodaStatics::TagActorsInLevel(this, true);
		USodaStatics::AddV2XMarkerToAllVehiclesInLevel(this, ASodaVehicle::StaticClass()->GetClass());
	}

	/* Initialize LevelState */
	LevelState = ALevelState::CreateOrLoad(this, DefaultLevelStateClass, ScenarioLevelSavedData.bIsValid);
	check(LevelState);
	LevelState->FinishLoadLevel(); //Spawn all saved actors

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
			ViewPortClient->OnWindowCloseRequested().BindUObject(this, &USodaGameModeComponent::OnWindowCloseRequested);
		}
	}
}

void USodaGameModeComponent::PreEndGame(UWorld* World)
{
	if (World->WorldType != EWorldType::Game && World->WorldType != EWorldType::PIE)
	{
		UE_LOG(LogSoda, Warning, TEXT("USodaGameModeComponent::PreEndGame(); Wrong WorldType"));
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

	soda::FDBGateway::Instance().ClearDatasetsQueue();
}

bool USodaGameModeComponent::OnWindowCloseRequested()
{
	RequestQuit();
	return false;
}


void USodaGameModeComponent::BeginPlay()
{
	Super::BeginPlay();

	check(this == USodaGameModeComponent::Get());
	UWorld* World = GetWorld();
	check(World);
	APlayerController* PlayerController = World->GetFirstPlayerController();
	check(PlayerController);
	FInputModeGameAndUI Mode;
	Mode.SetHideCursorDuringCapture(true).SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	PlayerController->SetInputMode(Mode);
	PlayerController->bShowMouseCursor = true;

	ViewportClient = Cast<USodaGameViewportClient>(GetWorld()->GetGameViewport());
	if (ViewportClient)
	{
		SAssignNew(SodaViewport, soda::SSodaViewport, GetWorld());
		ViewportClient->InitUI(SodaViewport);
	}

	if (SodaApp.GetSodaUserSettings()->bAutoConnect)
	{
		UpdateDBGateway(false);
	}

	AfterScenarioStop();
}

void USodaGameModeComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
}

void USodaGameModeComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	UWorld* World = GetWorld();
	check(World);

	if (IsScenarioRunning())
	{
		if (soda::FDBGateway::Instance().IsDatasetRecording() && soda::FDBGateway::Instance().IsDatasetRecordingStalled())
		{
			ScenarioStop(EScenarioStopReason::InnerError, EScenarioStopMode::RestartLevel, "Scenario Recording Failed. " + soda::FDBGateway::Instance().GetLastError());
		}
	}
}

void USodaGameModeComponent::PushToolBox(TSharedRef<soda::SToolBox> Widget, bool InstedPrev)
{
	if (SodaViewport.IsValid())
	{
		SodaViewport->PushToolBox(Widget, InstedPrev);;
	}
}

bool USodaGameModeComponent::PopToolBox()
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

bool USodaGameModeComponent::CloseWindow(bool bCloseAllWindows)
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

void USodaGameModeComponent::SetSpectatorMode(bool bSpectatorMode)
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

ASodaVehicle* USodaGameModeComponent::GetActiveVehicle()
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

void USodaGameModeComponent::SetActiveVehicle(ASodaVehicle* Vehicle)
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

bool USodaGameModeComponent::ScenarioPlay()
{
	if (!bIsScenarioRunning)
	{
		ScenarioLevelSavedData.bIsValid = LevelState->SaveLevelToTransientSlot();
		if (!ScenarioLevelSavedData.bIsValid)
		{
			UE_LOG(LogSoda, Error, TEXT("USodaGameModeComponent::ScenarioPlay(); Can't SaveLevelToTransientSlot()"));
			ShowMessageBox(soda::EMessageBoxType::OK, "Scenario Play Failed", soda::FDBGateway::Instance().GetLastError());
			return false;
		}
		ScenarioLevelSavedData.ActorsDirty.Empty();
		ScenarioLevelSavedData.SelectedActor.Reset();
		ScenarioLevelSavedData.PossesdActor.Reset();
		ScenarioLevelSavedData.UserMessage.Empty();
		if (APlayerController* PlayerController = GetWorld()->GetFirstPlayerController())
		{
			if (APawn * Pawn = PlayerController->GetPawn())
			{
				ScenarioLevelSavedData.PossesdActor = Pawn->GetName();
			}
		}
		if (ViewportClient)
		{
			ScenarioLevelSavedData.Mode = SodaViewport->GetUIMode();
			if (const AActor* Actor = ViewportClient->Selection->GetSelectedActor())
			{
				ScenarioLevelSavedData.SelectedActor = Actor->GetName();
			}
		}
		
		for (auto& It : LevelState->ActorFactory->GetActors())
		{
			if (ISodaActor* SodaActor = Cast<ISodaActor>(It))
			{
				if (SodaActor->IsDirty())
				{
					ScenarioLevelSavedData.ActorsDirty.Add(It->GetName());
				}
			}
		}

		if (SodaApp.GetSodaUserSettings()->bRecordDataset)
		{
			if (soda::FDBGateway::Instance().BeginRecordDataset())
			{
				FNotificationInfo Info(FText::FromString(FString("Begin recording dataset; ID: ") + FString::FromInt(soda::FDBGateway::Instance().GetDatasetID())));
				Info.ExpireDuration = 5.0f;
				Info.Image = FCoreStyle::Get().GetBrush(TEXT("Icons.SuccessWithColor"));
				FSlateNotificationManager::Get().AddNotification(Info);
			}
			else
			{
				ShowMessageBox(soda::EMessageBoxType::OK, "Scenario Play Failed", soda::FDBGateway::Instance().GetLastError());
				return false;
			}
		}

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
		return true;
	}
	return false;
}

void USodaGameModeComponent::ScenarioStop(EScenarioStopReason Reason, EScenarioStopMode Mode, const FString& UserMessage)
{
	if (bIsScenarioRunning)
	{
		bIsScenarioRunning = false;
		ScenarioLevelSavedData.UserMessage = UserMessage;

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
		if (soda::FDBGateway::Instance().IsDatasetRecording())
		{
			soda::FDBGateway::Instance().EndRecordDataset(Reason);
		}

		OnScenarioStop.Broadcast(Reason);

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
			FCoreUObjectDelegates::GetPostGarbageCollect().AddUObject(this, &USodaGameModeComponent::OnPostGarbageCollect, 0.0f);
		}
		else // (Mode == EScenarioStopMode::StopSiganalOnly)
		{
			// Do not anything
		}
	}
}

void USodaGameModeComponent::AfterScenarioStop()
{
	SetSpectatorMode(true);

	if (ScenarioLevelSavedData.bIsValid)
	{
		ScenarioLevelSavedData.bIsValid = false;

		for (auto& It : ScenarioLevelSavedData.ActorsDirty)
		{
			AActor* const* Actor = LevelState->ActorFactory->GetActors().FindByPredicate([&It](const AActor* Actor) { return Actor->GetName() == It; });
			if (ISodaActor* SodaActor = Cast<ISodaActor>(*Actor))
			{
				SodaActor->MarkAsDirty();
			}
		}

		APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
		check(PlayerController);

		SodaViewport->SetUIMode(ScenarioLevelSavedData.Mode);
		if (ViewportClient && !ScenarioLevelSavedData.SelectedActor.IsEmpty())
		{
			if (AActor* const* Actor = LevelState->ActorFactory->GetActors().FindByPredicate([this](const AActor* Actor) { return Actor->GetName() == ScenarioLevelSavedData.SelectedActor; }))
			{
				ViewportClient->Selection->SelectActor(*Actor, nullptr);
			}
		}

		if (!ScenarioLevelSavedData.PossesdActor.IsEmpty() && SpectatorActor->GetName() != ScenarioLevelSavedData.PossesdActor)
		{
			for (TActorIterator<APawn> It(GetWorld()); It; ++It)
			{
				if (It->GetName() == ScenarioLevelSavedData.PossesdActor)
				{
					PlayerController->Possess(*It);
					break;
				}
			}
		}

		if (!ScenarioLevelSavedData.UserMessage.IsEmpty())
		{
			SodaApp.GetGameModeChecked()->ShowMessageBox(soda::EMessageBoxType::OK, "Scenarip stoped", ScenarioLevelSavedData.UserMessage);
		}
	}
}

void USodaGameModeComponent::OnPostGarbageCollect(float Delay /*= 0.0f*/)
{
	FCoreUObjectDelegates::GetPostGarbageCollect().RemoveAll(this);
	LevelState->FinishLoadLevel();
	AfterScenarioStop();
}

int64 USodaGameModeComponent::GetScenarioID() const
{
	if (!bIsScenarioRunning)
	{
		return -1;
	}
	else 
	{
		return soda::FDBGateway::Instance().GetDatasetID();
	}
}


void USodaGameModeComponent::RequestQuit(bool bForce)
{
	ScenarioStop(EScenarioStopReason::QuitApplication, EScenarioStopMode::StopSiganalOnly);

	CloseWindow(true);

	if (!bForce && SodaViewport.IsValid())
	{
		OpenWindow("Quit", SNew(soda::SSaveAllWindow, soda::ESaveAllWindowMode::Quit, true));
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

void USodaGameModeComponent::RequestRestartLevel(bool bForce)
{
	ScenarioStop(EScenarioStopReason::QuitApplication, EScenarioStopMode::StopSiganalOnly);

	CloseWindow(true);

	if (!bForce && SodaViewport.IsValid())
	{
		OpenWindow("Restart Level", SNew(soda::SSaveAllWindow, soda::ESaveAllWindowMode::Restart, true));
	}
	else
	{
		//Delay before exiting to end the current scenario if it was running
		GetWorld()->GetTimerManager().SetTimer(TimerHandle, [this]()
		{
			UGameplayStatics::OpenLevel(this, *UGameplayStatics::GetCurrentLevelName(this, true), false);
		}, 0.2, false);
	}
}

TSharedPtr<soda::SMessageBox> USodaGameModeComponent::ShowMessageBox(soda::EMessageBoxType Type, const FString& Caption, const FString& Text)
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

TSharedPtr<soda::SMenuWindow> USodaGameModeComponent::OpenWindow(const FString& Caption, TSharedRef<soda::SMenuWindowContent> Content)
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

bool USodaGameModeComponent::CloseWindow(soda::SMenuWindow * Wnd)
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


void USodaGameModeComponent::NotifyLevelIsChanged()
{

}

ASodaActorFactory* USodaGameModeComponent::GetActorFactory()
{
	check(LevelState);
	return LevelState->ActorFactory;
}

const FSodaActorDescriptor& USodaGameModeComponent::GetSodaActorDescriptor(TSoftClassPtr<AActor> Class) const
{
	if (const FSodaActorDescriptor* Desc = SodaActorDescriptors.Find(Class))
	{
		return *Desc;
	}

	static FSodaActorDescriptor Default;
	return Default;
}

const TMap<TSoftClassPtr<AActor>, FSodaActorDescriptor>& USodaGameModeComponent::GetSodaActorDescriptors() const
{
	return SodaActorDescriptors;
}

void USodaGameModeComponent::UpdateDBGateway(bool bSync)
{
	if (soda::FDBGateway::Instance().GetStatus() == soda::EDBGatewayStatus::Connecting)
	{
		ShowMessageBox(soda::EMessageBoxType::OK, "Faild", "The connecting is already in progress");
		return;
	}

	soda::FDBGateway::Instance().Configure(SodaApp.GetSodaUserSettings()->MongoURL, SodaApp.GetSodaUserSettings()->DatabaseName, bSync);
}

TSharedPtr<soda::SWaitingPanel> USodaGameModeComponent::ShowWaitingPanel(const FString& Caption, const FString& SubCaption)
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

bool USodaGameModeComponent::CloseWaitingPanel(soda::SWaitingPanel* InWaitingPanel = nullptr)
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

bool USodaGameModeComponent::CloseWaitingPanel(bool bCloseAll)
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

