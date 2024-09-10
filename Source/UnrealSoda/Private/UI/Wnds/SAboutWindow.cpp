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
#include "Soda/LevelState.h"
#include "SodaStyleSet.h"

namespace soda
{

void SAboutWindow::Construct(const FArguments& InArgs)
{
	
	USodaSubsystem* SodaSubsystem = USodaSubsystem::Get();
	check(SodaSubsystem);
	//UWorld * World = SodaSubsystem->GetWorld();
	//check(World);

	FString LevelSource;

	switch (SodaSubsystem->LevelState->Slot.SlotSource)
	{
	case ELeveSlotSource::Local: LevelSource = "Local"; break;
	case ELeveSlotSource::Remote: LevelSource = "Remote"; break;
	case ELeveSlotSource::NoSlot: LevelSource = "NoSlot"; break;
	case ELeveSlotSource::NewSlot: LevelSource = "NewSlot"; break;
	}

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
			.AutoHeight()
			.Padding(0, 0, 0, 0)
			[
				SNew(STextBlock)
				.Text(FText::FromString(FString("Level Info: " ) + SodaSubsystem->LevelState->Slot.LevelName))
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(15, 0, 0, 0)
			[
				SNew(STextBlock)
				.Text(FText::FromString(FString("LevelName: " ) + SodaSubsystem->LevelState->Slot.LevelName))
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(15, 0, 0, 0)
			[
				SNew(STextBlock)
				.Text(FText::FromString(FString("Source: " ) + LevelSource))
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(15, 0, 0, 0)
			[
				SNew(STextBlock)
				.Text(FText::FromString(FString("Description: " ) + SodaSubsystem->LevelState->Slot.Description))
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(15, 0, 0, 0)
			[
				SNew(STextBlock)
				.Text(FText::FromString(FString("DateTime: " ) + SodaSubsystem->LevelState->Slot.DateTime.ToString()))
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(15, 0, 0, 0)
			[
				SNew(STextBlock)
				.Text(FText::FromString(FString("SlotIndex: " ) + FString::FromInt(SodaSubsystem->LevelState->Slot.SlotIndex)))
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(15, 0, 0, 0)
			[
				SNew(STextBlock)
				.Text(FText::FromString(FString("ScenarioID: " ) + FString::FromInt(SodaSubsystem->LevelState->Slot.ScenarioID)))
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

