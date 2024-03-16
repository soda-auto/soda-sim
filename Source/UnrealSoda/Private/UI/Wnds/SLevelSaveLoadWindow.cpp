// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "UI/Wnds/SLevelSaveLoadWindow.h"
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
#include "Soda/SodaGameMode.h"
#include "Soda/DBGateway.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"

namespace soda
{

void SLevelSaveLoadWindow::Construct( const FArguments& InArgs )
{
	USodaGameModeComponent* GameMode = USodaGameModeComponent::Get();
	if (!GameMode || !GameMode->LevelState)
	{
		return;
	}

	if (FDBGateway::Instance().IsConnected() && GameMode->LevelState->Slot.SlotSource == ELeveSlotSource::Remote)
	{
		SetMode(ELevelSaveLoadWindowMode::Remote);
	}

	ChildSlot
	[
		SNew(SBox)
		.Padding(5)
		.MinDesiredWidth(600)
		.MinDesiredHeight(400)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0, 0, 0, 5)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.VAlign(VAlign_Center)
				.HAlign(HAlign_Center)
				.Padding(0, 0, 5, 0)
				.AutoWidth()
				[
					SNew(STextBlock)
					.Text(FText::FromString("Description"))
				]
				+ SHorizontalBox::Slot()
				.FillWidth(1.0)
				.Padding(0, 0, 5, 0)
				[
					SAssignNew(EditableTextBoxDesc, SEditableTextBox)
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SButton)
					.Text(FText::FromString("New Save"))
					.OnClicked(this, &SLevelSaveLoadWindow::OnNewSave)
				]
			]
			+ SVerticalBox::Slot()
			.FillHeight(1)
			.Padding(0, 0, 0, 5)
			[
				SNew(SBorder)
				.BorderImage(FSodaStyle::GetBrush("MenuWindow.Content"))
				.Padding(5.0f)
				[
					SAssignNew(ListView, SListView<TSharedPtr<FLevelStateSlotDescription>>)
					.ListItemsSource(&Source)
					.SelectionMode(ESelectionMode::Single)
					.OnGenerateRow(this, &SLevelSaveLoadWindow::OnGenerateRow)
					.OnSelectionChanged(this, &SLevelSaveLoadWindow::OnSelectionChanged)
				]
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Right)
			.Padding(0, 0, 0, 5)
			[
				SNew(SComboButton)
				.ContentPadding(3)
				.OnGetMenuContent(this, &SLevelSaveLoadWindow::GetComboMenuContent)
				.Visibility_Lambda([]()
				{
					return FDBGateway::Instance().IsConnected() ? EVisibility::Visible : EVisibility::Hidden;
				})
				.ButtonContent()
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.VAlign(VAlign_Center)
					.HAlign(HAlign_Center)
					.Padding(0, 0, 5, 0)
					.AutoWidth()
					[
						SNew(SImage)
						.Image_Lambda([this]()
						{
							if(this->GetMode() == ELevelSaveLoadWindowMode::Local)
							{
								return FSodaStyle::Get().GetBrush("Icons.FolderClosed");
							}
							else
							{
								return FSodaStyle::Get().GetBrush("Icons.Server");
							}
							
						})
					]
					+ SHorizontalBox::Slot()
					.VAlign(VAlign_Center)
					.HAlign(HAlign_Center)
					.Padding(0, 0, 5, 0)
					[
						SNew(STextBlock)
						.MinDesiredWidth(50)
						.Text_Lambda([this]()
						{
							if(this->GetMode() == ELevelSaveLoadWindowMode::Local)
							{
								return FText::FromString("Local");
							}
							else
							{
								return FText::FromString("Remote");
							}
						})
					]
				]
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
					SAssignNew(SaveButton, SButton)
					.Text(FText::FromString("Save"))
					.OnClicked(this, &SLevelSaveLoadWindow::OnSave)
				]
				+ SHorizontalBox::Slot()
				[
					SAssignNew(LoadButton, SButton)
					.Text(FText::FromString("Load"))
					.OnClicked(this, &SLevelSaveLoadWindow::OnLoad)
				]
			]
		]
	];

	UpdateSlots();
}

void SLevelSaveLoadWindow::UpdateSlots()
{
	USodaGameModeComponent* GameMode = USodaGameModeComponent::Get();
	if (!GameMode || !GameMode->LevelState)
	{
		return;
	}

	Source.Empty();

	SaveButton->SetEnabled(false);
	LoadButton->SetEnabled(false);

	TArray<FLevelStateSlotDescription> Slots;

	if (GetMode() == ELevelSaveLoadWindowMode::Local)
	{
		ALevelState::GetLevelSlotsLocally(nullptr, Slots, true);
	}
	else
	{
		if (!ALevelState::GetLevelSlotsRemotly(nullptr, Slots, true))
		{
			USodaGameModeComponent::GetChecked()->ShowMessageBox(soda::EMessageBoxType::OK, "Error", "MongoDB error. See log for more information");
		}
	}

	for (auto& Slot : Slots)
	{
		Source.Add(MakeShared<FLevelStateSlotDescription>(Slot));
	}

	for (auto& It : Source)
	{
		if ((It->SlotSource == GameMode->LevelState->Slot.SlotSource) && (
				(It->SlotSource == ELeveSlotSource::Local && It->SlotIndex == GameMode->LevelState->Slot.SlotIndex) ||
				(It->SlotSource == ELeveSlotSource::Remote && It->ScenarioID == GameMode->LevelState->Slot.ScenarioID)))
		{
			ListView->SetSelection(It);
			break;
		}
	}
	
	ListView->RequestListRefresh();
}

TSharedRef<SWidget> SLevelSaveLoadWindow::GetComboMenuContent()
{
	FMenuBuilder MenuBuilder(
		/*bShouldCloseWindowAfterMenuSelection*/ true, 
		/* InCommandList = */nullptr, 
		/* InExtender = */nullptr, 
		/*bCloseSelfOnly*/ false,
		&FSodaStyle::Get());

	MenuBuilder.BeginSection("Section");

	{
		FUIAction Action;
		Action.ExecuteAction.BindLambda([this]() 
		{
			SetMode(ELevelSaveLoadWindowMode::Remote);
			UpdateSlots();
		});
		MenuBuilder.AddMenuEntry(
			FText::FromString(TEXT("Remote")),
			FText::FromString(TEXT("Remote")),
			FSlateIcon(FSodaStyle::Get().GetStyleSetName(), "Icons.Server"),
			Action);
	}

	{
		FUIAction Action;
		Action.ExecuteAction.BindLambda([this]() 
		{
			SetMode(ELevelSaveLoadWindowMode::Local);
			UpdateSlots();
		});
		MenuBuilder.AddMenuEntry(
			FText::FromString(TEXT("Local")),
			FText::FromString(TEXT("Local")),
			FSlateIcon(FSodaStyle::Get().GetStyleSetName(), "Icons.FolderClosed"),
			Action);
	}

	MenuBuilder.EndSection();

	return MenuBuilder.MakeWidget();
}

TSharedRef<ITableRow> SLevelSaveLoadWindow::OnGenerateRow(TSharedPtr<FLevelStateSlotDescription> Slot, const TSharedRef< STableViewBase >& OwnerTable)
{
	return 
		SNew(STableRow<TSharedPtr<FLevelStateSlotDescription>>, OwnerTable)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.Padding(FMargin(0, 0, 5, 0))
			.AutoWidth()
			[
				SNew(STextBlock)
				.Text(FText::FromString(Slot->DateTime.ToString()))
			]
			+ SHorizontalBox::Slot()
			.FillWidth(1.0)
			[
				SNew(STextBlock)
				.Text(FText::FromString(Slot->Description))
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SButton)
				.ButtonStyle(FSodaStyle::Get(), "MenuWindow.DeleteButton")
				.OnClicked(this, &SLevelSaveLoadWindow::OnDeleteSlot, Slot)
			]
		];
}

void SLevelSaveLoadWindow::OnSelectionChanged(TSharedPtr<FLevelStateSlotDescription> Slot, ESelectInfo::Type SelectInfo)
{
	if (Slot)
	{
		EditableTextBoxDesc->SetText(FText::FromString(Slot->Description));
		SaveButton->SetEnabled(true);
		LoadButton->SetEnabled(true);
	}
	else
	{
		EditableTextBoxDesc->SetText(FText::GetEmpty());
		SaveButton->SetEnabled(false);
		LoadButton->SetEnabled(false);
	}
}

FReply SLevelSaveLoadWindow::OnSave()
{
	if (USodaGameModeComponent* GameMode = USodaGameModeComponent::Get())
	{
		TSharedPtr<FLevelStateSlotDescription> SelectedItem;
		if (ListView->GetSelectedItems().Num() == 1) SelectedItem = *ListView->GetSelectedItems().begin();
		if (SelectedItem)
		{
			if (SelectedItem->SlotSource == ELeveSlotSource::Remote)
			{
				check(SelectedItem->ScenarioID >= 0);
				GameMode->LevelState->SaveLevelRemotlyAs(SelectedItem->ScenarioID, EditableTextBoxDesc->GetText().ToString());
			}
			else
			{
				check(SelectedItem->SlotIndex >= 0);
				GameMode->LevelState->SaveLevelLocallyAs(SelectedItem->SlotIndex, EditableTextBoxDesc->GetText().ToString());
			}
			this->UpdateSlots();
		}
	}
	CloseWindow();
	return FReply::Handled();
}

FReply SLevelSaveLoadWindow::OnNewSave()
{
	if (USodaGameModeComponent* GameMode = USodaGameModeComponent::Get())
	{
		if (GetMode() == ELevelSaveLoadWindowMode::Local)
		{
			GameMode->LevelState->SaveLevelLocallyAs(-1, EditableTextBoxDesc->GetText().ToString());
		}
		else
		{
			GameMode->LevelState->SaveLevelRemotlyAs(-1, EditableTextBoxDesc->GetText().ToString());
		}
		this->UpdateSlots();
	}
	CloseWindow();
	return FReply::Handled();
}

FReply SLevelSaveLoadWindow::OnLoad()
{
	if (USodaGameModeComponent* GameMode = USodaGameModeComponent::Get())
	{
		TSharedPtr<FLevelStateSlotDescription> SelectedItem;
		if (ListView->GetSelectedItems().Num() == 1) SelectedItem = *ListView->GetSelectedItems().begin();
		if (SelectedItem)
		{
			if (SelectedItem->SlotSource == ELeveSlotSource::Remote)
			{
				check(SelectedItem->ScenarioID >= 0);
				ALevelState::ReloadLevelFromSlotRemotly(nullptr, SelectedItem->ScenarioID);
			}
			else
			{
				check(SelectedItem->SlotIndex >= 0);
				ALevelState::ReloadLevelFromSlotLocally(nullptr, SelectedItem->SlotIndex);
			}
		}
	}
	CloseWindow();
	return FReply::Handled();
}

FReply SLevelSaveLoadWindow::OnDeleteSlot(TSharedPtr<FLevelStateSlotDescription> SelectedItem)
{
	if (USodaGameModeComponent* GameMode = USodaGameModeComponent::Get())
	{
		TSharedPtr<soda::SMessageBox> MsgBox = GameMode->ShowMessageBox(
			soda::EMessageBoxType::YES_NO_CANCEL,
			"Delete Slot",
			"Are you sure you want to delete the \"" + SelectedItem->Description + "\" slot?");
		MsgBox->SetOnMessageBox(soda::FOnMessageBox::CreateLambda([this, SelectedItem](soda::EMessageBoxButton Button)
		{
			if (Button == soda::EMessageBoxButton::YES)
			{
				if (SelectedItem->SlotSource == ELeveSlotSource::Remote)
				{
					check(SelectedItem->ScenarioID >= 0);
					ALevelState::DeleteLevelRemotly(nullptr, SelectedItem->ScenarioID);
				}
				else
				{
					check(SelectedItem->SlotIndex >= 0);
					ALevelState::DeleteLevelLocally(nullptr, SelectedItem->SlotIndex);
				}
				this->UpdateSlots();
			}
		}));
	}
	return FReply::Handled();
}

} // namespace soda

