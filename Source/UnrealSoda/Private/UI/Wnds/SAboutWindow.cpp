// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "UI/Wnds/SAboutWindow.h"
#include "Soda/UnrealSodaVersion.h"
#include "SodaStyleSet.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/SBoxPanel.h"
#include "Soda/SodaSubsystem.h"

namespace soda
{

void SAboutWindow::Construct(const FArguments& InArgs)
{
	
	USodaSubsystem* SodaSubsystem = USodaSubsystem::GetChecked();

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
				.Text(FText::FromString("SodaSim (c)"))
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0, 0, 0, 5)
			[
				SNew(STextBlock)
				.Text(FText::FromString("Version: " UNREALSODA_VERSION_STRING))
			]
			+ SVerticalBox::Slot()
			.FillHeight(1)
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

