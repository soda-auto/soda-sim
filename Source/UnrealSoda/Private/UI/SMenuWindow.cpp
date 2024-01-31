// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/UI/SMenuWindow.h"
#include "SodaStyleSet.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SBackgroundBlur.h"
#include "Soda/SodaGameMode.h"

namespace soda
{

SMenuWindow::SMenuWindow()
{
}

SMenuWindow::~SMenuWindow()
{
}

void SMenuWindow::Construct( const FArguments& InArgs )
{
	check(InArgs._Content);

	InArgs._Content->SetParentMenuWindow(SharedThis(this));	

	ChildSlot
	[
		SNew(SBackgroundBlur)
		.VAlign(VAlign_Center)
		.HAlign(HAlign_Center)
		.BlurStrength(4)
		.Padding(20.0f)
		.CornerRadius(FVector4(4.0f, 4.0f, 4.0f, 4.0f))
		.Cursor(EMouseCursor::Default)
		[
			SNew(SBorder)
			.BorderImage(FSodaStyle::GetBrush("MenuWindow.Border"))
			.Padding(4)
			[
				SNew(SVerticalBox)
				+SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SBorder)
					.BorderImage(FSodaStyle::GetBrush("MenuWindow.Caption"))
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot()
						.Padding(5, 2, 5, 2)
						.FillWidth(1.0)
						[
							SNew(STextBlock)
							//.TextStyle(&FSodaStyle::Get().GetWidgetStyle<FTextBlockStyle>("SmallText"))
							.Text(InArgs._Caption)
						]
						+ SHorizontalBox::Slot()
						.Padding(2)
						.AutoWidth()
						[
							SNew(SButton)
							.ButtonStyle(FSodaStyle::Get(), "MenuWindow.CloseButton")
							.OnClicked(FOnClicked::CreateLambda([this]() {
								CloseWindow();
								return FReply::Handled();
							}))
						]
					]
				]
				+SVerticalBox::Slot()
				.Padding(0, 5, 0, 0)
				.FillHeight(1)
				[
					SNew(SBox)
					.MinDesiredWidth(200)
					.MinDesiredHeight(40)
					[
						InArgs._Content.ToSharedRef()
					]
				]
			]
		]
	];
}

bool SMenuWindow::CloseWindow()
{
	if (USodaGameModeComponent* GameMode = USodaGameModeComponent::Get())
	{
		if (!GameMode->CloseWindow(this))
		{
			checkf(true, TEXT("SMenuWindow::CloseWindow(), but the window isn't fins in the USodaGameModeComponent"));
		}
		else
		{
			return true;
		}
	}
	return false;
}


} // namespace soda

