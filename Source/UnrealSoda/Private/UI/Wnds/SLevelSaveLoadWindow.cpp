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
#include "Soda/SodaSubsystem.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "RuntimeEditorUtils.h"

namespace soda
{

static const FName Column_DateTime("DateTime");
static const FName Column_Description("Description");
static const FName Column_DeleteButton("DeleteButton");
//static const FName Column_Startup("Startup");

class SLevelSaveLoadRow : public SMultiColumnTableRow<TSharedPtr<FLevelStateSlotDescription>>
{
public:
	SLATE_BEGIN_ARGS(SLevelSaveLoadRow)
		{}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTable, TSharedPtr<FLevelStateSlotDescription> InSlot, TWeakPtr<SLevelSaveLoadWindow> InParent)
	{
		Slot = InSlot;
		Parent = InParent;
		FSuperRowType::FArguments OutArgs;
		OutArgs.Padding(3);
		//OutArgs._SignalSelectionMode = ETableRowSignalSelectionMode::Instantaneous;
		SMultiColumnTableRow<TSharedPtr<FLevelStateSlotDescription>>::Construct(OutArgs, InOwnerTable);
	}

	virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& InColumnName) override
	{
		if (InColumnName == Column_DateTime)
		{
			return SNew(STextBlock)
				.Text(FText::FromString(Slot->DateTime.ToString()))
				.ColorAndOpacity(Slot == Parent.Pin()->GetCurrentSlot() ? FLinearColor::Yellow : FLinearColor::White);
		}

		if (InColumnName == Column_Description)
		{
			return SNew(STextBlock)
				.Text(FText::FromString(Slot->Description))
				.ColorAndOpacity(Slot == Parent.Pin()->GetCurrentSlot() ? FLinearColor::Yellow : FLinearColor::White);
		}

		if (InColumnName == Column_DeleteButton)
		{
			return SNew(SBox)
				.VAlign(VAlign_Center)
				.HAlign(HAlign_Center)
				[
					SNew(SButton)
					.ButtonStyle(FSodaStyle::Get(), "MenuWindow.DeleteButton")
					.OnClicked(this, &SLevelSaveLoadRow::OnDeleteSlot, Slot)
				];
		}
		/*
		if (InColumnName == Column_Startup)
		{
			return SNew(SCheckBox)
				.IsChecked_Lambda([this]() { return Slot->bUseLikeStartup ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
				.OnCheckStateChanged(this, &SLevelSaveLoadRow::OnStartupChange);
		}
		*/

		return SNullWidget::NullWidget;
	}

	FReply OnDeleteSlot(TSharedPtr<FLevelStateSlotDescription> SelectedItem)
	{
		if (USodaSubsystem* SodaSubsystem = USodaSubsystem::Get())
		{
			TSharedPtr<soda::SMessageBox> MsgBox = SodaSubsystem->ShowMessageBox(
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
					Parent.Pin()->UpdateSlots();
				}
			}));
		}
		return FReply::Handled();
	}

	void OnStartupChange(ECheckBoxState NewState)
	{

	}

	TWeakObjectPtr<AActor> Actor;
	FSodaActorDescriptor Desc;
	bool IsSpawnedActor;
	TWeakPtr<SLevelSaveLoadWindow> Parent;
	TSharedPtr<SInlineEditableTextBlock> InlineTextBlock;

	TSharedPtr<FLevelStateSlotDescription> Slot;
};



void SLevelSaveLoadWindow::Construct( const FArguments& InArgs )
{
	USodaSubsystem* SodaSubsystem = USodaSubsystem::Get();
	if (!SodaSubsystem || !SodaSubsystem->LevelState)
	{
		return;
	}

	/*
	if (FDBGateway::Instance().IsConnected() && SodaSubsystem->LevelState->Slot.SlotSource == ELeveSlotSource::Remote)
	{
		SetMode(ELevelSaveLoadWindowMode::Remote);
	}
	*/

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
					.HeaderRow(
						SNew(SHeaderRow)
						+ SHeaderRow::Column(Column_DateTime)
						.DefaultLabel(FText::FromString("DateTime"))
						.FixedWidth(140)
						+ SHeaderRow::Column(Column_Description)
						.DefaultLabel(FText::FromString("Description"))
						.FillWidth(1)
						//+ SHeaderRow::Column(Column_Startup)
						//.FixedWidth(24)
						//.DefaultLabel(FText::FromString(""))
						+ SHeaderRow::Column(Column_DeleteButton)
						.FixedWidth(24)
						.DefaultLabel(FText::FromString(""))
					)
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
					//return FDBGateway::Instance().IsConnected() ? EVisibility::Visible : EVisibility::Hidden;
					return EVisibility::Hidden;
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
	USodaSubsystem* SodaSubsystem = USodaSubsystem::Get();
	if (!SodaSubsystem || !SodaSubsystem->LevelState)
	{
		return;
	}

	Source.Empty();
	CurrentSlot.Reset();

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
			USodaSubsystem::GetChecked()->ShowMessageBox(soda::EMessageBoxType::OK, "Error", "MongoDB error. See log for more information");
		}
	}

	for (auto& Slot : Slots)
	{
		Source.Add(MakeShared<FLevelStateSlotDescription>(Slot));
	}


	for (auto& It : Source)
	{
		if ((It->SlotSource == SodaSubsystem->LevelState->Slot.SlotSource) && (
				(It->SlotSource == ELeveSlotSource::Local && It->SlotIndex == SodaSubsystem->LevelState->Slot.SlotIndex) ||
				(It->SlotSource == ELeveSlotSource::Remote && It->ScenarioID == SodaSubsystem->LevelState->Slot.ScenarioID)))
		{
			CurrentSlot = It;
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

	return FRuntimeEditorUtils::MakeWidget_HackTooltip(MenuBuilder, nullptr, 1000);;
}

TSharedRef<ITableRow> SLevelSaveLoadWindow::OnGenerateRow(TSharedPtr<FLevelStateSlotDescription> Slot, const TSharedRef< STableViewBase >& OwnerTable)
{
	return SNew(SLevelSaveLoadRow, OwnerTable, Slot, SharedThis(this));
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
	if (USodaSubsystem* SodaSubsystem = USodaSubsystem::Get())
	{
		TSharedPtr<FLevelStateSlotDescription> SelectedItem;
		if (ListView->GetSelectedItems().Num() == 1) SelectedItem = *ListView->GetSelectedItems().begin();
		if (SelectedItem)
		{
			if (SelectedItem->SlotSource == ELeveSlotSource::Remote)
			{
				check(SelectedItem->ScenarioID >= 0);
				SodaSubsystem->LevelState->SaveLevelRemotlyAs(SelectedItem->ScenarioID, EditableTextBoxDesc->GetText().ToString());
			}
			else
			{
				check(SelectedItem->SlotIndex >= 0);
				SodaSubsystem->LevelState->SaveLevelLocallyAs(SelectedItem->SlotIndex, EditableTextBoxDesc->GetText().ToString());
			}
			this->UpdateSlots();
		}
	}
	CloseWindow();
	return FReply::Handled();
}

FReply SLevelSaveLoadWindow::OnNewSave()
{
	if (USodaSubsystem* SodaSubsystem = USodaSubsystem::Get())
	{
		if (GetMode() == ELevelSaveLoadWindowMode::Local)
		{
			SodaSubsystem->LevelState->SaveLevelLocallyAs(-1, EditableTextBoxDesc->GetText().ToString());
		}
		else
		{
			SodaSubsystem->LevelState->SaveLevelRemotlyAs(-1, EditableTextBoxDesc->GetText().ToString());
		}
		this->UpdateSlots();
	}
	CloseWindow();
	return FReply::Handled();
}

FReply SLevelSaveLoadWindow::OnLoad()
{
	if (USodaSubsystem* SodaSubsystem = USodaSubsystem::Get())
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



} // namespace soda

