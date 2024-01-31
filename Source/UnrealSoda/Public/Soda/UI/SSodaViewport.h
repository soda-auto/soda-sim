// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Layout/Visibility.h"
#include "Styling/SlateColor.h"
#include "Input/Reply.h"
#include "Widgets/SOverlay.h"
#include "Widgets/SViewport.h"
#include "Widgets/Input/SButton.h"
#include "Soda/SodaTypes.h"
#include "Engine/GameViewportClient.h"
#include "Soda/UI/SResetScaleBox.h"
#include "Templates/SharedPointer.h"
#include "UObject/WeakObjectPtrTemplates.h"

class FUICommandList;
class USodaGameViewportClient;
class STooltipPresenter;
class FUICommandInfo;
class SSplitter;


namespace soda
{
class SMenuWindowContent;
class SMessageBox;
class SMenuWindow;
class SToolBox;
class SWaitingPanel;
class SOutputLog;
enum class EMessageBoxType : uint8;

enum class EUIMode : uint8
{
	Free,
	Editing
};

class SToolBox;

class  SSodaViewport : public SResetScaleBox
{
public:
	SLATE_BEGIN_ARGS(SSodaViewport)
	{ }
	SLATE_END_ARGS()
	
	SSodaViewport();
	virtual ~SSodaViewport();

	void Construct( const FArguments& InArgs, UWorld * World);

	void SetUIMode(EUIMode InMode);
	EUIMode GetUIMode() const;

	virtual void SetToolBoxWidget(TSharedRef<SToolBox> Widget);
	virtual void RemoveToolBoxWidget();

	void PushToolBox(TSharedRef<soda::SToolBox> Widget, bool InstedPrev = false);
	bool PopToolBox();

	TSharedPtr<soda::SMessageBox> ShowMessageBox(soda::EMessageBoxType Type, const FString& Caption, const FString& Text);
	TSharedPtr<soda::SMenuWindow> OpenWindow(const FString& Caption, TSharedRef<soda::SMenuWindowContent> Content);
	bool CloseWindow(soda::SMenuWindow* Wnd);
	bool CloseWindow(bool bCloseAllWindows = false);

	TSharedPtr<soda::SWaitingPanel> ShowWaitingPanel(const FString& Caption, const FString& SubCaption);
	bool CloseWaitingPanel(soda::SWaitingPanel* WaitingPanel);
	bool CloseWaitingPanel(bool bCloseAll = false);

	void BackMenu();

	/**
	 * Adds a widget to the Slate viewport's overlay (i.e for in game UI or tools) at the specified Z-order
	 *
	 * @param  ViewportContent	The widget to add.  Must be valid.
	 * @param  ZOrder  The Z-order index for this widget.  Larger values will cause the widget to appear on top of widgets with lower values.
	 */
	virtual void AddViewportWidgetContent(TSharedRef<class SWidget> ViewportContent, const int32 ZOrder = 0);

	/**
	 * Removes a previously-added widget from the Slate viewport
	 *
	 * @param	ViewportContent  The widget to remove.  Must be valid.
	 */
	virtual void RemoveViewportWidgetContent(TSharedRef<class SWidget> ViewportContent);

	const TSharedPtr<FUICommandList> GetCommandList() const { return CommandList; }
	UWorld* GetWorld() const { return World.Get(); }
	FViewport* GetViewport() const { return Viewport; }
	USodaGameViewportClient* GetViewportClient() const { return ViewportClient.Get(); }

public:
	virtual TSharedPtr<SWidget> MakeViewportToolbar();
	virtual void BindCommands();
	
	void OnToggleStats();
	virtual void ToggleStatCommand(FString CommandName);
	virtual bool IsStatCommandVisible(FString CommandName) const;

	void OnToggleAllStatCommands(bool bVisible);

	bool IsWidgetModeActive(soda::EWidgetMode Mode) const;
	bool CanSetWidgetMode(soda::EWidgetMode NewMode) const;

	bool IsCoordSystemActive(soda::ECoordSystem CoordSystem) const;
	void OnCycleCoordinateSystem();

	void SetViewportType(ESodaSpectatorMode Mode);
	bool IsActiveViewportType(ESodaSpectatorMode Mode);

	bool IsLogVisible() const;
	void SetLogVisible(bool bVisible);

	void OnToggleStats() const;
	void ToggleStatCommand(FString CommandName) const;
	void BindStatCommand(const TSharedPtr<FUICommandInfo> InMenuItem, const FString& InCommandName);

protected:
	//~ Begin SWidget overrides
	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;
	//virtual bool SupportsKeyboardFocus() const override;
	//virtual FReply OnFocusReceived(const FGeometry& MyGeometry, const FFocusEvent& InFocusEvent) override;
	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;
	virtual void OnDragEnter(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent) override;
	virtual void OnDragLeave(const FDragDropEvent& DragDropEvent) override;
	virtual FReply OnDragOver(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent) override;
	virtual FReply OnDrop(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent) override;
	virtual bool OnVisualizeTooltip(const TSharedPtr<SWidget>& TooltipContent) override;
	//~ End SWidget overrides

	//virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;

	/** An internal handler for dagging dropable objects into the viewport. */
	bool HandleDragObjects(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent);

	/** An internal handler for dropping objects into the viewport.
	 *	@param DragDropEvent		The drag event.
	 *	@param bCreateDropPreview	If true, a drop preview actor will be spawned instead of a normal actor.
	 */
	bool HandlePlaceDraggedObjects(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent, bool bCreateDropPreview);

protected:	
	float ToolBoxSize = 0.33;
	mutable float CashedScale = 1;
	EUIMode UIMode = EUIMode::Free;

	/** Commandlist used in the viewport (Maps commands to viewport specific actions) */
	TSharedPtr<FUICommandList> CommandList;
	
	TObjectPtr<UWorld> World = nullptr;
	FViewport* Viewport = nullptr;
	TWeakObjectPtr<USodaGameViewportClient> ViewportClient;

	TSharedPtr<SSplitter> MajorSplitter;
	TSharedPtr<SSplitter> MinorSplitter;
	TSharedPtr<STooltipPresenter> TooltipPresenter;
	TSharedPtr<SOverlay> ViewportOverlayWidget;
	TSharedPtr<soda::SOutputLog> OutputLog;

	TArray<TSharedPtr<soda::SToolBox>> StackToolBox;
	TArray<TSharedPtr<soda::SMenuWindow>> OpenedWindows;
	TArray<TSharedPtr<soda::SWaitingPanel>> WaitingPanels;
};

} // namespace soda