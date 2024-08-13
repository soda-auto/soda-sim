// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "GameFramework/SaveGame.h"
#include "Blueprint/UserWidget.h"
#include "Soda/SodaTypes.h"
#include "Soda/ISodaActor.h"
#include "SodaGameMode.generated.h"

#define GPS_EPOCH_OFFSET 315964800
#define GPS_LEAP_SECONDS_OFFSET 17

class ASodaVehicle;
class ALevelState;
class ASodaActorFactory;
class UPinnedToolActorsSaveGame;

namespace soda
{
	class SSodaViewport;
	class SMenuWindowContent;
	class SMessageBox;
	class SMenuWindow;
	class SToolBox;
	class SWaitingPanel;
	enum class EMessageBoxType : uint8;
	enum class EUIMode : uint8;
}

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FScenarioPlaySignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FScenarioStopSignature, EScenarioStopReason, Reason);


/*
 * USodaGameModeComponent
 * USodaGameModeComponent must be added to the AGameMode to support UnrealSoda plugin
 */
UCLASS(ClassGroup = Soda, BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class UNREALSODA_API USodaGameModeComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = SodaGameMode)
	TSubclassOf <ASodalSpectator> SpectatorClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = SodaGameMode)
	TSubclassOf <ALevelState> DefaultLevelStateClass;

	UPROPERTY(BlueprintReadOnly, Category = SodaGameMode)
	ASodaVehicle* UnpossesVehicle = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = SodaGameMode)
	ALevelState * LevelState = nullptr;

	UPROPERTY(BlueprintReadWrite, Transient, DuplicateTransient, Category = SodaGameMode)
	ASodalSpectator* SpectatorActor = nullptr;

	UPROPERTY(BlueprintReadWrite, Category = SodaGameMode)
	TMap<UClass*, UPinnedToolActorsSaveGame*> PinnedToolActorsSaveGame;

	UPROPERTY(BlueprintReadOnly, Category = SodaGameMode)
	USodaGameViewportClient* ViewportClient = nullptr;

	UPROPERTY(BlueprintAssignable, Category = SodaGameMode)
	FScenarioPlaySignature OnScenarioPlay;

	UPROPERTY(BlueprintAssignable, Category = SodaGameMode)
	FScenarioStopSignature OnScenarioStop;

public:
	UFUNCTION(BlueprintCallable, Category = DB)
	virtual void UpdateDBGateway(bool bSync);

	UFUNCTION(BlueprintCallable, Category = DB, meta = (CallInRuntime))
	virtual void ReconnectDBGateway() { UpdateDBGateway(false); }


	void PushToolBox(TSharedRef<soda::SToolBox> Widget, bool InstedPrev = false);

	UFUNCTION(BlueprintCallable, Category = SodaGameMode)
	bool PopToolBox();

	UFUNCTION(BlueprintCallable, Category = SodaGameMode)
	bool CloseWindow(bool bCloseAllWindows = false);

	UFUNCTION(BlueprintCallable, Category = SodaGameMode)
	void SetSpectatorMode(bool bSpectatorMode);

	UFUNCTION(BlueprintCallable, Category = SodaGameMode)
	ASodaVehicle* GetActiveVehicle();

	UFUNCTION(BlueprintCallable, Category = SodaGameMode)
	void SetActiveVehicle(ASodaVehicle * Vehicle);

	UFUNCTION(BlueprintCallable, Category = SodaGameMode)
	ASodalSpectator* GetSpectatorActor() const { return SpectatorActor; }

	UFUNCTION(BlueprintCallable, Category = Scenario)
	bool ScenarioPlay();

	UFUNCTION(BlueprintCallable, Category = Scenario)
	void ScenarioStop(EScenarioStopReason Reason, EScenarioStopMode Mode, const FString & UserMessage=TEXT(""));

	/** Valid only during scenario playing. -1 means no dataset recording in this scenario */
	UFUNCTION(BlueprintCallable, Category = Scenario)
	int64 GetScenarioID() const;

	UFUNCTION(BlueprintCallable, Category = Scenario)
	bool IsScenarioRunning() { return bIsScenarioRunning; }

	UFUNCTION(BlueprintCallable, Category = SodaGameMode)
	void RequestQuit(bool bForce = false);

	UFUNCTION(BlueprintCallable, Category = SodaGameMode)
	void RequestRestartLevel(bool bForce = false);

	UFUNCTION(BlueprintCallable, Category = SodaGameMode)
	void NotifyLevelIsChanged();

	UFUNCTION(BlueprintCallable, Category = SodaGameMode)
	ASodaActorFactory * GetActorFactory();

	const FSodaActorDescriptor& GetSodaActorDescriptor(TSoftClassPtr<AActor> Class) const;
	const TMap<TSoftClassPtr<AActor>, FSodaActorDescriptor> & GetSodaActorDescriptors() const;

	TSharedPtr<soda::SMessageBox> ShowMessageBox(soda::EMessageBoxType Type, const FString& Caption, const FString& Text);
	TSharedPtr<soda::SMenuWindow> OpenWindow(const FString& Caption, TSharedRef<soda::SMenuWindowContent> Content);
	bool CloseWindow(soda::SMenuWindow * Wnd);

	TSharedPtr<soda::SWaitingPanel> ShowWaitingPanel(const FString& Caption, const FString& SubCaption);
	bool CloseWaitingPanel(soda::SWaitingPanel * WaitingPanel);
	bool CloseWaitingPanel(bool bCloseAll = false);

	soda::SSodaViewport* GetSodaViewport() const { return SodaViewport.Get(); }
	
public:
	static USodaGameModeComponent* Get();
	static USodaGameModeComponent* GetChecked();

public:
	USodaGameModeComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	//virtual void InitializeComponent() override;
	//virtual void UninitializeComponent() override;
	virtual void OnComponentCreated() override;
	virtual void OnComponentDestroyed(bool bDestroyingHierarchy) override;

protected:
	virtual void InitGame(AGameModeBase* GameMode);
	virtual void PreEndGame(UWorld* world);
	virtual bool OnWindowCloseRequested();

	void OnPostGarbageCollect(float Delay);
	void AfterScenarioStop();

protected:
	bool bIsScenarioRunning = false;
	FDelegateHandle InitGameHandle;
	FDelegateHandle PreEndGameHandle;
	FTimerHandle TimerHandle;
	TMap<TSoftClassPtr<AActor>, FSodaActorDescriptor> SodaActorDescriptors;
	TSharedPtr<soda::SSodaViewport> SodaViewport;

	struct FScenarioLevelSavedData
	{
		//TArray<uint8> Memory{};
		TArray<FString> ActorsDirty{};
		soda::EUIMode Mode{};
		FString SelectedActor{};
		FString PossesdActor{};
		bool bIsValid = false;
		FString UserMessage;
		//inline const bool IsValid() { return Memory.Num(); }
	};
	static FScenarioLevelSavedData ScenarioLevelSavedData;
};
