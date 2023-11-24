// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "UI/Common/SActionButton.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Styling/StyleColors.h"

void SActionButton::Construct(const FArguments& InArgs)
{
	check(InArgs._Icon.IsSet());

	TAttribute<FText> Text = InArgs._Text;

	TSharedRef<SWidget> ButtonContent = SNullWidget::NullWidget;

	if (InArgs._Icon.Get())
	{
		ButtonContent = SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			.AutoWidth()
			[
				SNew(SImage)
				.Image(InArgs._Icon)
				.ColorAndOpacity(InArgs._IconColor)
			]
			+ SHorizontalBox::Slot()
			.Padding(FMargin(3, 0, 0, 0))
			.VAlign(VAlign_Center)
			.AutoWidth()
			[
				SNew(STextBlock)
				.TextStyle(FAppStyle::Get(), "SmallButtonText")
				.Text(InArgs._Text)
				.Visibility_Lambda([Text]() { return Text.Get(FText::GetEmpty()).IsEmpty() ? EVisibility::Collapsed : EVisibility::Visible; })
			];
	}
	else
	{
		ButtonContent =
			SNew(STextBlock)
			.TextStyle(FAppStyle::Get(), "SmallButtonText")
			.Text(InArgs._Text)
			.Visibility_Lambda([Text]() { return Text.Get(FText::GetEmpty()).IsEmpty() ? EVisibility::Collapsed : EVisibility::Visible; });
			
	}

	if (InArgs._OnClicked.IsBound())
	{
		ChildSlot
		[
			SAssignNew(Button, SButton)
			.ContentPadding(InArgs._ContentPadding)
			.ForegroundColor(FSlateColor::UseStyle())
			.ButtonStyle(InArgs._ButtonStyle)
			.IsEnabled(InArgs._IsEnabled)
			.ToolTipText(InArgs._ToolTipText)
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			.OnClicked(InArgs._OnClicked)
			[
				ButtonContent
			]
		];
	}
	else
	{
		ChildSlot
		[
			SAssignNew(ComboButton, SComboButton)
			.ContentPadding(InArgs._ContentPadding)
			.HasDownArrow(false)
			.ButtonStyle(InArgs._ButtonStyle)
			.ForegroundColor(FSlateColor::UseStyle())
			.IsEnabled(InArgs._IsEnabled)
			.ToolTipText(InArgs._ToolTipText)
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			.ButtonContent()
			[
				ButtonContent
			]
			.MenuContent()
			[
				InArgs._MenuContent.Widget
			]
			.OnGetMenuContent(InArgs._OnGetMenuContent)
			.OnMenuOpenChanged(InArgs._OnMenuOpenChanged)
			.OnComboBoxOpened(InArgs._OnComboBoxOpened)
		];
	}
}

void SActionButton::SetMenuContentWidgetToFocus(TWeakPtr<SWidget> Widget)
{
	check(ComboButton.IsValid());
	ComboButton->SetMenuContentWidgetToFocus(Widget);
}

void SActionButton::SetIsMenuOpen(bool bIsOpen, bool bIsFocused)
{
	check(ComboButton.IsValid());
	ComboButton->SetIsOpen(bIsOpen, bIsFocused);
}
