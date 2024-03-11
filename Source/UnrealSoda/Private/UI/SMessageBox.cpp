// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/UI/SMessageBox.h"
#include "SodaStyleSet.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"

namespace soda
{

SMessageBox::SMessageBox()
{
}

SMessageBox::~SMessageBox()
{
}

void SMessageBox::Construct( const FArguments& InArgs )
{
	OnMessageBox = InArgs._OnMessageBox;

	TSharedPtr<SHorizontalBox> ButtonsHorizontalBox;
	SMenuWindow::Construct(SMenuWindow::FArguments()
		.Caption(InArgs._Caption)
		.Content(
			SNew(SMenuWindowContent)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.Padding(5)
				.FillHeight(1)
				[
					SNew(SBox)
					.MaxDesiredWidth(500)
					[
						SNew(STextBlock)
						.AutoWrapText(true)
						.Text(InArgs._TextContent)
					]
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.HAlign(HAlign_Center)
				[
					SAssignNew(ButtonsHorizontalBox, SHorizontalBox)
				]
			]
		)
	);

	if (InArgs._Type.Get() == EMessageBoxType::OK || InArgs._Type.Get() == EMessageBoxType::OK_CANCEL)
	{
		ButtonsHorizontalBox->AddSlot()
		.Padding(3)
		.AutoWidth()
		[
			SNew(SButton)
			.Text(FText::FromString("Ok"))
			.OnClicked(FOnClicked::CreateLambda([this]() {
				ExecuteOnMessageBox(EMessageBoxButton::OK);
				return FReply::Handled();
			}))
		];
	}
	else if (InArgs._Type.Get() == EMessageBoxType::YES_NO || InArgs._Type.Get() == EMessageBoxType::YES_NO_CANCEL)
	{
		ButtonsHorizontalBox->AddSlot()
		.Padding(3)
		.AutoWidth()
		[
			SNew(SButton)
			.Text(FText::FromString("Yes"))
			.OnClicked(FOnClicked::CreateLambda([this]() {
				ExecuteOnMessageBox(EMessageBoxButton::YES);
				return FReply::Handled();
			}))
		];

		ButtonsHorizontalBox->AddSlot()
		.Padding(3)
		.AutoWidth()
		[
			SNew(SButton)
			.Text(FText::FromString("No"))
			.OnClicked(FOnClicked::CreateLambda([this]() {
				ExecuteOnMessageBox(EMessageBoxButton::NO);
				return FReply::Handled();
			}))
		];
	}

	if (InArgs._Type.Get() == EMessageBoxType::OK_CANCEL || InArgs._Type.Get() == EMessageBoxType::YES_NO_CANCEL)
	{
		ButtonsHorizontalBox->AddSlot()
		.Padding(3)
		.AutoWidth()
		[
			SNew(SButton)
			.Text(FText::FromString("Cancl"))
			.OnClicked(FOnClicked::CreateLambda([this]() {
				ExecuteOnMessageBox(EMessageBoxButton::CANCEL);
				return FReply::Handled();
			}))
		];
	}
}

void SMessageBox::ExecuteOnMessageBox(EMessageBoxButton Button)
{
	if (OnMessageBox.IsBound())
	{
		OnMessageBox.Execute(Button);
	}

	CloseWindow();
}

void SMessageBox::SetOnMessageBox(FOnMessageBox InOnMessageBox)
{
	OnMessageBox = InOnMessageBox;
}

} // namespace soda

