// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once 

#include "CoreMinimal.h"
#include "Engine/EngineBaseTypes.h"
#include "Widgets/SWidget.h"
#include "Framework/Commands/Commands.h"
#include "SodaStyleSet.h"
#include "Engine/TextureStreamingTypes.h"

class FUICommandList;


namespace soda
{
/**
 * Class containing commands for editor viewport actions common to all viewports
 */
class UNREALSODA_API FSodalViewportCommands : public TCommands<FSodalViewportCommands>
{
public:
	FSodalViewportCommands();
	virtual ~FSodalViewportCommands();

	struct FShowMenuCommand
	{
		TSharedPtr<FUICommandInfo> ShowMenuItem;
		FText LabelOverride;

		FShowMenuCommand(TSharedPtr<FUICommandInfo> InShowMenuItem, const FText& InLabelOverride)
			: ShowMenuItem(InShowMenuItem)
			, LabelOverride(InLabelOverride)
		{
		}

		FShowMenuCommand(TSharedPtr<FUICommandInfo> InShowMenuItem)
			: ShowMenuItem(InShowMenuItem)
		{
		}
	};

	/** Hides all Stats categories  */
	TSharedPtr< FUICommandInfo > HideAllStats;

	/** A map of stat categories and the commands that belong in them */
	TMap< FString, TArray< FShowMenuCommand > > ShowStatCatCommands;

	/** Delegate we fire every time a new stat has been had a command added */
	DECLARE_EVENT_TwoParams(FSodalViewportCommands, FOnNewStatCommandAdded, const TSharedPtr<FUICommandInfo>, const FString&);
	static FOnNewStatCommandAdded NewStatCommandDelegate;

	/** Restart Level*/
	TSharedPtr< FUICommandInfo > RestartLevel;

	/** New Level*/
	TSharedPtr< FUICommandInfo > ClearLevel;
	
	/** Changes the viewport to perspective view */
	TSharedPtr< FUICommandInfo > Perspective;

	/** Changes the viewport to orthographic free view */
	TSharedPtr< FUICommandInfo > OrthographicFree;

	/** Changes the viewport to top view */
	TSharedPtr< FUICommandInfo > Top;

	/** Changes the viewport to bottom view */
	TSharedPtr< FUICommandInfo > Bottom;

	/** Changes the viewport to left view */
	TSharedPtr< FUICommandInfo > Left;

	/** Changes the viewport to right view */
	TSharedPtr< FUICommandInfo > Right;

	/** Changes the viewport to front view */
	TSharedPtr< FUICommandInfo > Front;

	/** Changes the viewport to back view */
	TSharedPtr< FUICommandInfo > Back;

	/** Rotate through viewport view options */
	TSharedPtr< FUICommandInfo > Next;

	/** Toggles showing stats in the viewport */
	TSharedPtr< FUICommandInfo > ToggleStats;

	/** Toggles showing fps in the viewport */
	TSharedPtr< FUICommandInfo > ToggleFPS;

	/** Toggles showing debug vehicle panel */
	TSharedPtr< FUICommandInfo > ToggleVehiclePanel;

	/** Toggles showing debug vehicle panel */
	TSharedPtr< FUICommandInfo > ToggleFullScreen;

	/** Select Mode */
	TSharedPtr< FUICommandInfo > SelectMode;

	/** Translate Mode */
	TSharedPtr< FUICommandInfo > TranslateMode;

	/** Rotate Mode */
	TSharedPtr< FUICommandInfo > RotateMode;

	/** Scale Mode */
	TSharedPtr< FUICommandInfo > ScaleMode;

	/** World relative coordinate system */
	TSharedPtr< FUICommandInfo > RelativeCoordinateSystem_World;

	/** Local relative coordinate system */
	TSharedPtr< FUICommandInfo > RelativeCoordinateSystem_Local;

	TSharedPtr< FUICommandInfo > CycleTransformGizmos;
	TSharedPtr< FUICommandInfo > CycleTransformGizmoCoordSystem;

	TSharedPtr< FUICommandInfo > RecordDataset;
	TSharedPtr< FUICommandInfo > AutoConnectDB;


	TSharedPtr< FUICommandInfo > ToggleSpectatorMode;
	TSharedPtr< FUICommandInfo > PossesNextVehicle;
	TSharedPtr< FUICommandInfo > BackMenu;
	TSharedPtr< FUICommandInfo > BackMenuEditor;
	TSharedPtr< FUICommandInfo > TagActors;
	TSharedPtr< FUICommandInfo > Exit;

public:
	/** Registers our commands with the binding system */
	virtual void RegisterCommands() override;

private:
	/** Registers additional commands as they are loaded */
	void HandleNewStatGroup(const TArray<FStatNameAndInfo>& NameAndInfos);
	void HandleNewStat(const FName& InStatName, const FName& InStatCategory, const FText& InStatDescription);
	int32 FindStatIndex(const TArray< FShowMenuCommand >* ShowStatCommands, const FString& InCommandName) const;
};

} // namespace soda