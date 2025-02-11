// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "SSodaViewportToolBar.h"
#include "Framework/Commands/UIAction.h"
#include "Framework/MultiBox/MultiBoxDefs.h"
#include "Framework/MultiBox/MultiBoxExtender.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "HAL/PlatformFileManager.h"
#include "Modules/ModuleManager.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Input/SSpinBox.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "SodaStyleSet.h"
#include "Camera/CameraActor.h"
#include "GameFramework/WorldSettings.h"
#include "Soda/SodaSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "Soda/SodaStatics.h"
#include "Soda/SodaActorFactory.h"
#include "Soda/Vehicles/SodaVehicle.h"
#include "Soda/UI/SMessageBox.h"
#include "Soda/UI/SToolBox.h"
#include "Soda/UI/SodaViewportCommands.h"
#include "UI/Toolbar/STransformViewportToolbar.h"
#include "UI/Toolbar/SCameraViewportToolbar.h"
#include "UI/Toolbar/Common/SViewportToolBarMenu.h"
#include "UI/Toolbar/Common/SViewportToolBarButton.h"
#include "UI/Wnds/SChooseMapWindow.h"
#include "UI/Wnds/SLevelSaveLoadWindow.h"
#include "UI/Wnds/SSlotActorManagerWindow.h"
#include "UI/Wnds/SAboutWindow.h"
#include "UI/Wnds/PakWindow/SPakWindow.h"
#include "UI/Wnds/SQuickStartWindow.h"
#include "UI/Wnds/SSaveAllWindow.h"
#include "SPlacementModeTools.h"
#include "Soda/ISodaActor.h"
#include "RuntimeEditorModule.h"
#include "RuntimePropertyEditor/IDetailsView.h"
#include "Soda/UnrealSoda.h"
#include "Misc/Paths.h"
#include "Soda/SodaApp.h"
#include "Soda/SodaCommonSettings.h"
#include "JoystickGameSettings.h"
#include "SodaJoystick.h"
#include "RemoteControlSettings.h"
#include "GameFramework/GameUserSettings.h"
#include "RuntimeEditorUtils.h"
#include "Soda/FileDatabaseManager.h"
#include "Soda/SodaGameViewportClient.h"
#include "Soda/Editor/SodaSelection.h"
#include "GameFramework/PlayerController.h"

#define LOCTEXT_NAMESPACE "SodaViewportToolBar"


namespace soda
{

static void FillShowMenuStatic(FMenuBuilder& MenuBuilder, TArray< FSodalViewportCommands::FShowMenuCommand > MenuCommands, int32 EntryOffset)
{

	// Generate entries for the standard show flags
	// Assumption: the first 'n' entries types like 'Show All' and 'Hide All' buttons, so insert a separator after them
	for (int32 EntryIndex = 0; EntryIndex < MenuCommands.Num(); ++EntryIndex)
	{
		MenuBuilder.AddMenuEntry(
			MenuCommands[EntryIndex].ShowMenuItem,
			NAME_None,
			MenuCommands[EntryIndex].LabelOverride
		);

		if (EntryIndex == EntryOffset - 1)
		{
			MenuBuilder.AddSeparator(NAME_None);
		}
	}
}

static void FillShowStatsSubMenus(FMenuBuilder& MenuBuilder, TArray< FSodalViewportCommands::FShowMenuCommand > MenuCommands, TMap< FString, TArray< FSodalViewportCommands::FShowMenuCommand > > StatCatCommands)
{
	MenuBuilder.BeginSection(NAME_None);

	FillShowMenuStatic(MenuBuilder, MenuCommands, 1);

	// Separate out stats into two list, those with and without submenus
	TArray< FSodalViewportCommands::FShowMenuCommand > SingleStatCommands;
	TMap< FString, TArray< FSodalViewportCommands::FShowMenuCommand > > SubbedStatCommands;
	for (auto StatCatIt = StatCatCommands.CreateConstIterator(); StatCatIt; ++StatCatIt)
	{
		const TArray< FSodalViewportCommands::FShowMenuCommand >& ShowStatCommands = StatCatIt.Value();
		const FString& CategoryName = StatCatIt.Key();

		// If no category is specified, or there's only one category, don't use submenus
		FString NoCategory = "NoCategory";
		NoCategory.RemoveFromStart(TEXT("STATCAT_"));
		if (CategoryName == NoCategory || StatCatCommands.Num() == 1)
		{
			for (int32 StatIndex = 0; StatIndex < ShowStatCommands.Num(); ++StatIndex)
			{
				const FSodalViewportCommands::FShowMenuCommand& StatCommand = ShowStatCommands[StatIndex];
				SingleStatCommands.Add(StatCommand);
			}
		}
		else
		{
			SubbedStatCommands.Add(CategoryName, ShowStatCommands);
		}
	}

	// First add all the stats that don't have a sub menu
	for (auto StatCatIt = SingleStatCommands.CreateConstIterator(); StatCatIt; ++StatCatIt)
	{
		const FSodalViewportCommands::FShowMenuCommand& StatCommand = *StatCatIt;
		MenuBuilder.AddMenuEntry(
			StatCommand.ShowMenuItem,
			NAME_None,
			StatCommand.LabelOverride
		);
	}

	// Now add all the stats that have sub menus
	for (auto StatCatIt = SubbedStatCommands.CreateConstIterator(); StatCatIt; ++StatCatIt)
	{
		const TArray< FSodalViewportCommands::FShowMenuCommand >& StatCommands = StatCatIt.Value();
		const FText CategoryName = FText::FromString(StatCatIt.Key());

		FFormatNamedArguments Args;
		Args.Add(TEXT("StatCat"), CategoryName);
		const FText CategoryDescription = FText::Format(NSLOCTEXT("UICommands", "StatShowCatName", "Show {StatCat} stats"), Args);

		MenuBuilder.AddSubMenu(
			CategoryName, 
			CategoryDescription,
			FNewMenuDelegate::CreateStatic(&FillShowMenuStatic, StatCommands, 0));
	}

	MenuBuilder.EndSection();
}

void SSodaViewportToolBar::Construct( const FArguments& InArgs )
{
	static const FSlateIcon IconDB_Set = FSlateIcon(FSodaStyle::Get().GetStyleSetName(), "SodaIcons.DB.IdleBG", NAME_None, "SodaIcons.DB.Idle");
	static const FSlateIcon IconDB_Unset = FSlateIcon(FSodaStyle::Get().GetStyleSetName(), "SodaIcons.DB.DisabledBG", NAME_None, "SodaIcons.DB.Disabled");
	static const FSlateIcon IconDB_Recording = FSlateIcon(FSodaStyle::Get().GetStyleSetName(), "SodaIcons.DB.UploadBG", NAME_None, "SodaIcons.DB.Upload");
	static const FSlateIcon IconDB_Warning = FSlateIcon(FSodaStyle::Get().GetStyleSetName(), "SodaIcons.DB.WarningBG", NAME_None, "SodaIcons.DB.Warning");

	Viewport = InArgs._Viewport;
	TSharedRef<SSodaViewport> ViewportRef = Viewport.Pin().ToSharedRef();

	TSharedRef<FUICommandList> CommandList = Viewport.Pin()->GetCommandList().ToSharedRef();
	FSlimHorizontalToolBarBuilder ToolbarBuilder(CommandList, FMultiBoxCustomization::None);
	ToolbarBuilder.SetLabelVisibility(EVisibility::Collapsed);
	ToolbarBuilder.SetIsFocusable(false);
	ToolbarBuilder.SetStyle(&FSodaStyle::Get(), "EditorViewportToolBar");

	ToolbarBuilder.BeginSection(NAME_None);

	ToolbarBuilder.AddWidget
	(
		SNew(SViewportToolbarMenu)
		.ParentToolBar(SharedThis(this))
		.Image("EditorViewportToolBar.OptionsDropdown")
		.OnGetMenuContent(this, &SSodaViewportToolBar::GenerateMainMenu)
		.ToolTipText(FText::FromString(TEXT("Main menu")))
	);

	ToolbarBuilder.AddSeparator();
	ToolbarBuilder.AddWidget
	(
		SNew(SViewportToolbarMenu)
		.ParentToolBar(SharedThis(this))
		.Image("Icons.Toolbar.Settings")
		.OnGetMenuContent(this, &SSodaViewportToolBar::GenerateOptionsMenu)
		.ToolTipText(FText::FromString(TEXT("Settings Menu")))
	);

	ToolbarBuilder.AddSeparator();
	ToolbarBuilder.AddWidget
	(
		SNew(SViewportToolBarButton)
		.ToolTipText(FText::FromString(TEXT("Scenario Play/Stop")))
		//.Visibility(Viewport.Pin().Get(), &SSodaViewport::GetToolbarVisibility)
		.OnClicked(FOnClicked::CreateLambda([this]()
		{
			USodaSubsystem * SodaSubsystem = USodaSubsystem::GetChecked();
			if (SodaSubsystem->IsScenarioRunning())
			{
				const USodaCommonSettings* Settings = GetDefault<USodaCommonSettings>();
				SodaSubsystem->ScenarioStop(EScenarioStopReason::UserRequest, Settings->ScenarioStopMode);
			}
			else
			{
				SodaSubsystem->ScenarioPlay();
			}
			return FReply::Handled();
		}))
		[
			SNew(SImage)
			.Image_Lambda([]()
			{
				return FSodaStyle::GetBrush(USodaSubsystem::GetChecked()->IsScenarioRunning() ? "Icons.Toolbar.Stop" : "Icons.Toolbar.Play");
			})
		]
	);

	ToolbarBuilder.AddSeparator();
	ToolbarBuilder.AddWidget
	(
		SNew(SViewportToolBarButton)
		.IsEnabled_Lambda([]() {return !USodaSubsystem::GetChecked()->IsScenarioRunning(); })
		.Image("Icons.Save")
		.ToolTipText(FText::FromString(TEXT("Save/Load scenario")))
		.OnClicked(FOnClicked::CreateLambda([this]()
		{
			OnOpenSaveLoadWindow();
			return FReply::Handled();
		}))
	);

	ToolbarBuilder.AddSeparator();

	struct FDBStatusData
	{
		const FSlateIcon* Icon;
		FText ToolTip;

		void Update()
		{
			USodaSubsystem* Subsystem = SodaApp.GetSodaSubsystem();

			auto CommonStatus = EDatasetManagerStatus::Standby;
			for (auto& [Name, Manager] : SodaApp.GetDatasetManagers())
			{
				CommonStatus = FMath::Max(CommonStatus, Manager->GetStatus());
			}

			if (Subsystem->IsScenarioRunning())
			{
				switch (CommonStatus)
				{
				case EDatasetManagerStatus::Standby: 
					ToolTip = LOCTEXT("DSStatus_Playing_NotUsed", "Dataset isn't used");
					Icon = &IconDB_Unset;
					break;
				case EDatasetManagerStatus::Fluishing: 
					ToolTip = LOCTEXT("DSStatus_Playing_Flushing", "Dataset is flushing");
					Icon = &IconDB_Recording;
					break;
				case EDatasetManagerStatus::Recording: 
					Icon = &IconDB_Recording;
					ToolTip = LOCTEXT("DSStatus_Playing_Recording", "Dataset is recordering");
					break;
				case EDatasetManagerStatus::Faild:
				default: 
					Icon = &IconDB_Warning;
					ToolTip = LOCTEXT("DSStatus_Playing_Faild", "Dataset is fail");
					break;
				}
			}
			else
			{
				if (CommonStatus == EDatasetManagerStatus::Fluishing)
				{
					ToolTip = LOCTEXT("DSStatus_NoPlaying_Flushing", "Dataset is flushing");
					Icon = &IconDB_Recording;
				}
				else if (GetDefault<USodaCommonSettings>()->DatasetNamesToUse.IsEmpty())
				{
					ToolTip = LOCTEXT("DSStatus_NoPlaying_NotUsed", "Dataset isn't used");
					Icon = &IconDB_Unset;
				}
				else
				{
					ToolTip = LOCTEXT("DSStatus_NoPlaying_Set", "Set at least one dataset recorder");
					Icon = &IconDB_Set;
				}
			}
		}
	};

	TSharedPtr<FDBStatusData> DBStatusData = MakeShared<FDBStatusData>();
	DBStatusData->Update();

	ToolbarBuilder.AddWidget
	(
		SNew(SViewportToolbarMenu)
		.ParentToolBar(SharedThis(this))
		.LabelIcon_Lambda([DBStatusData]()
		{
			DBStatusData->Update();
			return DBStatusData->Icon->GetIcon();
		})
		.OverlayLabelIcon_Lambda([DBStatusData]()
		{
			return DBStatusData->Icon->GetOverlayIcon();
		})
		.ToolTipText_Lambda([DBStatusData]()
		{
			return DBStatusData->ToolTip;
		})
		.OnGetMenuContent(this, &SSodaViewportToolBar::GenerateDBMenu)
		.IsEnabled_Lambda([]() {return !USodaSubsystem::GetChecked()->IsScenarioRunning(); })
	);

	ToolbarBuilder.AddSeparator();
	ToolbarBuilder.AddWidget
	(
		SNew(SViewportToolbarMenu)
		.ParentToolBar(SharedThis(this))
		.Label(this, &SSodaViewportToolBar::GetModeMenuLabel)
		.LabelIcon(this, &SSodaViewportToolBar::GetModeMenuLabelIcon)
		//.IsEnabled_Lambda([]() {return !UUSodaSubsystem::GetChecked()->IsScenarioRunning(); })
		.OnGetMenuContent(this, &SSodaViewportToolBar::GenerateModeMenu)
		.ToolTipText(FText::FromString(TEXT("Change UI mode")))
	);

	ToolbarBuilder.AddSeparator();
	FUIAction AddAction;
	AddAction.IsActionVisibleDelegate.BindLambda([this]() {return Viewport.Pin()->GetUIMode() == soda::EUIMode::Editing; });
	ToolbarBuilder.AddComboButton
	(
		AddAction,
		FOnGetContent::CreateSP(this, &SSodaViewportToolBar::GenerateAddMenu),
		LOCTEXT("AddContent_Label", "Add"),
		LOCTEXT("AddContent_Tooltip", "Add a new actor to the scene"),
		FSlateIcon(FSodaStyle::Get().GetStyleSetName(), "LevelEditor.OpenAddContent.Background", NAME_None, "LevelEditor.OpenAddContent.Overlay")
	);

	/*
	ToolbarBuilder.AddSeparator();
	ToolbarBuilder.AddWidget
	(
		SNew(SViewportToolbarMenu)
		.ParentToolBar(SharedThis(this))
		.Image("Icons.Toolbar.Settings")
	);
	*/

	ToolbarBuilder.EndSection();

	const FMargin ToolbarSlotPadding(4.0f, 1.0f);

	ChildSlot
	[
		SNew(SBorder)
		.BorderImage(FSodaStyle::Get().GetBrush("EditorViewportToolBar.Background"))
		.Cursor(EMouseCursor::Default)
		[
			SNew( SHorizontalBox )
			+ SHorizontalBox::Slot()
			.FillWidth(1.0)
			.Padding(ToolbarSlotPadding)
			[
				FRuntimeEditorUtils::MakeWidget_HackTooltip(ToolbarBuilder)
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(ToolbarSlotPadding)
			.HAlign(HAlign_Right)
			[
				SNew(STransformViewportToolBar)
				.Viewport(ViewportRef)
				.CommandList(ViewportRef->GetCommandList())
				//.Extenders(LevelEditorModule.GetToolBarExtensibilityManager()->GetAllExtenders())
				.Visibility(this, &SSodaViewportToolBar::IsEditorMode)
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(ToolbarSlotPadding)
			.HAlign(HAlign_Right)
			[
				SNew(SCameraViewportToolbar)
				.Viewport(ViewportRef)
				.CommandList(ViewportRef->GetCommandList())
				//.Extenders(LevelEditorModule.GetToolBarExtensibilityManager()->GetAllExtenders())
				.Visibility(this, &SSodaViewportToolBar::IsSpectatorMode)
			]
		]
	];

	SViewportToolBar::Construct(SViewportToolBar::FArguments());
}

/*
FLevelEditorViewportClient* SSodaViewportToolBar::GetLevelViewportClient()
{
	if (Viewport.IsValid())
	{
		return &Viewport.Pin()->GetLevelViewportClient();
	}

	return nullptr;
}

*/

FText SSodaViewportToolBar::GetModeMenuLabel() const
{
	switch (Viewport.Pin()->GetUIMode())
	{
	case soda::EUIMode::Free: return FText::FromString("Free");
	case soda::EUIMode::Editing: return FText::FromString("Editor");
	}
	return FText::FromString("None");
}

const FSlateBrush* SSodaViewportToolBar::GetModeMenuLabelIcon() const
{	
	switch (Viewport.Pin()->GetUIMode())
	{
	case soda::EUIMode::Free: return FSodaStyle::GetOptionalBrush("Icons.Toolbar.FreeMode");
	case soda::EUIMode::Editing:   return FSodaStyle::GetOptionalBrush("Icons.Toolbar.EditorMode");
	}
	return FStyleDefaults::GetNoBrush();
}

TSharedRef<SWidget> SSodaViewportToolBar::GenerateMainMenu()
{
	FMenuBuilder MenuBuilder(true, Viewport.Pin()->GetCommandList(), TSharedPtr<FExtender>(), false, &FSodaStyle::Get());

	MenuBuilder.BeginSection(NAME_None);

	{
		FUIAction Action;
		Action.ExecuteAction.BindSP(this, &SSodaViewportToolBar::OnOpenPakWindow);
		MenuBuilder.AddMenuEntry(
			FText::FromString(TEXT("Paks")),
			FText::FromString(TEXT("Paks")),
			FSlateIcon(FSodaStyle::Get().GetStyleSetName(), "Icons.Import"),
			Action);
	}

	{
		FUIAction Action;
		Action.ExecuteAction.BindSP(this, &SSodaViewportToolBar::OnOpenLevelWindow);
		MenuBuilder.AddMenuEntry(
			FText::FromString(TEXT("Open Level")),
			FText::FromString(TEXT("Open Level")),
			FSlateIcon(FSodaStyle::Get().GetStyleSetName(), "Icons.Level"),
			Action);
	}

	{
		FUIAction Action;
		Action.ExecuteAction.BindSP(this, &SSodaViewportToolBar::OnOpenScenariosManagerWindow);
		MenuBuilder.AddMenuEntry(
			FText::FromString(TEXT("Scenarios Manager")),
			FText::FromString(TEXT("Scenarios Manager")),
			FSlateIcon(FSodaStyle::Get().GetStyleSetName(), "Icons.Level"),
			Action);
	}

	{
		FUIAction Action;
		Action.ExecuteAction.BindSP(this, &SSodaViewportToolBar::OnOpenVehicleManagerWindow);
		MenuBuilder.AddMenuEntry(
			FText::FromString(TEXT("Vehicles Manager")),
			FText::FromString(TEXT("Vehicles Manager")),
			FSlateIcon(FSodaStyle::Get().GetStyleSetName(), "SodaIcons.Car"),
			Action);
	}


	MenuBuilder.AddMenuSeparator();
	
	MenuBuilder.AddMenuEntry(FSodalViewportCommands::Get().ToggleSpectatorMode);
	MenuBuilder.AddMenuEntry(FSodalViewportCommands::Get().PossesNextVehicle);
	MenuBuilder.AddMenuEntry(FSodalViewportCommands::Get().ClearLevel);
	MenuBuilder.AddMenuEntry(FSodalViewportCommands::Get().RestartLevel);

	MenuBuilder.AddMenuSeparator();

	{
		FUIAction Action;
		Action.ExecuteAction.BindSP(this, &SSodaViewportToolBar::OnOpenQuickStartWindow);
		MenuBuilder.AddMenuEntry(
			FText::FromString(TEXT("Quick Start")),
			FText::FromString(TEXT("Quick Start")),
			FSlateIcon(FSodaStyle::Get().GetStyleSetName(), "Icons.Advanced"),
			Action);
	}

	{
		FUIAction Action;
		Action.ExecuteAction.BindSP(this, &SSodaViewportToolBar::OnOpenAboutWindow);
		MenuBuilder.AddMenuEntry(
			FText::FromString(TEXT("About")),
			FText::FromString(TEXT("About")),
			FSlateIcon(FSodaStyle::Get().GetStyleSetName(), "SodaIcons.Soda"),
			Action);
	}

	MenuBuilder.AddMenuEntry(FSodalViewportCommands::Get().Exit);

	MenuBuilder.EndSection();

	return FRuntimeEditorUtils::MakeWidget_HackTooltip(MenuBuilder, nullptr, 1000);
}

TSharedRef<SWidget> SSodaViewportToolBar::GenerateOptionsMenu() 
{
	FMenuBuilder MenuBuilder(true, Viewport.Pin()->GetCommandList(), TSharedPtr<FExtender>(), false, &FSodaStyle::Get());
	MenuBuilder.BeginSection(NAME_None);

	for (TObjectIterator<USodaUserSettings> SettingsIt(RF_NoFlags); SettingsIt; ++SettingsIt)
	{
		if (USodaUserSettings* Settings = *SettingsIt)
		{
			// Only Add the CDO of any UDeveloperSettings objects.
			if (Settings->HasAnyFlags(RF_ClassDefaultObject) && !Settings->GetClass()->HasAnyClassFlags(CLASS_Deprecated | CLASS_Abstract))
			{
				//RegisterDeveloperSettings(SettingsModule, Settings);

				FUIAction Action;
				Action.ExecuteAction.BindLambda([Settings]()
				{
					Settings->OnPreOpen();
					USodaSubsystem* SodaSubsystem = USodaSubsystem::GetChecked();
					FRuntimeEditorModule& RuntimeEditorModule = FModuleManager::LoadModuleChecked<FRuntimeEditorModule>("RuntimeEditor");
					soda::FDetailsViewArgs Args;
					Args.bHideSelectionTip = true;
					Args.bLockable = false;
					//Args.bAllowMultipleTopLevelObjects = true;


					TSharedPtr<soda::IDetailsView> DetailView = RuntimeEditorModule.CreateDetailView(Args);
					DetailView->SetObject(Settings);

					SodaSubsystem->PushToolBox(
						SNew(SToolBox)
						.Caption(Settings->GetClass()->GetDisplayNameText())
						[
							SNew(SVerticalBox)
							+ SVerticalBox::Slot()
							.FillHeight(1)
							[
								DetailView.ToSharedRef()
							]
							+ SVerticalBox::Slot()
							.AutoHeight()
							.VAlign(VAlign_Top)
							.HAlign(HAlign_Center)
							[
								SNew(SHorizontalBox)
								+ SHorizontalBox::Slot()
								.Padding(5)
								[
									SNew(SButton)
									.Text(FText::FromString(TEXT("Save")))
									.OnClicked(FOnClicked::CreateLambda([Settings]()
									{
					
										Settings->SaveConfig();
										return FReply::Handled();
									}))
								]
								+ SHorizontalBox::Slot()
								.Padding(5)
								[
									SNew(SButton)
									.Text(FText::FromString(TEXT("Reset")))
									.OnClicked(FOnClicked::CreateLambda([Settings]()
									{
										Settings->ResetToDefault();
										return FReply::Handled();
									}))
								]
							]
						]);
				});
				MenuBuilder.AddMenuEntry(
					Settings->GetMenuItemText(),
					Settings->GetMenuItemDescription(),
					FSlateIcon(FSodaStyle::Get().GetStyleSetName(), Settings->GetMenuItemIconName()),
					Action);
			
			}
		}
	}

	{
		FUIAction Action;
		Action.ExecuteAction.BindLambda([]() 
		{
			USodaSubsystem* SodaSubsystem = USodaSubsystem::GetChecked();
			FRuntimeEditorModule& RuntimeEditorModule = FModuleManager::LoadModuleChecked<FRuntimeEditorModule>("RuntimeEditor");
			soda::FDetailsViewArgs Args;
			Args.bHideSelectionTip = true;
			Args.bLockable = false;
			TSharedPtr<soda::IDetailsView> DetailView = RuntimeEditorModule.CreateDetailView(Args);
			DetailView->SetObject(GetMutableDefault<UJoystickGameSettings>());
			SodaSubsystem->PushToolBox(
				SNew(SToolBox)
				.Caption(FText::FromString("Joystic Settings"))
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					.FillHeight(1)
					
					[
						DetailView.ToSharedRef()
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					.VAlign(VAlign_Top)
					.HAlign(HAlign_Center)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot()
						.Padding(5)
						[
							SNew(SButton)
							.Text(FText::FromString(TEXT("Save")))
							.OnClicked(FOnClicked::CreateLambda([]()
							{
								GetMutableDefault<UJoystickGameSettings>()->SaveConfig();
								ISodaJoystickPlugin::Get().UpdateSettings();
								return FReply::Handled();
							}))
						]
						/*
						+ SHorizontalBox::Slot()
						.Padding(5)
						[
							SNew(SButton)
							.Text(FText::FromString(TEXT("Reset")))
							.OnClicked(FOnClicked::CreateLambda([]()
							{
								GetMutableDefault<UJoystickGameSettings>()->LoadConfig();
								ISodaJoystickPlugin::Get().UpdateSettings();
								return FReply::Handled();
							}))
						]
						*/
					]
				]);
		});
		MenuBuilder.AddMenuEntry(
			FText::FromString("Joystic Settings"),
			FText::FromString("Joystic Settings"),
			FSlateIcon(FSodaStyle::Get().GetStyleSetName(), "SodaIcons.Joystick"),
			Action);
	}

	{
		FUIAction Action;
		Action.ExecuteAction.BindLambda([]() 
		{
			USodaSubsystem* SodaSubsystem = USodaSubsystem::GetChecked();
			FRuntimeEditorModule& RuntimeEditorModule = FModuleManager::LoadModuleChecked<FRuntimeEditorModule>("RuntimeEditor");
			soda::FDetailsViewArgs Args;
			Args.bHideSelectionTip = true;
			Args.bLockable = false;
			TSharedPtr<soda::IDetailsView> DetailView = RuntimeEditorModule.CreateDetailView(Args);
			DetailView->SetObject(GetMutableDefault<URemoteControlSettings>());
			SodaSubsystem->PushToolBox(
				SNew(SToolBox)
				.Caption(FText::FromString("RC Settings"))
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					.FillHeight(1)
					
					[
						DetailView.ToSharedRef()
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					.VAlign(VAlign_Top)
					.HAlign(HAlign_Center)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot()
						.Padding(5)
						[
							SNew(SButton)
							.Text(FText::FromString(TEXT("Save")))
							.OnClicked(FOnClicked::CreateLambda([]()
							{
								GetMutableDefault<URemoteControlSettings>()->SaveConfig();
								return FReply::Handled();
							}))
						]
						/*
						+ SHorizontalBox::Slot()
						.Padding(5)
						[
							SNew(SButton)
							.Text(FText::FromString(TEXT("Reset")))
							.OnClicked(FOnClicked::CreateLambda([]()
							{
								GetMutableDefault<URemoteControlSettings>()->LoadConfig();
								return FReply::Handled();
							}))
						]
						*/
					]
				]);
		});
		MenuBuilder.AddMenuEntry(
			FText::FromString("RC Settings"),
			FText::FromString("Remote Control Settings"),
			FSlateIcon(FSodaStyle::Get().GetStyleSetName(), "Icons.World"),
			Action);
	}

	{
		FUIAction Action;
		Action.ExecuteAction.BindLambda([this]() 
		{
			Viewport.Pin()->SetLogVisible(!Viewport.Pin()->IsLogVisible());
		});
		Action.GetActionCheckState.BindLambda([this]() 
		{
			return Viewport.Pin()->IsLogVisible() ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
		});
		MenuBuilder.AddMenuEntry(
			FText::FromString("Show Log"),
			FText::FromString("Show Log"),
			FSlateIcon(FSodaStyle::Get().GetStyleSetName(), "Icons.Log"),
			Action,
			NAME_None,
			EUserInterfaceActionType::ToggleButton);
	}

	MenuBuilder.AddMenuEntry(FSodalViewportCommands::Get().ToggleVehiclePanel);
	MenuBuilder.AddMenuEntry(FSodalViewportCommands::Get().ToggleFPS); 
	MenuBuilder.AddMenuEntry(FSodalViewportCommands::Get().ToggleFullScreen);
	MenuBuilder.AddMenuEntry(FSodalViewportCommands::Get().TagActors);
	MenuBuilder.AddMenuEntry(FSodalViewportCommands::Get().ToggleStats);
	
	{
		const FSodalViewportCommands& SodaViewportCommands = FSodalViewportCommands::Get();
		TArray< FSodalViewportCommands::FShowMenuCommand > HideStatsMenu;
		HideStatsMenu.Add(FSodalViewportCommands::FShowMenuCommand(SodaViewportCommands.HideAllStats, LOCTEXT("HideAllLabel", "Hide All"))); // 'Hide All' button

		MenuBuilder.AddSubMenu(
			LOCTEXT("ShowStatsMenu", "Stat"),
			LOCTEXT("ShowStatsMenu_ToolTip", "Show Stat commands"),
			FNewMenuDelegate::CreateStatic(&FillShowStatsSubMenus, HideStatsMenu, SodaViewportCommands.ShowStatCatCommands),
			false,
			FSlateIcon(FSodaStyle::Get().GetStyleSetName(), "SodaViewport.SubMenu.Stats"));
	}

	MenuBuilder.EndSection();
	return FRuntimeEditorUtils::MakeWidget_HackTooltip(MenuBuilder, nullptr, 1000);
}

TSharedRef<SWidget> SSodaViewportToolBar::GenerateDBMenu()
{
	static const FSlateIcon IconDB_Unset = FSlateIcon(FSodaStyle::Get().GetStyleSetName(), "SodaIcons.DB.DisabledBG", NAME_None, "SodaIcons.DB.Disabled");

	FMenuBuilder MenuBuilder(true, Viewport.Pin()->GetCommandList(), TSharedPtr<FExtender>(), false, &FSodaStyle::Get());
	MenuBuilder.BeginSection(NAME_None);

	auto & DatasteManagers = SodaApp.GetDatasetManagers();
	//auto ActiveDatasteManager = SodaApp.GetActiveDatasetManager();

	{
		FUIAction Action;
		Action.CanExecuteAction.BindLambda([]() 
		{
			return !GetMutableDefault<USodaCommonSettings>()->DatasetNamesToUse.IsEmpty();
		});
		Action.ExecuteAction.BindLambda([]()
		{
			GetMutableDefault<USodaCommonSettings>()->DatasetNamesToUse.Empty();
		});
		MenuBuilder.AddMenuEntry(
			FText::FromString(TEXT("Don't Record")),
			FText::FromString(TEXT("Do not record the dataset when stratifying a scenario")),
			FSlateIcon(FSodaStyle::Get().GetStyleSetName(), "SodaIcons.DB.DisabledFull"),
			Action);
	}

	for (auto & [Name, DatasteManage] : DatasteManagers)
	{
		FUIAction Action;
		Action.GetActionCheckState.BindLambda([DatasteManage, Name]()
		{
			return GetDefault<USodaCommonSettings>()->DatasetNamesToUse.Contains(Name) ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
		});
		Action.ExecuteAction.BindLambda([DatasteManage, Name]()
		{
			auto* Settings = GetMutableDefault<USodaCommonSettings>();
			if(Settings->DatasetNamesToUse.Contains(Name))
			{
				Settings->DatasetNamesToUse.Remove(Name);
			}
			else
			{
				Settings->DatasetNamesToUse.Add(Name);
			}
			Settings->SaveConfig();
		});
		MenuBuilder.AddMenuEntry(
			DatasteManage->GetDisplayName(),
			DatasteManage->GetToolTip(),
			FSlateIcon(FSodaStyle::Get().GetStyleSetName(), DatasteManage->GetIconName()),
			Action, 
			NAME_None, 
			EUserInterfaceActionType::Check);
	}

	MenuBuilder.EndSection();
	return FRuntimeEditorUtils::MakeWidget_HackTooltip(MenuBuilder, nullptr, 1000);
}

TSharedRef<SWidget> SSodaViewportToolBar::GenerateAddMenu()
{
	FMenuBuilder MenuBuilder(true, Viewport.Pin()->GetCommandList(), TSharedPtr<FExtender>(), false, &FSodaStyle::Get());

	// Build menu by categiries
	MenuBuilder.BeginSection(NAME_None);
	TMap<FString, TMap<FString, TArray<TSoftClassPtr<AActor>>>> ActorsMap;
	for (const auto & It : USodaSubsystem::GetChecked()->GetSodaActorDescriptors())
	{
		if (It.Value.bAllowSpawn)
		{
			auto & Categoty = ActorsMap.FindOrAdd(It.Value.Category);
			FString SubCategory = It.Value.SubCategory;
			if (It.Value.SubCategory == "" || It.Value.SubCategory.Compare("default", ESearchCase::IgnoreCase) == 0)
			{
				SubCategory = "";
			}
			auto & SubCategoty = Categoty.FindOrAdd(SubCategory);
			SubCategoty.AddUnique(It.Key);
		}
	}
	TArray<FString> CategoryKeys;
	ActorsMap.GetKeys(CategoryKeys);
	CategoryKeys.Sort();
	for (const FString& Category : CategoryKeys)
	{
		MenuBuilder.AddSubMenu(
			FText::FromString(Category),
			FText::GetEmpty(),
			FNewMenuDelegate::CreateLambda([CategoryMap=ActorsMap[Category]](FMenuBuilder& MenuBuilder)
			{
				TArray<FString> SubCategoryKeys;
				CategoryMap.GetKeys(SubCategoryKeys);
				SubCategoryKeys.Sort();
				//SubCategoryKeys.FindByPredicate([](const FString& Str) { return Str.Compare("default", ESearchCase::IgnoreCase) == 0; });
				for (auto& SubCategory : SubCategoryKeys)
				{
					MenuBuilder.BeginSection(NAME_None, FText::FromString(SubCategory == "" ? TEXT("Default") : *SubCategory));
					for (auto & Actor : CategoryMap[SubCategory])
					{
						const FSodaActorDescriptor & Desc = USodaSubsystem::GetChecked()->GetSodaActorDescriptor(Actor);
						TSharedPtr<FPlaceableItem> Item = MakeShared<FPlaceableItem>(
							Actor,
							Desc.Icon, 
							Desc.Icon, 
							TOptional<FLinearColor>(), 
							FName(Desc.DisplayName.Len() ? *Desc.DisplayName : *FName::NameToDisplayString(Actor.GetAssetName(), false))
						);
						MenuBuilder.AddWidget(
							SNew(SPlacementAssetMenuEntry, Item), FText()
						);
					}
					MenuBuilder.EndSection();
				}
			})
		);
	}
	MenuBuilder.EndSection();

	MenuBuilder.BeginSection(NAME_None);
	MenuBuilder.AddSeparator();
	MenuBuilder.AddSubMenu(
		FText::FromString("SODA Vehicles"),
		FText::GetEmpty(),
		FNewMenuDelegate::CreateLambda([World=GetWorld()](FMenuBuilder& MenuBuilder)
		{
			MenuBuilder.BeginSection(NAME_None);
			auto Slots = SodaApp.GetFileDatabaseManager().GetSlots(EFileSlotType::Vehicle);
			
			for (auto& [Key, Info] : Slots)
			{
				TSharedPtr<FPlaceableItem> Item = MakeShared<FPlaceableItem>(
					FSpawnCastomActorDelegate::CreateLambda([World, Info](const FTransform& Transform)
					{
						ASodaVehicle * Vehicle = ASodaVehicle::SpawnVehicleFormSlot(World.Get(), Info->GUID, Transform.GetTranslation(), Transform.GetRotation().Rotator(), false, NAME_None, true);
						return Vehicle;
					}),
					TEXT("Icons.Save"),
					TEXT("Icons.Save"),
					TOptional<FLinearColor>(),
					FName(*Info->Lable)
				);
				MenuBuilder.AddWidget(
					SNew(SPlacementAssetMenuEntry, Item), FText()
				);
			}
			MenuBuilder.EndSection();
		}),
		false,
		FSlateIcon(FSodaStyle::Get().GetStyleSetName(), "Icons.Save")
	);
	MenuBuilder.EndSection();
	
	return FRuntimeEditorUtils::MakeWidget_HackTooltip(MenuBuilder, nullptr, 1000);
}

TSharedRef<SWidget> SSodaViewportToolBar::GenerateModeMenu()
{
	FMenuBuilder MenuBuilder(true, Viewport.Pin()->GetCommandList(), TSharedPtr<FExtender>(), false, &FSodaStyle::Get());
	MenuBuilder.BeginSection(NAME_None);

	{
		FUIAction Action(
			FExecuteAction::CreateSP(this, &SSodaViewportToolBar::ChangeMode, EUIMode::Free),
			FCanExecuteAction(),
			FIsActionChecked::CreateSP(this, &SSodaViewportToolBar::ModeIs, EUIMode::Free)
		);
		MenuBuilder.AddMenuEntry(
			FText::FromString("Free Mode"),
			FText::GetEmpty(),
			FSlateIcon(FSodaStyle::Get().GetStyleSetName(), "Icons.Toolbar.FreeMode"),
			Action,
			NAME_None,
			EUserInterfaceActionType::RadioButton);
	}

	{
		FUIAction Action(
			FExecuteAction::CreateSP(this, &SSodaViewportToolBar::ChangeMode, EUIMode::Editing),
			FCanExecuteAction(),
			FIsActionChecked::CreateSP(this, &SSodaViewportToolBar::ModeIs, EUIMode::Editing)
		);
		MenuBuilder.AddMenuEntry(
			FText::FromString("Editor Mode"),
			FText::GetEmpty(),
			FSlateIcon(FSodaStyle::Get().GetStyleSetName(), "Icons.Toolbar.EditorMode"),
			Action,
			NAME_None,
			EUserInterfaceActionType::RadioButton);
	}


	MenuBuilder.EndSection();
	return FRuntimeEditorUtils::MakeWidget_HackTooltip(MenuBuilder, nullptr, 1000);
}


void SSodaViewportToolBar::ChangeMode(soda::EUIMode InMode) const 
{ 
	Viewport.Pin()->SetUIMode(InMode);
}

bool SSodaViewportToolBar::ModeIs(soda::EUIMode InMode) const 
{ 
	return InMode == Viewport.Pin()->GetUIMode();
}

TWeakObjectPtr<UWorld> SSodaViewportToolBar::GetWorld() const
{
	if (Viewport.IsValid())
	{
		return Viewport.Pin()->GetWorld();
	}
	return nullptr;
}

/*
TSharedPtr<FExtender> SSodaViewportToolBar::GetViewMenuExtender()
{
	FLevelEditorModule& LevelEditorModule = FModuleManager::GetModuleChecked<FLevelEditorModule>(TEXT("LevelEditor"));
	return LevelEditorModule.GetMenuExtensibilityManager()->GetAllExtenders();
}
*/

/*
FLevelEditorViewportClient* ULevelViewportToolBarContext::GetLevelViewportClient()
{
	if (LevelViewportToolBarWidget.IsValid())
	{
		return LevelViewportToolBarWidget.Pin()->GetLevelViewportClient();
	}

	return nullptr;
}
*/

void SSodaViewportToolBar::OnOpenLevelWindow()
{
	USodaSubsystem::GetChecked()->OpenWindow("Choose Map", SNew(SChooseMapWindow));
}

void SSodaViewportToolBar::OnOpenPakWindow()
{
	USodaSubsystem::GetChecked()->OpenWindow("Paks", SNew(SPakWindow));
}

void SSodaViewportToolBar::OnOpenAboutWindow()
{
	USodaSubsystem::GetChecked()->OpenWindow("About SODA.Sim", SNew(SAboutWindow));
}

void SSodaViewportToolBar::OnOpenQuickStartWindow()
{
	USodaSubsystem::GetChecked()->OpenWindow("Quick Start", SNew(SQuickStartWindow));
}

void SSodaViewportToolBar::OnOpenVehicleManagerWindow()
{
	USodaSubsystem* SodaSubsystem = USodaSubsystem::GetChecked();
	USodaGameViewportClient* GameViewportClient = Cast<USodaGameViewportClient>(GetWorld()->GetGameViewport());
	ASodaVehicle* SelectedVehicle = nullptr;
	
	if (IsValid(GameViewportClient->Selection))
	{
		SelectedVehicle = Cast<ASodaVehicle>(GameViewportClient->Selection->GetSelectedActor());
	}

	if (!SelectedVehicle)
	{
		if (APlayerController* PlayerController = GetWorld()->GetFirstPlayerController())
		{
			SelectedVehicle = PlayerController->GetPawn<ASodaVehicle>();
		}
	}

	SodaSubsystem->OpenWindow("Vehicles Manager", SNew(SSlotActorManagerWindow, soda::EFileSlotType::Vehicle, SelectedVehicle));
}

void SSodaViewportToolBar::OnOpenScenariosManagerWindow()
{
	USodaSubsystem::GetChecked()->OpenWindow("Scenarios Manager", SNew(SLevelSaveLoadWindow));
}

void SSodaViewportToolBar::OnOpenSaveLoadWindow()
{
	USodaSubsystem::GetChecked()->OpenWindow("Save All", SNew(SSaveAllWindow, ESaveAllWindowMode::Normal));
}

EVisibility SSodaViewportToolBar::IsEditorMode() const
{
	return Viewport.Pin()->GetUIMode() == EUIMode::Editing ?
		EVisibility::Visible :
		EVisibility::Collapsed;
}

EVisibility SSodaViewportToolBar::IsSpectatorMode() const
{
	USodaSubsystem* SodaSubsystem = USodaSubsystem::GetChecked();
	if (SodaSubsystem && SodaSubsystem->SpectatorActor)
	{
		return EVisibility::Visible;
	}
	else
	{
		return EVisibility::Collapsed;
	}
}

} // namespace soda

#undef LOCTEXT_NAMESPACE
