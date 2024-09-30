// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "UI/Wnds/SQuickStartWindow.h"
#include "SodaStyleSet.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/SBoxPanel.h"
#include "Soda/SodaSubsystem.h"
#include "Soda/SodaApp.h"
#include "Soda/SodaUserSettings.h"
#include "SodaStyleSet.h"

namespace soda
{

void SQuickStartWindow::Construct(const FArguments& InArgs)
{
	ChildSlot
	[
		SNew(SBox)
		.Padding(5)
		.MinDesiredWidth(500)
		.MinDesiredHeight(150)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0, 0, 0, 5)
			[
				SNew(STextBlock)
				.Text(FText::FromString("Controls"))
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0, 0, 0, 5)
			[
				SNew(STextBlock)
				.Text(FText::FromString("Quick Start"))
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0, 0, 0, 0)
			[
				SNew(STextBlock)
				.Text(FText::FromString("Demo Scenarios"))
			]
			+ SVerticalBox::Slot()
			.FillHeight(1)
			+ SVerticalBox::Slot()
			.Padding(3)
			.AutoHeight()
			.HAlign(HAlign_Right)
			.VAlign(VAlign_Center)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("Show at StartUp")))
				]
				+SHorizontalBox::Slot()
				//.Padding(FMargin(18, 0, 17, 0))
				.HAlign(HAlign_Right)
				.AutoWidth()
				[
					SNew(SCheckBox)
					.OnCheckStateChanged_Lambda([](ECheckBoxState NewState) 
					{
						SodaApp.GetSodaUserSettings()->bShowQuickStartAtStartUp = (NewState == ECheckBoxState::Checked ? true : false);
						SodaApp.GetSodaUserSettings()->SaveSettings();
					})
					.IsChecked_Lambda([]() 
					{ 
						return SodaApp.GetSodaUserSettings()->bShowQuickStartAtStartUp ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
					})
				]
			]
			+ SVerticalBox::Slot()
			.Padding(3)
			.AutoHeight()
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			[
				SNew(SButton)
				.Text(FText::FromString("Close"))
				.OnClicked_Lambda([this]() { CloseWindow(); return FReply::Handled(); })
			]

		]
	];
}

} // namespace soda

