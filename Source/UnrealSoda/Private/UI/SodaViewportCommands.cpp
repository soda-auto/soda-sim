// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/UI/SodaViewportCommands.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Components/PrimitiveComponent.h"
#include "AssetRegistry/AssetData.h"
#include "Modules/ModuleManager.h"
#include "Materials/MaterialInterface.h"
#include "MaterialShared.h"
#include "Engine/Texture2D.h"
#include "Stats/StatsData.h"
#include "Engine/Engine.h"

#define LOCTEXT_NAMESPACE "SodaViewportCommands"

namespace soda
{

FSodalViewportCommands::FOnNewStatCommandAdded FSodalViewportCommands::NewStatCommandDelegate;

FSodalViewportCommands::FSodalViewportCommands()
	: TCommands<FSodalViewportCommands>
	(
		TEXT("SodaViewport"),
		NSLOCTEXT("Contexts", "SodaViewportCommands", "Soda Viewport Commands"),
		NAME_None,
		FSodaStyle::GetStyleSetName() // Icon Style Set
	)
{
}

FSodalViewportCommands::~FSodalViewportCommands()
{
	UEngine::NewStatDelegate.RemoveAll(this);
	FStatGroupGameThreadNotifier::Get().NewStatGroupDelegate.Unbind();
}

void FSodalViewportCommands::RegisterCommands()
{
	UI_COMMAND( Perspective, "Perspective", "Switches the viewport to perspective view", EUserInterfaceActionType::RadioButton, FInputChord( EModifierKey::Alt, EKeys::G ) );
	UI_COMMAND( OrthographicFree, "OrthographicFree", "Switches the viewport to orthographic free view", EUserInterfaceActionType::RadioButton, FInputChord(EModifierKey::Alt, EKeys::O));
	UI_COMMAND( Front, "Front", "Switches the viewport to front view", EUserInterfaceActionType::RadioButton, FInputChord( EModifierKey::Alt, EKeys::H ) );
	UI_COMMAND( Back, "Back", "Switches the viewport to back view", EUserInterfaceActionType::RadioButton, FInputChord( EModifierKey::Alt | EModifierKey::Shift, EKeys::H ) );
	UI_COMMAND( Top, "Top", "Switches the viewport to top view", EUserInterfaceActionType::RadioButton, FInputChord( EModifierKey::Alt, EKeys::J ) );
	UI_COMMAND( Bottom, "Bottom", "Switches the viewport to bottom view", EUserInterfaceActionType::RadioButton, FInputChord( EModifierKey::Alt | EModifierKey::Shift, EKeys::J ) );
	UI_COMMAND( Left, "Left", "Switches the viewport to left view", EUserInterfaceActionType::RadioButton, FInputChord( EModifierKey::Alt, EKeys::K ) );
	UI_COMMAND( Right, "Right", "Switches the viewport to right view", EUserInterfaceActionType::RadioButton, FInputChord( EModifierKey::Alt | EModifierKey::Shift, EKeys::K ) );
	UI_COMMAND( Next, "Next", "Rotate through each view options", EUserInterfaceActionType::RadioButton, FInputChord( EModifierKey::Control | EModifierKey::Shift, EKeys::SpaceBar ) );

	UI_COMMAND( ToggleStats, "Show Stats", "Toggles the ability to show stats in this viewport (enables realtime)", EUserInterfaceActionType::ToggleButton, FInputChord( EModifierKey::Shift, EKeys::L ) );
	UI_COMMAND( ToggleFPS, "Show FPS", "Toggles showing frames per second in this viewport (enables realtime)", EUserInterfaceActionType::ToggleButton, FInputChord( EModifierKey::Control|EModifierKey::Shift, EKeys::H ) );
	UI_COMMAND( ToggleVehiclePanel, "Show Vehicle Panel", "Toggles the debug vehicle panel", EUserInterfaceActionType::ToggleButton, FInputChord(EModifierKey::Control | EModifierKey::Shift, EKeys::V));
	UI_COMMAND( ToggleFullScreen, "Full Screen", "Toggles full screen", EUserInterfaceActionType::ToggleButton, FInputChord());
	
	UI_COMMAND( SelectMode, "Select Mode", "Select objects", EUserInterfaceActionType::ToggleButton, FInputChord());
	UI_COMMAND( TranslateMode, "Translate Mode", "Select and translate objects", EUserInterfaceActionType::ToggleButton, FInputChord() );
	UI_COMMAND( RotateMode, "Rotate Mode", "Select and rotate objects", EUserInterfaceActionType::ToggleButton, FInputChord() );
	UI_COMMAND( ScaleMode, "Scale Mode", "Select and scale objects", EUserInterfaceActionType::ToggleButton, FInputChord() );

	UI_COMMAND( RelativeCoordinateSystem_World, "World-relative Transform", "Move and rotate objects relative to the cardinal world axes", EUserInterfaceActionType::RadioButton, FInputChord() );
	UI_COMMAND( RelativeCoordinateSystem_Local, "Local-relative Transform", "Move and rotate objects relative to the object's local axes", EUserInterfaceActionType::RadioButton, FInputChord() );

#if PLATFORM_MAC
	UI_COMMAND( CycleTransformGizmoCoordSystem, "Cycle Transform Coordinate System", "Cycles the transform gizmo coordinate systems between world and local (object) space", EUserInterfaceActionType::Button, FInputChord(EModifierKey::Command, EKeys::Tilde));
#else
	UI_COMMAND( CycleTransformGizmoCoordSystem, "Cycle Transform Coordinate System", "Cycles the transform gizmo coordinate systems between world and local (object) space", EUserInterfaceActionType::Button, FInputChord(EModifierKey::Control, EKeys::Tilde));
#endif
	UI_COMMAND( CycleTransformGizmos, "Cycle Between Translate, Rotate, and Scale", "Cycles the transform gizmos between translate, rotate, and scale", EUserInterfaceActionType::Button, FInputChord(EKeys::SpaceBar) );
	
	
	UI_COMMAND(HideAllStats, "Hide All Stats", "Hides all Stats", EUserInterfaceActionType::Button, FInputChord());

	UI_COMMAND(RestartLevel, "Restart Level", "Restart current level", EUserInterfaceActionType::Button, FInputChord(EModifierKey::Shift, EKeys::F));
	UI_COMMAND(ClearLevel, "Clear Level", "Reload level without saved data", EUserInterfaceActionType::Button, FInputChord());

	//UI_COMMAND(RecordDataset, "Record Dataset", "Record dataset when a scenario is running", EUserInterfaceActionType::ToggleButton, FInputChord());
	//UI_COMMAND(AutoConnectDB, "Auto Connect", "Automatically connect to the database on application startup", EUserInterfaceActionType::ToggleButton, FInputChord());

	UI_COMMAND(ToggleSpectatorMode, "Toggle Spectator Mode", "Enbale/disable the spectator mode", EUserInterfaceActionType::Button, FInputChord(EKeys::Enter));
	UI_COMMAND(PossesNextVehicle, "Posses Next Vehicle", "Posses next Soda Vehicle", EUserInterfaceActionType::Button, FInputChord(EKeys::X));
	UI_COMMAND(BackMenu, "Back Menu", "Back menu", EUserInterfaceActionType::Button, FInputChord(EKeys::Escape));
	UI_COMMAND(BackMenuEditor, "Back Menu", "Back menu", EUserInterfaceActionType::Button, FInputChord(EKeys::Tab));
	UI_COMMAND(TagActors, "Tag Actors", "Tag all actors for semantic segmentation and add V2X components for all SodaVehicles", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(Exit, "Exit", "Exit application", EUserInterfaceActionType::Button, FInputChord(EModifierKey::Shift, EKeys::Q));
	

	// Bind a listener here for any additional stat commands that get registered later.
	UEngine::NewStatDelegate.AddRaw(this, &FSodalViewportCommands::HandleNewStat);
	FStatGroupGameThreadNotifier::Get().NewStatGroupDelegate.BindRaw(this, &FSodalViewportCommands::HandleNewStatGroup);
}

void FSodalViewportCommands::HandleNewStatGroup(const TArray<FStatNameAndInfo>& NameAndInfos)
{
	// #Stats: FStatNameAndInfo should be private and visible only to stats2 system
	for (int32 InfoIdx = 0; InfoIdx < NameAndInfos.Num(); InfoIdx++)
	{
		const FStatNameAndInfo& NameAndInfo = NameAndInfos[InfoIdx];
		const FName GroupName = NameAndInfo.GetGroupName();
		const FName GroupCategory = NameAndInfo.GetGroupCategory();
		const FText GroupDescription = FText::FromString(NameAndInfo.GetDescription());	// @todo localize description?
		HandleNewStat(GroupName, GroupCategory, GroupDescription);
	}
}

void FSodalViewportCommands::HandleNewStat(const FName& InStatName, const FName& InStatCategory, const FText& InStatDescription)
{
	FString CommandName = InStatName.ToString();
	if (CommandName.RemoveFromStart(TEXT("STATGROUP_")) || CommandName.RemoveFromStart(TEXT("STAT_")))
	{
		// Trim the front to get our category name
		FString GroupCategory = InStatCategory.ToString();
		if (!GroupCategory.RemoveFromStart(TEXT("STATCAT_")))
		{
			GroupCategory.Empty();
		}

		// If we already have an entry (which can happen if a category has changed [when loading older saved stat data]) or we don't have a valid category then skip adding
		if (!FInputBindingManager::Get().FindCommandInContext(this->GetContextName(), InStatName).IsValid() && !GroupCategory.IsEmpty())
		{
			// Find or Add the category
			TArray<FShowMenuCommand>* ShowStatCommands = ShowStatCatCommands.Find(GroupCategory);
			if (!ShowStatCommands)
			{
				// New category means we'll need to resort
				ShowStatCatCommands.Add(GroupCategory);
				ShowStatCatCommands.KeySort(TLess<FString>());
				ShowStatCommands = ShowStatCatCommands.Find(GroupCategory);
			}

			const int32 NewIndex = FindStatIndex(ShowStatCommands, CommandName);
			if (NewIndex != INDEX_NONE)
			{
				const FText DisplayName = FText::FromString(CommandName);

				FText DescriptionName = InStatDescription;
				FFormatNamedArguments Args;
				Args.Add(TEXT("StatName"), DisplayName);
				if (DescriptionName.IsEmpty())
				{
					DescriptionName = FText::Format(NSLOCTEXT("UICommands", "StatShowCommandName", "Show {StatName} Stat"), Args);
				}

				TSharedPtr<FUICommandInfo> StatCommand
					= FUICommandInfoDecl(this->AsShared(), InStatName, DisplayName, DescriptionName)
					.UserInterfaceType(EUserInterfaceActionType::ToggleButton);

				FSodalViewportCommands::FShowMenuCommand ShowStatCommand(StatCommand, DisplayName);
				ShowStatCommands->Insert(ShowStatCommand, NewIndex);
				NewStatCommandDelegate.Broadcast(ShowStatCommand.ShowMenuItem, ShowStatCommand.LabelOverride.ToString());
			}
		}
	}
}

int32 FSodalViewportCommands::FindStatIndex(const TArray< FShowMenuCommand >* ShowStatCommands, const FString& InCommandName) const
{
	check(ShowStatCommands);
	for (int32 StatIndex = 0; StatIndex < ShowStatCommands->Num(); ++StatIndex)
	{
		const FString CommandName = (*ShowStatCommands)[StatIndex].LabelOverride.ToString();
		const int32 Compare = InCommandName.Compare(CommandName);
		if (Compare == 0)
		{
			return INDEX_NONE;
		}
		else if (Compare < 0)
		{
			return StatIndex;
		}
	}
	return ShowStatCommands->Num();
}


} // namespace soda

#undef LOCTEXT_NAMESPACE
