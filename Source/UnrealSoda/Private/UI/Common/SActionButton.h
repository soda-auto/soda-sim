// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "Framework/SlateDelegates.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Input/SComboButton.h"
#include "Styling/StyleColors.h"

/** A Button that is used to call out/highlight a positive option (Add, Save etc). It can also be used to open a menu.
*/
class UNREALSODA_API SActionButton : public SCompoundWidget
{
public:

	SLATE_BEGIN_ARGS(SActionButton) 
		: _Icon(FAppStyle::Get().GetBrush("Icons.Plus"))
		, _IconColor(FStyleColors::AccentGreen)
		, _ContentPadding(FMargin(2.0f, 3.0f))
		, _ButtonStyle(&FCoreStyle::Get().GetWidgetStyle< FButtonStyle >("Button"))
		{}

		/** The text to display in the button. */
		SLATE_ATTRIBUTE(FText, Text)

		SLATE_ATTRIBUTE(const FSlateBrush*, Icon)

		SLATE_ATTRIBUTE(FSlateColor, IconColor)
			
		SLATE_ATTRIBUTE( FMargin, ContentPadding )

		SLATE_STYLE_ARGUMENT( FButtonStyle, ButtonStyle )

		/** The clicked handler. Note that if this is set, the button will behave as though it were just a button.
		 * This means that OnGetMenuContent, OnComboBoxOpened and OnMenuOpenChanged will all be ignored, since there is no menu.
		 */
		SLATE_EVENT(FOnClicked, OnClicked)

		/** The static menu content widget. */
		SLATE_NAMED_SLOT(FArguments, MenuContent)

		SLATE_EVENT(FOnGetContent, OnGetMenuContent)
		SLATE_EVENT(FOnComboBoxOpened, OnComboBoxOpened)
		SLATE_EVENT(FOnIsOpenChanged, OnMenuOpenChanged)

	SLATE_END_ARGS()

	SActionButton() {}

	void Construct(const FArguments& InArgs);

	void SetMenuContentWidgetToFocus(TWeakPtr<SWidget> Widget);
	void SetIsMenuOpen(bool bIsOpen, bool bIsFocused);

private:

	TSharedPtr<SComboButton> ComboButton;
	TSharedPtr<class SButton> Button;
};