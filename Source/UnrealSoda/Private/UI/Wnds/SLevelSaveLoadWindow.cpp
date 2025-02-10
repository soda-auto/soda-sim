// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "UI/Wnds/SLevelSaveLoadWindow.h"
#include "Soda/UI/SFileDatabaseManager.h"
#include "Soda/SodaApp.h"
#include "SodaStyleSet.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Layout/SWidgetSwitcher.h"
#include "Widgets/Input/SComboButton.h"
#include "Soda/UI/SMessageBox.h"
#include "Soda/LevelState.h"
#include "Soda/SodaSubsystem.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "RuntimeEditorUtils.h"

namespace soda
{


void SLevelSaveLoadWindow::Construct( const FArguments& InArgs )
{
	USodaSubsystem* SodaSubsystem = USodaSubsystem::GetChecked();
	check(SodaSubsystem->LevelState);

	ChildSlot
	[
		SNew(SBox)
		.Padding(5)
		.MinDesiredWidth(600)
		.MinDesiredHeight(400)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.FillHeight(1.0)
			.Padding(0, 0, 0, 5)
			[
				SAssignNew(FileDatabaseManager, SFileDatabaseManager, EFileSlotType::Level, SodaSubsystem->LevelState->GetSlotGuid())
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Center)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.Padding(0, 0, 5, 0)
				[
					SNew(SBox)
					.MinDesiredWidth(100)
					[
						SAssignNew(SaveButton, SButton)
						.Text(FText::FromString("Save"))
						.HAlign(HAlign_Center)
						.OnClicked(this, &SLevelSaveLoadWindow::OnSave)
						.IsEnabled_Lambda([this](){ return FileDatabaseManager->ListView->GetNumItemsSelected() == 1; })
					]
				]
				+ SHorizontalBox::Slot()
				.Padding(0, 0, 5, 0)
				[
					SNew(SBox)
					.MinDesiredWidth(100)
					[
						SAssignNew(SaveButton, SButton)
						.Text(FText::FromString("New Slot"))
						.HAlign(HAlign_Center)
						.OnClicked(this, &SLevelSaveLoadWindow::OnNewSave)
					]
				]
				+ SHorizontalBox::Slot()
				[
					SNew(SBox)
					.MinDesiredWidth(100)
					[
						SAssignNew(LoadButton, SButton)
						.Text(FText::FromString("Load"))
						.HAlign(HAlign_Center)
						.OnClicked(this, &SLevelSaveLoadWindow::OnLoad)
						.IsEnabled_Lambda([this](){ return FileDatabaseManager->ListView->GetNumItemsSelected() == 1; })
					]
				]
			]
		]
	];
}


FReply SLevelSaveLoadWindow::OnSave()
{
	if (TSharedPtr<FFileDatabaseSlotInfo> Slot = FileDatabaseManager->GetSelectedSlot())
	{
		USodaSubsystem* SodaSubsystem = USodaSubsystem::GetChecked();
		ALevelState* LevelState = ALevelState::GetChecked();

		auto ProccSave = [Slot, Lable = FileDatabaseManager->GetLableText(), Description = FileDatabaseManager->GetDescriptionText()]()
		{
			if (!ALevelState::GetChecked()->SaveToSlot(Lable, Description, Slot->GUID))
			{
				soda::ShowNotification(ENotificationLevel::Error, 5.0, TEXT("Can't save to the selected slot"));
			}
		};

		if (!LevelState->GetSlotGuid().IsValid() || LevelState->GetSlotGuid() != Slot->GUID)
		{
			TSharedPtr<soda::SMessageBox> MsgBox = SodaSubsystem->ShowMessageBox(
				soda::EMessageBoxType::YES_NO_CANCEL,
				"Replace Slot",
				"Are you sure you want to replace the \"" + Slot->Lable + "\" with new slot data ?");
			MsgBox->SetOnMessageBox(soda::FOnMessageBox::CreateLambda([this, ProccSave](soda::EMessageBoxButton Button)
			{
				if (Button == soda::EMessageBoxButton::YES)
				{
					ProccSave();
				}
			}));
		}
		else
		{
			ProccSave();
		}
	}

	CloseWindow();
	return FReply::Handled();
}

FReply SLevelSaveLoadWindow::OnNewSave()
{
	if (!ALevelState::GetChecked()->SaveToSlot(FileDatabaseManager->GetLableText(), FileDatabaseManager->GetDescriptionText(), FGuid()))
	{
		soda::ShowNotification(ENotificationLevel::Error, 5.0, TEXT("Can't save level to a new slot"));
	}
	CloseWindow();
	return FReply::Handled();
}

FReply SLevelSaveLoadWindow::OnLoad()
{
	if (auto SelectedSlot = FileDatabaseManager->GetSelectedSlot())
	{
		USodaSubsystem::GetChecked()->LoadLevelFromSlot(SelectedSlot->GUID);
	}
	CloseWindow();
	return FReply::Handled();
}

} // namespace soda

