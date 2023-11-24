// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "SViewportToolBarMenuButton.h"
#include "Widgets/Input/SMenuAnchor.h"

namespace soda
{

void SViewportToolBarMenuButton::Construct(const FArguments& InArgs, TSharedRef<SMenuAnchor> InMenuAnchor)
{
	SButton::Construct(SButton::FArguments()
		// Allows users to drag with the mouse to select options after opening the menu
		.ClickMethod(EButtonClickMethod::MouseDown)
		.OnClicked(InArgs._OnClicked)
		.ForegroundColor(InArgs._ForegroundColor)
		.HAlign(InArgs._HAlign)
		.VAlign(InArgs._VAlign)
		.ContentPadding(InArgs._ContentPadding)
		.ButtonStyle(InArgs._ButtonStyle)
		.IsFocusable(false)
		[
			InArgs._Content.Widget
		]
	);

	SetAppearPressed(TAttribute<bool>::CreateSP(this, &SViewportToolBarMenuButton::IsMenuOpen));
	MenuAnchor = InMenuAnchor;
}

bool SViewportToolBarMenuButton::IsMenuOpen() const
{
	if (TSharedPtr<SMenuAnchor> MenuAnchorPinned = MenuAnchor.Pin())
	{
		return MenuAnchorPinned->IsOpen();
	}

	return false;
}

}