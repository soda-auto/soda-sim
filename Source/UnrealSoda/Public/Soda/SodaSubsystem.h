// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "Subsystems/WorldSubsystem.h"
#include "Blueprint/UserWidget.h"
#include "Soda/SodaTypes.h"
#include "Soda/ISodaActor.h"
#include "SodaSubsystem.generated.h"

#define GPS_EPOCH_OFFSET 315964800
#define GPS_LEAP_SECONDS_OFFSET 17

class ASodaVehicle;
class ALevelState;
class ASodaActorFactory;
class UPinnedToolActorsSaveGame;

namespace soda
{
	class SSodaViewport;
	class IDatasetManager;
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
 * USodaSubsystem
 * USodaSubsystem must be added to the AGameMode to support UnrealSoda plugin
 */
UCLASS(ClassGroup = Soda, config = SODA, defaultconfig)
class UNREALSODA_API USodaSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = SodaSubsystem)
	TSubclassOf <ASodalSpectator> SpectatorClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = SodaSubsystem)
	TSubclassOf <ALevelState> DefaultLevelStateClass;

	UPROPERTY(BlueprintReadOnly, Category = SodaSubsystem)
	ASodaVehicle* UnpossesVehicle = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = SodaSubsystem)
	ALevelState * LevelState = nullptr;

	UPROPERTY(BlueprintReadWrite, Transient, DuplicateTransient, Category = SodaSubsystem)
	ASodalSpectator* SpectatorActor = nullptr;

	UPROPERTY(BlueprintReadWrite, Category = SodaSubsystem)
	TMap<UClass*, UPinnedToolActorsSaveGame*> PinnedToolActorsSaveGame;

	UPROPERTY(BlueprintReadOnly, Category = SodaSubsystem)
	USodaGameViewportClient* ViewportClient = nullptr;

	UPROPERTY(BlueprintAssignable, Category = SodaSubsystem)
	FScenarioPlaySignature OnScenarioPlay;

	UPROPERTY(BlueprintAssignable, Category = SodaSubsystem)
	FScenarioStopSignature OnScenarioStop;

public:
	void PushToolBox(TSharedRef<soda::SToolBox> Widget, bool InstedPrev = false);

	UFUNCTION(BlueprintCallable, Category = SodaSubsystem)
	bool PopToolBox();

	UFUNCTION(BlueprintCallable, Category = SodaSubsystem)
	bool CloseWindow(bool bCloseAllWindows = false);

	UFUNCTION(BlueprintCallable, Category = SodaSubsystem)
	void SetSpectatorMode(bool bSpectatorMode);

	UFUNCTION(BlueprintCallable, Category = SodaSubsystem)
	ASodaVehicle* GetActiveVehicle();

	UFUNCTION(BlueprintCallable, Category = SodaSubsystem)
	void SetActiveVehicle(ASodaVehicle * Vehicle);

	UFUNCTION(BlueprintCallable, Category = SodaSubsystem)
	ASodalSpectator* GetSpectatorActor() const { return SpectatorActor; }

	UFUNCTION(BlueprintCallable, Category = Scenario)
	bool ScenarioPlay();

	UFUNCTION(BlueprintCallable, Category = Scenario)
	void ScenarioStop(EScenarioStopReason Reason, EScenarioStopMode Mode, const FString & UserMessage=TEXT(""));

	UFUNCTION(BlueprintCallable, Category = Scenario)
	bool IsScenarioRunning() { return bIsScenarioRunning; }

	UFUNCTION(BlueprintCallable, Category = SodaSubsystem)
	void RequestQuit(bool bForce = false);

	UFUNCTION(BlueprintCallable, Category = SodaSubsystem)
	void RequestRestartLevel(bool bForce = false);

	UFUNCTION(BlueprintCallable, Category = SodaSubsystem)
	void NotifyLevelIsChanged();

	UFUNCTION(BlueprintCallable, Category = SodaSubsystem)
	ASodaActorFactory * GetActorFactory();

	UFUNCTION(BlueprintCallable, Category = SodaSubsystem)
	const TSet<AActor*>& GetSodaActors() const { return SodaActors; }

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
	static USodaSubsystem* Get();
	static USodaSubsystem* GetChecked();

public:
	USodaSubsystem();

	// UTickableWorldSubsystem implementation Begin
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Tick(float DeltaTime) override;
	virtual void PostInitialize() override;
	virtual void Deinitialize() override;
	virtual bool IsTickable() const override { return true; }
	virtual bool IsTickableInEditor() const override { return true; }
	virtual TStatId GetStatId() const override;
	// UTickableWorldSubsystem implementation End

protected:
	virtual void InitGame(AGameModeBase* GameMode);
	virtual void PreEndGame(UWorld* world);
	virtual bool OnWindowCloseRequested();

	void OnPostGarbageCollect(float Delay);
	void AfterScenarioStop();

	void OnActorSpawned(AActor* InActor);
	void OnActorDestroyed(AActor* InActor);

protected:
	bool bIsScenarioRunning = false;
	FDelegateHandle InitGameHandle;
	FDelegateHandle PreEndGameHandle;
	FDelegateHandle ActorSpawnedDelegateHandle;
	FDelegateHandle ActorDestroyedDelegateHandle;
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

	UPROPERTY()
	TSet<AActor*> SodaActors;
};
