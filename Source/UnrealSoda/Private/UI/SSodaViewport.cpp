// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/UI/SSodaViewport.h"
#include "Misc/Paths.h"
#include "Framework/Commands/UICommandList.h"
#include "Misc/App.h"
#include "EngineGlobals.h"
#include "Engine/TextureStreamingTypes.h"
#include "Engine/Engine.h"
#include "Slate/SceneViewport.h"
#include "Slate/SGameLayerManager.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Input/SSpinBox.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/Layout/SSplitter.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Colors/SComplexGradient.h"
#include "Widgets/LayerManager/STooltipPresenter.h"
#include "Widgets/Layout/SPopup.h"
#include "Widgets/SWeakWidget.h"
#include "SodaStyleSet.h"
#include "Soda/UI/SToolBox.h"
#include "Soda/UI/SodaViewportCommands.h"
#include "UI/Toolbar/SodaDragDropOp.h"
#include "UI/Toolbar/SSodaViewportToolBar.h"
#include "UI/Toolbar/SPlacementModeTools.h"
#include "Soda/UI/SMessageBox.h"
#include "Soda/UI/SMenuWindow.h"
#include "UI/Wnds/SSaveAllWindow.h"
#include "Soda/UI/SWaitingPanel.h"
#include "UI/SOutputLog.h"
#include "Soda/SodaActorFactory.h"
#include "Soda/SodaSpectator.h"
#include "Soda/SodaGameMode.h"
#include "Soda/SodaApp.h"
#include "Soda/SodaUserSettings.h"
#include "Soda/Editor/SodaSelection.h"
#include "Soda/SodaGameViewportClient.h"
#include "Soda/SodaHUD.h"
#include "GameFramework/GameUserSettings.h"
#include "Soda/Misc/EditorUtils.h"
#include "Framework/Commands/UICommandInfo.h"
#include "UI/ToolBoxes/SActorToolBox.h"
#include "Soda/LevelState.h"
#include "EngineUtils.h"
#include "Soda/SodaStatics.h"

#define LOCTEXT_NAMESPACE "SodaViewport"

namespace soda
{

class SToolBoxBorder : public SBorder
{
public:
	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
	{
		return FReply::Handled();
	}
	virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
	{
		return FReply::Handled();
	}
	//virtual FReply OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
	//{
		//return FReply::Handled();
	//}
	virtual FReply OnMouseWheel(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
	{
		return FReply::Handled();
	}
};

SSodaViewport::SSodaViewport()
{
}

SSodaViewport::~SSodaViewport()
{
}

void SSodaViewport::Construct( const FArguments& InArgs, UWorld* InWorld)
{
	check(InWorld);
	World = InWorld;
	ViewportClient = Cast<USodaGameViewportClient>(InWorld->GetGameViewport());
	check(ViewportClient.IsValid());
	Viewport = ViewportClient->Viewport;
	check(Viewport);

	CommandList = MakeShareable(new FUICommandList);
	BindCommands();

	ChildSlot
	[
		SNew(SOverlay)
		+ SOverlay::Slot()
		[
			SAssignNew(MajorSplitter, SSplitter)
			//.VAlign(VAlign_Fill)
			//.HAlign(HAlign_Fill)
			.Orientation(EOrientation::Orient_Horizontal)
			+SSplitter::Slot()
			.Value(1 - ToolBoxSize)
			[
				SAssignNew(MinorSplitter, SSplitter)
				.Orientation(EOrientation::Orient_Vertical)
				+ SSplitter::Slot()
				.Value(0.8)
				[
					MakeViewportToolbar().ToSharedRef()
				]
			]
		]
		+ SOverlay::Slot()
		[
			SAssignNew(ViewportOverlayWidget, SOverlay)
		]
		+ SOverlay::Slot()
		[
			SNew(SPopup)
			[
				SAssignNew(TooltipPresenter, STooltipPresenter)
			]
		]
	];

}

bool SSodaViewport::OnVisualizeTooltip(const TSharedPtr<SWidget>& TooltipContent)
{
	TooltipPresenter->SetContent(TooltipContent); 

	return true;
}

FReply SSodaViewport::OnKeyDown( const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent )
{
	FReply Reply = FReply::Unhandled();
	if( CommandList->ProcessCommandBindings( InKeyEvent ) )
	{
		return FReply::Handled();
	}
	return SResetScaleBox::OnKeyDown(MyGeometry, InKeyEvent);		
}

void SSodaViewport::Tick( const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime )
{
	SResetScaleBox::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);
}

bool SSodaViewport::IsLogVisible() const
{
	return OutputLog.IsValid();
}

void SSodaViewport::SetLogVisible(bool bVisible)
{
	if (bVisible)
	{
		if (!OutputLog.IsValid())
		{
			if (MinorSplitter->GetChildren()->Num() == 1)
			{
				MinorSplitter->AddSlot()
				//.Value(0.3)
				[
					SNew(SToolBoxBorder)
					.BorderImage(FSodaStyle::GetBrush(TEXT("Brushes.Recessed")))
					[
						SAssignNew(OutputLog, SOutputLog, false)
						.Messages(SodaApp.GetOutputLogHistory()->GetMessages())
					]
				];
			}
		}
	}
	else
	{
		if (MinorSplitter->GetChildren()->Num() == 2)
		{
			ToolBoxSize = MinorSplitter->SlotAt(1).GetSizeValue();
			MinorSplitter->RemoveAt(1);
		}
		OutputLog.Reset();
	}
}

void SSodaViewport::BindCommands()
{
	FUICommandList& CommandListRef = *CommandList;

	const FSodalViewportCommands& Commands = FSodalViewportCommands::Get();

	USodaGameViewportClient* ClientRef = ViewportClient.Get();

	CommandListRef.MapAction( 
		Commands.ToggleFPS,
		FExecuteAction::CreateSP(this, &SSodaViewport::ToggleStatCommand, FString("FPS")),
		FCanExecuteAction(),
		FIsActionChecked::CreateSP(this, &SSodaViewport::IsStatCommandVisible, FString("FPS"))
	);

	CommandListRef.MapAction(
		Commands.ToggleVehiclePanel,
		FExecuteAction::CreateLambda([World=GetWorld()]()
		{
			if (IsValid(World))
			{
				if (ASodaHUD* SodaHUD = World->GetFirstPlayerController()->GetHUD<ASodaHUD>())
				{
					SodaHUD->bDrawVehicleDebugPanel = !SodaHUD->bDrawVehicleDebugPanel;
				}
			}
		}),
		FCanExecuteAction(),
		FIsActionChecked::CreateLambda([World=GetWorld()]()
		{
			if (IsValid(World))
			{
				if (ASodaHUD* SodaHUD = World->GetFirstPlayerController()->GetHUD<ASodaHUD>())
				{
					return SodaHUD->bDrawVehicleDebugPanel;
				}
			}
			return false;
		})
	);

	CommandListRef.MapAction(
		Commands.ToggleFullScreen,
		FExecuteAction::CreateLambda([]()
		{
			if (UGameUserSettings* GameUserSettings = UGameUserSettings::GetGameUserSettings())
			{
				EWindowMode::Type Mode = GameUserSettings->GetFullscreenMode();
				bool bIsFullscreen = (Mode == EWindowMode::Fullscreen || Mode == EWindowMode::WindowedFullscreen);
				GameUserSettings->SetFullscreenMode(bIsFullscreen ? EWindowMode::Windowed : EWindowMode::WindowedFullscreen);
			}
		}),
		FCanExecuteAction(),
		FIsActionChecked::CreateLambda([]()
		{
			if (UGameUserSettings* GameUserSettings = UGameUserSettings::GetGameUserSettings())
			{
				EWindowMode::Type Mode = GameUserSettings->GetFullscreenMode();
				bool bIsFullscreen = (Mode == EWindowMode::Fullscreen || Mode == EWindowMode::WindowedFullscreen);
				return bIsFullscreen;
			}
			return false;
		})
	);

	CommandListRef.MapAction( 
		Commands.Perspective,
		FExecuteAction::CreateSP(this, &SSodaViewport::SetViewportType, ESodaSpectatorMode::Perspective),
		FCanExecuteAction(),
		FIsActionChecked::CreateSP(this, &SSodaViewport::IsActiveViewportType, ESodaSpectatorMode::Perspective));

	CommandListRef.MapAction(
		Commands.OrthographicFree,
		FExecuteAction::CreateSP(this, &SSodaViewport::SetViewportType, ESodaSpectatorMode::OrthographicFree),
		FCanExecuteAction(),
		FIsActionChecked::CreateSP(this, &SSodaViewport::IsActiveViewportType, ESodaSpectatorMode::OrthographicFree));

	CommandListRef.MapAction( 
		Commands.Front,
		FExecuteAction::CreateSP(this, &SSodaViewport::SetViewportType, ESodaSpectatorMode::Front),
		FCanExecuteAction(),
		FIsActionChecked::CreateSP(this, &SSodaViewport::IsActiveViewportType, ESodaSpectatorMode::Front));

	CommandListRef.MapAction( 
		Commands.Left,
		FExecuteAction::CreateSP(this, &SSodaViewport::SetViewportType, ESodaSpectatorMode::Left),
		FCanExecuteAction(),
		FIsActionChecked::CreateSP(this, &SSodaViewport::IsActiveViewportType, ESodaSpectatorMode::Left));

	CommandListRef.MapAction( 
		Commands.Top,
		FExecuteAction::CreateSP(this, &SSodaViewport::SetViewportType, ESodaSpectatorMode::Top),
		FCanExecuteAction(),
		FIsActionChecked::CreateSP(this, &SSodaViewport::IsActiveViewportType, ESodaSpectatorMode::Top));

	CommandListRef.MapAction(
		Commands.Back,
		FExecuteAction::CreateSP(this, &SSodaViewport::SetViewportType, ESodaSpectatorMode::Back),
		FCanExecuteAction(),
		FIsActionChecked::CreateSP(this, &SSodaViewport::IsActiveViewportType, ESodaSpectatorMode::Back));

	CommandListRef.MapAction(
		Commands.Right,
		FExecuteAction::CreateSP(this, &SSodaViewport::SetViewportType, ESodaSpectatorMode::Right),
		FCanExecuteAction(),
		FIsActionChecked::CreateSP(this, &SSodaViewport::IsActiveViewportType, ESodaSpectatorMode::Right));

	CommandListRef.MapAction(
		Commands.Bottom,
		FExecuteAction::CreateSP(this, &SSodaViewport::SetViewportType, ESodaSpectatorMode::Bottom),
		FCanExecuteAction(),
		FIsActionChecked::CreateSP(this, &SSodaViewport::IsActiveViewportType, ESodaSpectatorMode::Bottom));

	CommandListRef.MapAction(
		Commands.SelectMode,
		FExecuteAction::CreateUObject(ClientRef, &USodaGameViewportClient::SetWidgetMode, soda::WM_None),
		FCanExecuteAction::CreateSP(this, &SSodaViewport::CanSetWidgetMode, soda::WM_None),
		FIsActionChecked::CreateSP(this, &SSodaViewport::IsWidgetModeActive, soda::WM_None)
	);

	CommandListRef.MapAction(
		Commands.TranslateMode,
		FExecuteAction::CreateUObject(ClientRef, &USodaGameViewportClient::SetWidgetMode, soda::WM_Translate ),
		FCanExecuteAction::CreateSP(this, &SSodaViewport::CanSetWidgetMode, soda::WM_Translate ),
		FIsActionChecked::CreateSP( this, &SSodaViewport::IsWidgetModeActive, soda::WM_Translate )
	);

	CommandListRef.MapAction( 
		Commands.RotateMode,
		FExecuteAction::CreateUObject(ClientRef, &USodaGameViewportClient::SetWidgetMode, soda::WM_Rotate ),
		FCanExecuteAction::CreateSP(this, &SSodaViewport::CanSetWidgetMode, soda::WM_Rotate ),
		FIsActionChecked::CreateSP( this, &SSodaViewport::IsWidgetModeActive, soda::WM_Rotate )
	);
		
	CommandListRef.MapAction( 
		Commands.ScaleMode,
		FExecuteAction::CreateUObject(ClientRef, &USodaGameViewportClient::SetWidgetMode, soda::WM_Scale ),
		FCanExecuteAction::CreateSP(this, &SSodaViewport::CanSetWidgetMode, soda::WM_Scale ),
		FIsActionChecked::CreateSP( this, &SSodaViewport::IsWidgetModeActive, soda::WM_Scale )
	);
	
	CommandListRef.MapAction( 
		Commands.RelativeCoordinateSystem_World,
		FExecuteAction::CreateUObject( ClientRef, &USodaGameViewportClient::SetWidgetCoordSystemSpace, soda::COORD_World ),
		FCanExecuteAction(),
		FIsActionChecked::CreateSP( this, &SSodaViewport::IsCoordSystemActive, soda::COORD_World )
	);

	CommandListRef.MapAction( 
		Commands.RelativeCoordinateSystem_Local,
		FExecuteAction::CreateUObject( ClientRef, &USodaGameViewportClient::SetWidgetCoordSystemSpace, soda::COORD_Local ),
		FCanExecuteAction(),
		FIsActionChecked::CreateSP( this, &SSodaViewport::IsCoordSystemActive, soda::COORD_Local )
	);
	
	CommandListRef.MapAction(
		Commands.HideAllStats,
		FExecuteAction::CreateSP(this, &SSodaViewport::OnToggleAllStatCommands, false));

	CommandListRef.MapAction(
		Commands.RestartLevel,
		FExecuteAction::CreateLambda([]()
		{
			if (USodaGameModeComponent* GameMode = USodaGameModeComponent::GetChecked())
			{
				GameMode->RequestRestartLevel(false);
			}
		})
	);

	CommandListRef.MapAction(
		Commands.ClearLevel,
		FExecuteAction::CreateLambda([&]()
		{
			if (ALevelState* LevelState = ALevelState::Get())
			{
				LevelState->ReloadLevelEmpty(GetWorld());
			}
		})
	);

	CommandListRef.MapAction(
		Commands.AutoConnectDB,
		FExecuteAction::CreateLambda([]()
		{
			SodaApp.GetSodaUserSettings()->bAutoConnect = !SodaApp.GetSodaUserSettings()->bAutoConnect;
			SodaApp.GetSodaUserSettings()->SaveSettings();
		}),
		FCanExecuteAction(),
		FIsActionChecked::CreateLambda([]()
		{
			return SodaApp.GetSodaUserSettings()->bAutoConnect;
		})
	);

	CommandListRef.MapAction(
		Commands.RecordDataset,
		FExecuteAction::CreateLambda([]()
		{
			SodaApp.GetSodaUserSettings()->bRecordDataset = !SodaApp.GetSodaUserSettings()->bRecordDataset;
			SodaApp.GetSodaUserSettings()->SaveSettings();
		}),
		FCanExecuteAction(),
		FIsActionChecked::CreateLambda([]()
		{
			return SodaApp.GetSodaUserSettings()->bRecordDataset;
		})
	);

	CommandListRef.MapAction(
		Commands.ToggleSpectatorMode,
		FExecuteAction::CreateLambda([]()
		{
			if (USodaGameModeComponent* GameMode = USodaGameModeComponent::Get())
			{
				APlayerController* PlayerController = GameMode->GetWorld()->GetFirstPlayerController();
				check(PlayerController);

				APawn* Pawn = PlayerController->GetPawn();

				if (Cast<ASodaVehicle>(Pawn))
				{
					GameMode->SetSpectatorMode(true);
				}
				else if (Cast<ASodalSpectator>(Pawn))
				{
					GameMode->SetSpectatorMode(false);
				}
				else
				{
					GameMode->SetSpectatorMode(true);
				}
			}
		})
	);

	CommandListRef.MapAction(
		Commands.PossesNextVehicle,
		FExecuteAction::CreateLambda([]()
		{
			if (USodaGameModeComponent* GameMode = USodaGameModeComponent::Get())
			{
				APlayerController* PlayerController = GameMode->GetWorld()->GetFirstPlayerController();
				check(PlayerController);

				ASodaVehicle* Vehicle = GameMode->GetActiveVehicle();
				if (Vehicle)
				{
					for (TActorIterator<ASodaVehicle> It(GameMode->GetWorld()); It; ++It)
					{
						if (*It == Vehicle)
						{
							++It;
							if (It)
							{
								GameMode->SetActiveVehicle(*It);
							}
							else
							{
								GameMode->SetActiveVehicle(*TActorIterator<ASodaVehicle>(GameMode->GetWorld()));
							}
							break;
						}
					}
				}
				else
				{
					TActorIterator<ASodaVehicle> It(GameMode->GetWorld());
					if (It)
					{
						GameMode->SetActiveVehicle(*It);
					}
				}
			}
		})
	);

	CommandListRef.MapAction(
		Commands.BackMenu,
		FExecuteAction::CreateSP(this, &SSodaViewport::BackMenu)
	);

#if WITH_EDITOR
	CommandListRef.MapAction(
		Commands.BackMenuEditor,
		FExecuteAction::CreateSP(this, &SSodaViewport::BackMenu)
	);
#endif

	CommandListRef.MapAction(
		Commands.TagActors,
		FExecuteAction::CreateLambda([this]()
		{
			USodaStatics::TagActorsInLevel(GetWorld(), true);
			USodaStatics::AddV2XMarkerToAllVehiclesInLevel(GetWorld(), ASodaVehicle::StaticClass());
		})
	);

	CommandListRef.MapAction(
		Commands.Exit,
		FExecuteAction::CreateLambda([]()
		{
			if (USodaGameModeComponent* GameMode = USodaGameModeComponent::Get())
			{
				GameMode->RequestQuit();
			}
		})
	);

	for (auto StatCatIt = Commands.ShowStatCatCommands.CreateConstIterator(); StatCatIt; ++StatCatIt)
	{
		const TArray< FSodalViewportCommands::FShowMenuCommand >& ShowStatCommands = StatCatIt.Value();
		for (int32 StatIndex = 0; StatIndex < ShowStatCommands.Num(); ++StatIndex)
		{
			const FSodalViewportCommands::FShowMenuCommand& StatCommand = ShowStatCommands[StatIndex];
			BindStatCommand(StatCommand.ShowMenuItem, StatCommand.LabelOverride.ToString());
		}
	}

	// Bind a listener here for any additional stat commands that get registered later.
	FSodalViewportCommands::NewStatCommandDelegate.AddRaw(this, &SSodaViewport::BindStatCommand);
	
	CommandListRef.MapAction(
		Commands.CycleTransformGizmoCoordSystem,
		FExecuteAction::CreateSP( this, &SSodaViewport::OnCycleCoordinateSystem )
		);

}

void SSodaViewport::BindStatCommand(const TSharedPtr<FUICommandInfo> InMenuItem, const FString& InCommandName)
{
	CommandList->MapAction(
		InMenuItem,
		FExecuteAction::CreateSP(this, &SSodaViewport::ToggleStatCommand, InCommandName),
		FCanExecuteAction(),
		FIsActionChecked::CreateSP(this, &SSodaViewport::IsStatCommandVisible, InCommandName));
}

void SSodaViewport::BackMenu()
{
	if (USodaGameModeComponent* GameMode = USodaGameModeComponent::Get())
	{
		if (!GameMode->CloseWindow())
		{
			GameMode->PopToolBox();
		}
	}
}

void SSodaViewport::OnCycleCoordinateSystem()
{
	ViewportClient->SetWidgetCoordSystemSpace(ViewportClient->GetWidgetCoordSystemSpace() == soda::COORD_World ? soda::COORD_Local : soda::COORD_World);
}

void SSodaViewport::OnToggleStats()
{
	/*
	bool bIsEnabled = ViewportClient->ShouldShowStats();
	ViewportClient->SetShowStats( !bIsEnabled );

	if( !bIsEnabled )
	{
		 // let the user know how they can enable stats via the console
		 FNotificationInfo Info(LOCTEXT("StatsEnableHint", "Stats display can be toggled via the STAT [type] console command"));
		 Info.ExpireDuration = 3.0f;

		 FSlateNotificationManager::Get().AddNotification(Info);
	}
	*/
}

void SSodaViewport::OnToggleAllStatCommands(bool bVisible)
{
	check(bVisible == 0);
	// If it's in the array, it's visible so just toggle it again
	const TArray<FString>* EnabledStats = ViewportClient->GetEnabledStats();
	check(EnabledStats);
	while (EnabledStats->Num() > 0)
	{
		const FString& CommandName = EnabledStats->Last();
		ToggleStatCommand(CommandName);
	}
}

void SSodaViewport::ToggleStatCommand(FString CommandName)
{
	GEngine->ExecEngineStat(GetWorld(), ViewportClient.Get(), *CommandName);
}

bool SSodaViewport::IsStatCommandVisible(FString CommandName) const
{
	return ViewportClient->IsStatEnabled(CommandName);
}

bool SSodaViewport::IsWidgetModeActive(soda::EWidgetMode Mode ) const
{
	return ViewportClient->GetWidgetMode() == Mode;
}

bool SSodaViewport::CanSetWidgetMode(soda::EWidgetMode NewMode) const
{
	return  true;
}

bool SSodaViewport::IsCoordSystemActive(ECoordSystem CoordSystem) const
{
	return ViewportClient->GetWidgetCoordSystemSpace() == CoordSystem;
}

void SSodaViewport::OnDragEnter(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent)
{
	if (HandleDragObjects(MyGeometry, DragDropEvent))
	{
		DragDropEvent.GetOperation()->SetDecoratorVisibility(false);
		if (!HandlePlaceDraggedObjects(MyGeometry, DragDropEvent, /*bCreateDropPreview=*/true))
		{
			//DragDropEvent.GetOperation()->SetDecoratorVisibility(true);
		}
	}
}

void SSodaViewport::OnDragLeave(const FDragDropEvent& DragDropEvent)
{
	if (ViewportClient->HasDropPreviewActor())
	{
		ViewportClient->DestroyDropPreviewActor();
	}

	TSharedPtr<FDragDropOperation> Operation = DragDropEvent.GetOperation();
	if (Operation.IsValid())
	{
		if (Operation->IsOfType<soda::FSodaDecoratedDragDropOp>())
		{
			TSharedPtr<soda::FSodaDecoratedDragDropOp> DragDropOp = StaticCastSharedPtr<soda::FSodaDecoratedDragDropOp>(Operation);
			DragDropOp->ResetToDefaultToolTip();
			Operation->SetDecoratorVisibility(false);
		}
	}
}

bool SSodaViewport::HandleDragObjects(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent)
{
	bool bValidDrag = false;

	TSharedPtr< FDragDropOperation > Operation = DragDropEvent.GetOperation();
	if (!Operation.IsValid())
	{
		return false;
	}

	if (Operation->IsOfType<FSodaActorDragDropOp>())
	{
		auto SodaActorDragDropOp = StaticCastSharedPtr<FSodaActorDragDropOp>(DragDropEvent.GetOperation());

		FVector2D ScreenPos;
		if (!FEditorUtils::AbsoluteToScreenPosition(ViewportClient.Get(), DragDropEvent.GetLastScreenSpacePosition(), ScreenPos))
		{
			return false;
		}

		if (ViewportClient->UpdateDropPreviewActor(ScreenPos.X, ScreenPos.Y))
		{
			//Operation->SetDecoratorVisibility(false);
			Operation->SetCursorOverride(TOptional<EMouseCursor::Type>());
		}
		else
		{
			Operation->SetCursorOverride(EMouseCursor::SlashedCircle);
		}

		//FText HintText = FText::FromString("***");
		//SodaActorDragDropOp->SetToolTip(HintText, nullptr);

		return true;
	}

	return false;
}

FReply SSodaViewport::OnDragOver(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent)
{
	if (HandleDragObjects(MyGeometry, DragDropEvent))
	{
		return FReply::Handled();
	}

	return FReply::Unhandled();
}

bool SSodaViewport::HandlePlaceDraggedObjects(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent, bool bCreateDropPreview)
{
	TSharedPtr< FDragDropOperation > Operation = DragDropEvent.GetOperation();
	if (!Operation.IsValid())
	{
		return false;
	}

	// Don't handle the placement if we couldn't handle the drag
	if (!HandleDragObjects(MyGeometry, DragDropEvent))
	{
		return false;
	}

	if (!Operation->IsOfType<FSodaActorDragDropOp>())
	{
		return false;
	}

	TSharedPtr<FSodaActorDragDropOp> DragDropOp = StaticCastSharedPtr<FSodaActorDragDropOp>(Operation);

	TSharedPtr<const FPlaceableItem> Item = DragDropOp->GetPlaceableItem();
	if (!Item)
	{
		return false;
	}
	
	FVector2D ScreenPos;
	if (!FEditorUtils::AbsoluteToScreenPosition(ViewportClient.Get(), DragDropEvent.GetLastScreenSpacePosition(), ScreenPos))
	{
		return false;
	}

	AActor* NewActor = nullptr;

	if (bCreateDropPreview)
	{
		ViewportClient->DropPreviewActorAtCoordinates(ScreenPos.X, ScreenPos.Y);
	}
	else
	{
		if (!Item->DefaultActor.IsNull())
		{
			UClass* Class = Item->DefaultActor.Get();
			if (!Class)
			{
				Class = LoadObject<UClass>(nullptr, *Item->DefaultActor.ToString());
			}

			if (!Class)
			{
				return false;
			}

			if (!ViewportClient->DropActorAtCoordinates(ScreenPos.X, ScreenPos.Y, Class, &NewActor, true))
			{
				//FNotificationInfo Info(LOCTEXT("Error_OperationDisall", "Operation failed"));
				//Info.ExpireDuration = 3.0f;
				//FSlateNotificationManager::Get().AddNotification(Info);
				return false;
			}
		}
		else if (Item->SpawnCastomActorDelegate.IsBound())
		{
			if (!ViewportClient->DropActorAtCoordinates(ScreenPos.X, ScreenPos.Y, Item->SpawnCastomActorDelegate, &NewActor, true))
			{
				//FNotificationInfo Info(LOCTEXT("Error_OperationDisall", "Operation failed"));
				//Info.ExpireDuration = 3.0f;
				//FSlateNotificationManager::Get().AddNotification(Info);
				return false;
			}
		}
		else
		{
			return false;
		}

	}

	if (!bCreateDropPreview && NewActor && ViewportClient->Selection)
	{
		ViewportClient->Selection->SelectActor(NewActor, nullptr);
	}

	return true;
}

FReply SSodaViewport::OnDrop(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent)
{
	bool bDroped = HandlePlaceDraggedObjects(MyGeometry, DragDropEvent, /*bCreateDropPreview=*/false);

	return bDroped ? FReply::Handled() : FReply::Unhandled();
}

TSharedPtr<SWidget> SSodaViewport::MakeViewportToolbar() 
{ 
	// Build our toolbar level toolbar
	TSharedRef< SSodaViewportToolBar > ToolBar =
		SNew( SSodaViewportToolBar )
		.Viewport( SharedThis( this ) )
		//.Visibility( this, &SSodaViewport::GetToolBarVisibility )
		.IsEnabled( FSlateApplication::Get().GetNormalExecutionAttribute() );


	return 
		SNew(SVerticalBox)
		.Visibility( EVisibility::SelfHitTestInvisible )
		+SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0, 1.0f, 0, 0)
		.VAlign(VAlign_Top)
		[
			ToolBar
		];
}

void SSodaViewport::SetToolBoxWidget(TSharedRef<SToolBox> InWidget)
{
	RemoveToolBoxWidget();

	if (MajorSplitter->GetChildren()->Num() == 1)
	{
		MajorSplitter->AddSlot()
		.Value(ToolBoxSize)
		[
			SNew(SToolBoxBorder)
			.Padding(FMargin(0, 0, 4, 4))
			.BorderImage(FSodaStyle::GetBrush(TEXT("Brushes.Recessed")))
			.Cursor(EMouseCursor::Default)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.Padding(5)
					[
						SNew(STextBlock)
						.Text(InWidget->GetCaption())
					]
					.FillWidth(1.0)
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					.HAlign(HAlign_Center)
					[
						SNew(SButton)
						.ButtonStyle(FSodaStyle::Get(), "MenuWindow.CloseButton")
						.OnClicked(FOnClicked::CreateLambda([this]() 
						{
							if(USodaGameModeComponent* GameMode = USodaGameModeComponent::Get())
							{
								GameMode->PopToolBox();
							}
							return FReply::Handled();
						}))
					]
				
				]
				+ SVerticalBox::Slot()
				.FillHeight(1)
				[ InWidget  ]
			]

		];
	}
}

void SSodaViewport::RemoveToolBoxWidget()
{
	if (MajorSplitter->GetChildren()->Num() == 2)
	{
		ToolBoxSize = MajorSplitter->SlotAt(1).GetSizeValue();
		MajorSplitter->RemoveAt(1);
	}
}

void SSodaViewport::SetViewportType(ESodaSpectatorMode InMode)
{
	USodaGameModeComponent* GameMode = USodaGameModeComponent::Get();
	if (GameMode && GameMode->SpectatorActor)
	{
		GameMode->SpectatorActor->SetMode(InMode);
	}
}

bool SSodaViewport::IsActiveViewportType(ESodaSpectatorMode InMode)
{
	USodaGameModeComponent* GameMode = USodaGameModeComponent::Get();
	if (GameMode && GameMode->SpectatorActor)
	{
		return GameMode->SpectatorActor->GetMode() == InMode;
	}
	return false;
}

void SSodaViewport::AddViewportWidgetContent(TSharedRef<SWidget> ViewportContent, const int32 ZOrder)
{
	if (ViewportOverlayWidget.IsValid())
	{
		ViewportOverlayWidget->AddSlot(ZOrder)
		[
			ViewportContent
		];
	}
}

void SSodaViewport::RemoveViewportWidgetContent(TSharedRef<SWidget> ViewportContent)
{
	if (ViewportOverlayWidget.IsValid())
	{
		ViewportOverlayWidget->RemoveSlot(ViewportContent);
	}
}

void SSodaViewport::SetUIMode(soda::EUIMode InMode)
{
	UIMode = InMode;
	StackToolBox.Empty();

	switch (UIMode)
	{
	case soda::EUIMode::Free:
		RemoveToolBoxWidget();
		break;
	case soda::EUIMode::Editing:
		PushToolBox(SNew(soda::SActorToolBox).Viewport(SharedThis(this)));
		break;
	}
	
	if (ViewportClient.IsValid())
	{
		ViewportClient->Invalidate();
	}
}

soda::EUIMode SSodaViewport::GetUIMode() const
{
	return UIMode;
}

void SSodaViewport::PushToolBox(TSharedRef<soda::SToolBox> Widget, bool InstedPrev)
{
	if (StackToolBox.Num() && InstedPrev)
	{
		StackToolBox.Pop();
	}
	StackToolBox.Add(Widget);
	SetToolBoxWidget(Widget);
}

bool SSodaViewport::PopToolBox()
{
	if (StackToolBox.Num() == 0)
	{
		return false;
	}

	StackToolBox.Pop()->OnPop();

	if (StackToolBox.Num() == 0)
	{
		SetUIMode(EUIMode::Free);
		return true;
	}
	
	SetToolBoxWidget(StackToolBox.Last().ToSharedRef());
	StackToolBox.Last()->OnPush();
	return true;
}


TSharedPtr<soda::SMessageBox> SSodaViewport::ShowMessageBox(soda::EMessageBoxType Type, const FString& Caption, const FString& Text)
{
	TSharedPtr<soda::SMessageBox> WindowWidget;
	SAssignNew(WindowWidget, soda::SMessageBox)
		.Caption(FText::FromString(Caption))
		.TextContent(FText::FromString(Text))
		.Type(Type);

	AddViewportWidgetContent(SNew(SWeakWidget).PossiblyNullContent(WindowWidget.ToSharedRef()), 14);
	OpenedWindows.Add(WindowWidget);
	return WindowWidget;
}

TSharedPtr<soda::SMenuWindow> SSodaViewport::OpenWindow(const FString& Caption, TSharedRef<soda::SMenuWindowContent> Content)
{
	TSharedPtr<soda::SMenuWindow> WindowWidget;
	SAssignNew(WindowWidget, soda::SMenuWindow)
		.Caption(FText::FromString(Caption))
		.Content(Content);
	AddViewportWidgetContent(SNew(SWeakWidget).PossiblyNullContent(WindowWidget.ToSharedRef()), 14);
	OpenedWindows.Add(WindowWidget);
	return WindowWidget;
}

bool SSodaViewport::CloseWindow(soda::SMenuWindow* Wnd)
{
	for (auto It = OpenedWindows.CreateIterator(); It; ++It)
	{
		if (It->Get() == Wnd)
		{
			It.RemoveCurrent();
			return true;
		}
	}
	return false;
}

TSharedPtr<soda::SWaitingPanel> SSodaViewport::ShowWaitingPanel(const FString& Caption, const FString& SubCaption)
{
	TSharedPtr<soda::SWaitingPanel> WaitingPanel;
	SAssignNew(WaitingPanel, soda::SWaitingPanel)
		.Caption(FText::FromString(Caption))
		.SubCaption(FText::FromString(SubCaption));
	AddViewportWidgetContent(SNew(SWeakWidget).PossiblyNullContent(WaitingPanel.ToSharedRef()), 15);
	return WaitingPanel;
}

bool SSodaViewport::CloseWaitingPanel(soda::SWaitingPanel* InWaitingPanel = nullptr)
{
	for (auto It = WaitingPanels.CreateIterator(); It; ++It)
	{
		if (It->Get() == InWaitingPanel)
		{
			It.RemoveCurrent();
			return true;
		}
	}
	return false;
}

bool SSodaViewport::CloseWaitingPanel(bool bCloseAll)
{
	bool Res = false;
	while (WaitingPanels.Num())
	{
		auto  Wnd = WaitingPanels.Pop();
		if (Wnd)
		{
			RemoveViewportWidgetContent(Wnd.ToSharedRef());
			Res = true;
			if (!bCloseAll) return true;
		}
	}
	return Res;
}

bool SSodaViewport::CloseWindow(bool bCloseAllWindows)
{
	bool Res = false;

	while (OpenedWindows.Num())
	{
		auto  Wnd = OpenedWindows.Pop();
		if (Wnd)
		{
			RemoveViewportWidgetContent(Wnd.ToSharedRef());
			Res = true;
			if (!bCloseAllWindows) return true;
		}
	}
	return Res;
}

} // namespace soda

#undef LOCTEXT_NAMESPACE
