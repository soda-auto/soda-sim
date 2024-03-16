// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Misc/Attribute.h"
#include "Layout/Visibility.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Input/Reply.h"
#include "Widgets/SCompoundWidget.h"
#include "SViewportToolBar.h"
#include "Framework/SlateDelegates.h"
#include "Styling/SlateTypes.h"
#include "SodaStyleSet.h"
#include "Styling/SlateWidgetStyleAsset.h"

class SMenuAnchor;
struct FSlateBrush;

namespace soda
{

namespace EMenuItemType
{
	enum Type
	{
		Default,
		Header,
		Separator,
	};
}


/**
 * Widget that opens a menu when clicked
 */
class SViewportToolbarMenu : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SViewportToolbarMenu)
		: _MenuStyle(&FSodaStyle::GetWidgetStyle<FButtonStyle>("EditorViewportToolBar.Button"))
		, _ForegroundColor(FSlateColor::UseStyle())
	{}
		/** We need to know about the toolbar we are in */
		SLATE_ARGUMENT(TSharedPtr<SViewportToolBar>, ParentToolBar);
		/** Style to use */
		SLATE_STYLE_ARGUMENT(FButtonStyle, MenuStyle)
		/** The label to show in the menu */
		SLATE_ATTRIBUTE(FText, Label)
		/** Optional icon to display next to the label */
		SLATE_ATTRIBUTE(const FSlateBrush*, LabelIcon)
		/** Optional overlay icon to display next to the label */
		SLATE_ATTRIBUTE(const FSlateBrush*, OverlayLabelIcon)
		/** The image to show in the menu.  If both the label and image are valid, the button image is used.  Note that if this image is used, the label icon will not be displayed. */
		SLATE_ARGUMENT(FName, Image)
		/** Content to show in the menu */
		SLATE_EVENT(FOnGetContent, OnGetMenuContent)
		/** The foreground color of the content */
		SLATE_ATTRIBUTE(FSlateColor, ForegroundColor)
	SLATE_END_ARGS()

	/**
	 * Constructs the menu
	 */
	void Construct( const FArguments& Declaration );

	/**
	 * Returns parent tool bar
	 */
	TWeakPtr<SViewportToolBar> GetParentToolBar() const;

	/**
	 * @return true if this menu is open
	 */
	bool IsMenuOpen() const;

private:
	/**
	 * Called when the menu button is clicked.  Will toggle the visibility of the menu content                   
	 */
	FReply OnMenuClicked();

	/**
	 * Called when the mouse enters a menu button.  If there was a menu previously opened
	 * we open this menu automatically
	 */
	void OnMouseEnter( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent );

	EVisibility GetLabelIconVisibility() const;

protected:
	/** Parent tool bar for querying other open menus */
	TWeakPtr<SViewportToolBar> ParentToolBar;

	/** Name of tool menu */
	FName MenuName;

	/**
	 * Called to query the tool tip text for this widget, but will return an empty text for menu bar items
	 * when a menu for that menu bar is already open
	 *
	 * @param	ToolTipText	Tool tip text to display, if possible
	 *
	 * @return	Tool tip text, or an empty text if filtered out
	 */
	FText GetFilteredToolTipText(TAttribute<FText> ToolTipText) const;

private:
	/** Our menus anchor */
	TSharedPtr<SMenuAnchor> MenuAnchor;
	TAttribute<const FSlateBrush*> LabelIconBrush;
	TAttribute<const FSlateBrush*> OverlayLabelIconBrush;
};

} // namespace soda