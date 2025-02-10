// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Layout/Visibility.h"
#include "Widgets/SWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Soda/UI/SSodaViewport.h"
#include "UI/Toolbar/Common/SViewportToolBar.h"
#include "Widgets/Input/SSpinBox.h"

class FExtender;
enum class EUIMode : uint8;

namespace soda
{

/**
 * A level viewport toolbar widget that is placed in a viewport
 */
class SSodaViewportToolBar : public SViewportToolBar
{
public:
	SLATE_BEGIN_ARGS( SSodaViewportToolBar ){}
		SLATE_ARGUMENT( TSharedPtr<class SSodaViewport>, Viewport )
	SLATE_END_ARGS()

	void Construct( const FArguments& InArgs );

	/** @return Level editor viewport client. */ 
	//FLevelEditorViewportClient* GetLevelViewportClient();

	/** Gets the world we are editing */
	TWeakObjectPtr<UWorld> GetWorld() const;

private:
	// Mode Menu
	FText GetModeMenuLabel() const;
	const FSlateBrush* GetModeMenuLabelIcon() const;
	void ChangeMode(EUIMode InMode) const;
	bool ModeIs(EUIMode InMode) const;

	void OnOpenLevelWindow();
	void OnOpenPakWindow();
	void OnOpenAboutWindow();
	void OnOpenQuickStartWindow();
	void OnOpenVehicleManagerWindow();
	void OnOpenScenariosManagerWindow();
	void OnOpenSaveLoadWindow();

	EVisibility IsEditorMode() const;
	EVisibility IsSpectatorMode() const;

private:
	TSharedRef<SWidget> GenerateMainMenu();
	TSharedRef<SWidget> GenerateOptionsMenu();
	TSharedRef<SWidget> GenerateAddMenu();
	TSharedRef<SWidget> GenerateModeMenu();
	TSharedRef<SWidget> GenerateDBMenu();

	/** Gets the extender for the view menu */
	TSharedPtr<FExtender> GetViewMenuExtender();

	/** The viewport that we are in */
	TWeakPtr<SSodaViewport> Viewport;
};

} // namespace soda

