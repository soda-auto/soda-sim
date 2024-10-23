// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "UI/Wnds/SQuickStartWindow.h"
#include "SodaStyleSet.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Input/SHyperlink.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SWrapBox.h"
#include "Soda/SodaSubsystem.h"
#include "Soda/SodaApp.h"
#include "Soda/SodaUserSettings.h"
#include "SodaStyleSet.h"
#include "Widgets/Layout/SUniformGridPanel.h"
#include "Soda/LevelState.h"

namespace soda
{

FString DemoLevel = TEXT("VirtualProvingGround");

TSharedRef<SWidget> MakeTextItem(FName Icon, const FString& CaptionRow1, const FString& CaptionRow2 = TEXT(""))
{
	return
		SNew(SBox)
		.WidthOverride(130)
		.HeightOverride(130)
		.Padding(10)
		[
			SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				.AutoHeight()
				[
					SNew(SImage)
					.Image(FSodaStyle::GetBrush(Icon))
				]
				+ SVerticalBox::Slot()
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				.AutoHeight()
				[
					SNew(STextBlock)
					.Text(FText::FromString(CaptionRow1))
					.AutoWrapText(true)
				]
				+ SVerticalBox::Slot()
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				.AutoHeight()
				[
					SNew(STextBlock)
						.Text(FText::FromString(CaptionRow2))
						.AutoWrapText(true)
				]
		];
};

TSharedRef<SWidget> MakeLinkItem(FName Icon, const FString& Caption, const FString& Url)
{
	return
		SNew(SBox)
		.WidthOverride(130)
		.HeightOverride(130)
		.Padding(10)
		[
			SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				.AutoHeight()
				[
					SNew(SImage)
					.Image(FSodaStyle::GetBrush(Icon))
					.OnMouseButtonDown_Lambda([=](const FGeometry&, const FPointerEvent& ){ FPlatformProcess::LaunchURL(*Url, nullptr, nullptr); return FReply::Handled(); })
					.Cursor(EMouseCursor::Hand)
				]
				+ SVerticalBox::Slot()
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				.AutoHeight()
				[
					SNew(SHyperlink)
					.Text(FText::FromString(Caption))
					.OnNavigate_Lambda([=]() { FPlatformProcess::LaunchURL(*Url, nullptr, nullptr); })
					.Style(FSodaStyle::Get(), "HoverOnlyHyperlink")
				]
		];
};

TSharedRef<SWidget> MakeScenarioItem(const FString& Caption, int SlotIndex)
{
	return
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Center)
		[
			SNew(SImage)
			.Image(FSodaStyle::GetBrush("Symbols.RightArrow"))
		]
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Center)
		.Padding(5, 0, 0, 0)
		[
			SNew(SHyperlink)
			.Text(FText::FromString(Caption))
			.OnNavigate_Lambda([=]() 
			{ 
					ALevelState::ReloadLevelFromSlotLocally(nullptr, SlotIndex, DemoLevel);
			})
			.Style(FSodaStyle::Get(), "HoverOnlyHyperlink")
		];
}

void SQuickStartWindow::Construct(const FArguments& InArgs)
{
	TArray<FLevelStateSlotDescription> Slots;
	ALevelState::GetLevelSlotsLocally(nullptr, Slots, false, DemoLevel);
	TSharedRef<SVerticalBox> DemoList = SNew(SVerticalBox);
	for (const auto& Slot : Slots)
	{
		if (Slot.Description.StartsWith(TEXT("Demo")))
		{
			DemoList->AddSlot()
			[
				MakeScenarioItem(Slot.Description, Slot.SlotIndex)
			];
		}
	}

	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		[
			SNew(SBorder)
			.BorderImage(FSodaStyle::GetBrush("MenuWindow.Content"))
			[
				SNew(SScrollBox)
				.Orientation(Orient_Vertical)
				+ SScrollBox::Slot()
				[
					SNew(SBox)
					.Padding(10)
					.MinDesiredWidth(600)
					//.MinDesiredHeight(150)
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0, 0, 0, 0)
						[
							SNew(STextBlock)
							.Text(FText::FromString("Basic Controls"))
							.Font(FSodaStyle::GetFontStyle("Font.Large"))
						]
						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0, 0, 0, 0)
						.HAlign(HAlign_Left)
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot()
							[
								MakeTextItem("QuickStart.Icons.WASD", TEXT("Move Vehicle /"), TEXT("Move Camera"))
							]
							+ SHorizontalBox::Slot()
							[
								MakeTextItem("QuickStart.Icons.Enter", TEXT("Possess Vehicle /"), TEXT("Free Camera"))
							]
							+ SHorizontalBox::Slot()
							[
								MakeTextItem("QuickStart.Icons.Mouse", TEXT("Look Around"))
							]
						]
						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0, 0, 0, 5)
						[
							SNew(STextBlock)
							.Text(FText::FromString("Tips"))
							.Font(FSodaStyle::GetFontStyle("Font.Large"))
						]
						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(20, 0, 0, 5)
						[
							SNew(SVerticalBox)
							+ SVerticalBox::Slot()
							[
								SNew(SWrapBox)
								.UseAllottedSize(true)
								.Orientation(Orient_Horizontal)
								+ SWrapBox::Slot()
								[
									SNew(STextBlock)
									.Text(FText::FromString("By default, low graphics settings are used. Go to"))
								]
								+ SWrapBox::Slot()
								.Padding(3,0,3,0)
								[
									SNew(SImage)
									.Image(FSodaStyle::GetBrush("Icons.Toolbar.Settings"))
								]
								+ SWrapBox::Slot()
								[
									SNew(STextBlock)
									.Text(FText::FromString("Settings ->"))
									.Font(FSodaStyle::GetFontStyle("NormalFontBold"))
								]
								+ SWrapBox::Slot()
								.Padding(3,0,3,0)
								[
									SNew(SImage)
									.Image(FSodaStyle::GetBrush("Icons.Adjust"))
								]
								+ SWrapBox::Slot()
								[
									SNew(STextBlock)
									.Text(FText::FromString("Common Settings -> "))
									.Font(FSodaStyle::GetFontStyle("NormalFontBold"))
								]
								+ SWrapBox::Slot()
								[
									SNew(STextBlock)
									.Text(FText::FromString("Graphics Settings "))
									.Font(FSodaStyle::GetFontStyle("NormalFontBold"))
								]
								+ SWrapBox::Slot()
								[
									SNew(STextBlock)
									.Text(FText::FromString("to increase the graphics quality."))
								]
							]
							/*
							+ SVerticalBox::Slot()
							[
								SNew(STextBlock)
								.Text(FText::FromString("Tip2"))
							]
							*/
						]
						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0, 10, 0, 0)
						[
							SNew(STextBlock)
							.Text(FText::FromString("Demo Scenarios"))
							.Font(FSodaStyle::GetFontStyle("Font.Large"))
						]
						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(20, 0, 0, 0)
						[
							DemoList
						]
						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0, 15, 0, 0)
						[
							SNew(STextBlock)
							.Text(FText::FromString("Useful Resources"))
							.Font(FSodaStyle::GetFontStyle("Font.Large"))
						]
						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0, 0, 0, 0)
						.HAlign(HAlign_Left)
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot()
							[
								MakeLinkItem("QuickStart.Icons.YouTube", TEXT("Quick Start"), TEXT("https://www.youtube.com/watch?v=_IWOFE-wUS4"))
							]
							+ SHorizontalBox::Slot()
							[
								MakeLinkItem("QuickStart.Icons.YouTube", TEXT("YouTube Channel"), TEXT("https://www.youtube.com/@soda-sim/videos"))
							]
							+ SHorizontalBox::Slot()
							[
								MakeLinkItem("QuickStart.Icons.Git", TEXT("GitHub"), TEXT("https://github.com/soda-auto/soda-sim"))
							]
							+ SHorizontalBox::Slot()
							[
								MakeLinkItem("QuickStart.Icons.Docs", TEXT("Documentation"), TEXT("https://docs.soda.auto/projects/soda-sim/en/latest/"))
							]
						]
					]
				]
			]
		]
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
				.Text(FText::FromString(TEXT("Show at Startup")))
			]
			+SHorizontalBox::Slot()
			.Padding(FMargin(5, 0, 0, 0))
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
	];
}

} // namespace soda

