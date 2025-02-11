// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/UI/SFileDatabaseManager.h"
#include "Soda/FileDatabaseManager.h"
#include "Soda/SodaApp.h"
#include "Soda/UI/SMessageBox.h"
#include "Soda/SodaSubsystem.h"
#include "Soda/LevelState.h"
#include "SodaStyleSet.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SComboButton.h"
#include "GenericPlatform/GenericPlatformFile.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "RuntimeEditorUtils.h"

#define LOCTEXT_NAMESPACE "SFileDatabaseManager"

namespace soda
{
static const FName Column_DateTime("DateTime");
static const FName Column_Lable("Lable");
static const FName Column_Type("Type");
static const FName Column_Version("Version");
static const FName Column_OptButton("OptButton");

static const FName Name_LocalSource("Local");

static FString GetSlotTypeColumnName(EFileSlotType Type)
{
	switch (Type)
	{
	case EFileSlotType::Vehicle:
	case EFileSlotType::Actor:
	case EFileSlotType::VehicleComponent:
		return TEXT("Class");
	case EFileSlotType::Level:
	{
		return TEXT("Level");
	}
	default:
		return TEXT("");
	}
}

static FString GetSlotTypeColumnData(const TSharedPtr<FFileDatabaseSlotInfo> & Slot)
{
	switch (Slot->Type)
	{
	case EFileSlotType::Vehicle:
	case EFileSlotType::Actor:
	case EFileSlotType::VehicleComponent:
		return IsValid(Slot->DataClass) ? Slot->DataClass->GetName() : TEXT("");
	case EFileSlotType::Level:
	{
		FString LevelName;
		if (ALevelState::DeserializeSlotDescriptor(Slot->JsonDescription, LevelName))
		{
			return LevelName;
		}
		else
		{
			return TEXT("");
		}
	}
	default:
		return TEXT("");
	}
}

class SDeleteSlotWindow  : public SMenuWindowContent
{
public:
	SLATE_BEGIN_ARGS(SDeleteSlotWindow)
	{}
	SLATE_END_ARGS()


	virtual ~SDeleteSlotWindow()
	{
	}

	void Construct(const FArguments& InArgs, TSharedRef<FFileDatabaseSlotInfo> InSlot, TWeakPtr<SFileDatabaseManager> InParent)
	{
		Slot = InSlot;
		Parent = InParent;

		SMenuWindowContent::Construct(
			SMenuWindowContent::FArguments()
			.Content()
			[
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
							.Text(FText::FromString("Are you sure you want to delete the \"" + Slot->Lable + "\" slot?"))
						]
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					.HAlign(HAlign_Center)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot()
						.Padding(3)
						.AutoWidth()
						[
							SNew(SButton)
							.Text(FText::FromString("Local Only"))
							.OnClicked(this, &SDeleteSlotWindow::OnDeleteslote, true, false)
						]
						+ SHorizontalBox::Slot()
						.Padding(3)
						.AutoWidth()
						[
							SNew(SButton)
							.Text(FText::FromString("Remote Only"))
							.OnClicked(this, &SDeleteSlotWindow::OnDeleteslote, false, true)
							.Visibility((uint8)Slot->SourceVersion > (uint8)EFileSlotSourceVersion::LocalOnly ? EVisibility::Visible : EVisibility::Collapsed)
						]
						+ SHorizontalBox::Slot()
						.Padding(3)
						.AutoWidth()
						[
							SNew(SButton)
							.Text(FText::FromString("Local & Remote"))
							.OnClicked(this, &SDeleteSlotWindow::OnDeleteslote, true, true)
							.Visibility((uint8)Slot->SourceVersion > (uint8)EFileSlotSourceVersion::LocalOnly ? EVisibility::Visible : EVisibility::Collapsed)
						]
						+ SHorizontalBox::Slot()
						.Padding(3)
						.AutoWidth()
						[
							SNew(SButton)
							.Text(FText::FromString("Cancle"))
							.OnClicked_Lambda([this]()
							{
								CloseWindow();
								return FReply::Handled();
							})
						]
					]
				]
			]
		);
	}

	FReply OnDeleteslote( bool bLocal, bool bRemote)
	{
		bool bRes = true;

		if (bLocal)
		{
			if (!SodaApp.GetFileDatabaseManager().DeleteSlot(Slot->GUID))
			{
				soda::ShowNotification(ENotificationLevel::Error, 5.0, TEXT("Can't delete the local slot: \"%s\""), *Slot->Lable);
				bRes = false;
			}
		}

		if (bRemote)
		{
			auto Source = SodaApp.GetFileDatabaseManager().GetSources()[Parent.Pin()->GetSelectedSource()];
			if (!Source->DeleteSlot(Slot->GUID))
			{
				soda::ShowNotification(ENotificationLevel::Error, 5.0, TEXT("Can't delete the remote slot: \"%s\""), *Slot->Lable);
				bRes = false;
			}
		}

		if (bRes)
		{
			soda::ShowNotification(ENotificationLevel::Success, 5.0, TEXT("Slot is deleted: \"%s\""), *Slot->Lable);
		}

		Parent.Pin()->UpdateSlots();
		CloseWindow();
		return FReply::Handled();
	}

	TSharedPtr<FFileDatabaseSlotInfo> Slot;
	TWeakPtr<SFileDatabaseManager> Parent;
};

// -------------------------------------------------------------------------------------------------

class SSlotRow : public SMultiColumnTableRow<TSharedPtr<FFileDatabaseSlotInfo>>
{
public:
	SLATE_BEGIN_ARGS(SSlotRow)
	{}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTable, TSharedPtr<FFileDatabaseSlotInfo> InSlot, bool bInIsActiveSlot, TWeakPtr<SFileDatabaseManager> InParent)
	{
		Slot = InSlot;
		Parent = InParent;
		FSuperRowType::FArguments OutArgs;
		OutArgs.Padding(3);
		//OutArgs._SignalSelectionMode = ETableRowSignalSelectionMode::Instantaneous;
		bIsActiveSlot = bInIsActiveSlot;
		SMultiColumnTableRow<TSharedPtr<FFileDatabaseSlotInfo>>::Construct(OutArgs, InOwnerTable);

	}

	virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& InColumnName) override
	{
		if (InColumnName == Column_DateTime)
		{
			return SNew(STextBlock)
				.Text(FText::FromString(Slot->DateTime.ToString()))
				.ColorAndOpacity(bIsActiveSlot ? FLinearColor::Yellow : FLinearColor::White);
		}

		if (InColumnName == Column_Lable)
		{
			return SNew(STextBlock)
				.Text(FText::FromString(Slot->Lable))
				.ColorAndOpacity(bIsActiveSlot ? FLinearColor::Yellow : FLinearColor::White);
		}

		if (InColumnName == Column_Type)
		{
			return SNew(STextBlock)
				.Text(FText::FromString(GetSlotTypeColumnData(Slot)))
				.ColorAndOpacity(bIsActiveSlot ? FLinearColor::Yellow : FLinearColor::White);
		}

		if (InColumnName == Column_Version)
		{
			TSharedPtr<SImage> Image;
			switch (Slot->SourceVersion)
			{
			case EFileSlotSourceVersion::NotChecked:
				SAssignNew(Image, SImage)
					.Image(FSodaStyle::GetBrush("SodaSource.NotChecked"))
					.ToolTipText(LOCTEXT("SlotSourceVersion_NotChecked", "Not checked. The source has not been checked for this slot."));
				break;

			case EFileSlotSourceVersion::LocalOnly:
				SAssignNew(Image, SImage)
					.Image(FSodaStyle::GetBrush("SodaSource.LocalOnly"))
					.ToolTipText(LOCTEXT("SlotSourceVersion_LocalOnly", "Local only. This slot is only available locally."));
				break;


			case EFileSlotSourceVersion::RemoteOnly:
				SAssignNew(Image, SImage)
					.Image(FSodaStyle::GetBrush("SodaSource.RemoteOnly"))
					.ToolTipText(LOCTEXT("SlotSourceVersion_RemoteOnly", "Remote only. This slot is only available remotly."));
				break;


			case EFileSlotSourceVersion::LocalIsNewer:
				SAssignNew(Image, SImage)
					.Image(FSodaStyle::GetBrush("SodaSource.LocalIsNewer"))
					.ToolTipText(LOCTEXT("SlotSourceVersion_LocalIsNewer", "Local is newer. The local slot is newer than the remote one"));
				break;


			case EFileSlotSourceVersion::RemoteIsNewer:
				SAssignNew(Image, SImage)
					.Image(FSodaStyle::GetBrush("SodaSource.RemoteIsNewer"))
					.ToolTipText(LOCTEXT("SlotSourceVersion_RemoteIsNewer", "Remote is newer. The remote slot is newer than the local one"));
				break;


			case EFileSlotSourceVersion::Synchronized:
				SAssignNew(Image, SImage)
					.Image(FSodaStyle::GetBrush("SodaSource.Synchronized"))
					.ToolTipText(LOCTEXT("SlotSourceVersion_Synchronized", "Synchronized. Remote and local slots are synchronized"));
				break;
			default:
				check(0);
			}
		
			return SNew(SBox)
				.VAlign(VAlign_Center)
				.HAlign(HAlign_Center)
				[
					Image.ToSharedRef()
				];
		}
		
		if (InColumnName == Column_OptButton)
		{
			return SNew(SBox)
				.VAlign(VAlign_Center)
				.HAlign(HAlign_Center)
				[
					SNew(SComboButton)
						.ComboButtonStyle(FSodaStyle::Get(), "OptComboButton")
						.OnGetMenuContent(this, &SSlotRow::OnGetMenuContent)
						.ContentPadding(8)
				];

		}

		return SNullWidget::NullWidget;
	}

	TSharedRef<SWidget> OnGetMenuContent()
	{
		FUIAction DeleteAction;
		DeleteAction.CanExecuteAction.BindLambda([]()
		{
			return true;
		});
		DeleteAction.ExecuteAction.BindLambda([this]()
		{
			USodaSubsystem* SodaSubsystem = USodaSubsystem::GetChecked();
			SodaSubsystem->OpenWindow(TEXT("Delete Slot"), SNew(SDeleteSlotWindow, Slot.ToSharedRef(), Parent));
		});

		FUIAction PushAction;
		PushAction.CanExecuteAction.BindLambda([this]()
		{
			return !Parent.Pin()->GetSelectedSource().IsNone() && Slot->SourceVersion != EFileSlotSourceVersion::RemoteOnly;
		});
		PushAction.ExecuteAction.BindLambda([this]()
		{
			TArray<uint8> Data;
			if(!SodaApp.GetFileDatabaseManager().GetSlotData(Slot->GUID, Data))
			{
				soda::ShowNotification(ENotificationLevel::Error, 5.0, TEXT("Slot \"%s\" isn't find"), *Slot->Lable);
				return;
			}

			auto Source = SodaApp.GetFileDatabaseManager().GetSources()[Parent.Pin()->GetSelectedSource()];
			if (Source->PushSlot(*Slot, Data))
			{
				soda::ShowNotification(ENotificationLevel::Success, 5.0, TEXT("Push success. Slot: \"%s\""), *Slot->Lable);
			}
			else
			{
				soda::ShowNotification(ENotificationLevel::Error, 5.0, TEXT("Push faild. Slot: \"%s\""), *Slot->Lable);
			}
			Parent.Pin()->UpdateSlots();
		});

		FUIAction PullAction;
		PullAction.CanExecuteAction.BindLambda([this]()
		{
			return (uint8)Slot->SourceVersion > (uint8)EFileSlotSourceVersion::LocalOnly;
		});
		PullAction.ExecuteAction.BindLambda([this]()
		{
			auto Source = SodaApp.GetFileDatabaseManager().GetSources()[Parent.Pin()->GetSelectedSource()];

			TArray<uint8> Data;
			FFileDatabaseSlotInfo Info;
			if (Source->PullSlot(Slot->GUID, Info, Data))
			{
				SodaApp.GetFileDatabaseManager().AddOrUpdateSlotInfo(Info);
				SodaApp.GetFileDatabaseManager().UpdateSlotData(Info.GUID, Data);
				Parent.Pin()->UpdateSlots();
			}
		});

		FMenuBuilder MenuBuilder(true, NULL);
		MenuBuilder.BeginSection(NAME_None);
		MenuBuilder.AddMenuEntry(
			LOCTEXT("OptMenuDelete_Caption", "Delete"),
			LOCTEXT("OptMenuDelete_Tooltip", "Delete selected item"),
			FSlateIcon(FSodaStyle::Get().GetStyleSetName(), "Icons.Delete"),
			DeleteAction,
			NAME_None,
			EUserInterfaceActionType::Button);
		MenuBuilder.AddMenuEntry(
			LOCTEXT("OptMenuPush_Caption", "Push"),
			LOCTEXT("OptMenuPush_Tooltip", "Upload selected item to sever"),
			FSlateIcon(FSodaStyle::Get().GetStyleSetName(), "Icons.Export"),
			PushAction,
			NAME_None,
			EUserInterfaceActionType::Button);
		MenuBuilder.AddMenuEntry(
			LOCTEXT("OptMenuPull_Caption", "Pull"),
			LOCTEXT("OptMenuPull_Tooltip", "Download selected item from sever"),
			FSlateIcon(FSodaStyle::Get().GetStyleSetName(), "Icons.Import"),
			PullAction,
			NAME_None,
			EUserInterfaceActionType::Button);
		MenuBuilder.EndSection();
		return FRuntimeEditorUtils::MakeWidget_HackTooltip(MenuBuilder, nullptr, 1000);
	}

	TWeakPtr<SFileDatabaseManager> Parent;
	TSharedPtr<SInlineEditableTextBlock> InlineTextBlock;
	TSharedPtr<FFileDatabaseSlotInfo> Slot;
	bool bIsActiveSlot;
};

// -------------------------------------------------------------------------------------------------

void SFileDatabaseManager::Construct( const FArguments& InArgs, EFileSlotType InSlotType, const FGuid& InTargetGuid)
{
	TargetGuid = InTargetGuid;
	SlotType = InSlotType;
	
	ChildSlot
	[
		SNew(SBox)
		.MinDesiredWidth(600)
		.MinDesiredHeight(300)
		.Padding(5)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Right)
			//.Padding(0, 0, 0, 10)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.Padding(3)
				[
					SNew(SBox)
					.MinDesiredWidth(160)
					[
						SNew(SComboButton)
						.ContentPadding(3)
						.OnGetMenuContent(this, &SFileDatabaseManager::GetComboMenuContent)
						.ButtonContent()
						[
							SNew(STextBlock)
							.MinDesiredWidth(50)
							.Text_Lambda([this]()
							{
								return FText::FromName(Source == NAME_None ? Name_LocalSource : Source);
							})
						]
					]
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(3)
				[
					SNew(SButton)
					//.IsEnabled_Lambda([this]() { return Source != NAME_None; })
					.ToolTipText(FText::FromString("Refresh and synchronize with server"))
					.VAlign(VAlign_Center)
					.HAlign(HAlign_Center)
					[
						SNew(SImage)
						.Image(FSodaStyle::GetBrush("Icons.Reset"))
					]
					.OnClicked_Lambda([this]()
					{
						UpdateSlots();
						return FReply::Handled();
					})
				]
			]
			+ SVerticalBox::Slot()
			.FillHeight(1.0)
			.Padding(0, 0, 0, 5)
			[
				SNew(SBorder)
				.BorderImage(FSodaStyle::GetBrush("MenuWindow.Content"))
				.Padding(5.0f)
				[
					SAssignNew(ListView, SListView<TSharedPtr<FFileDatabaseSlotInfo>>)
					.ListItemsSource(&Slots)
					.SelectionMode(ESelectionMode::Single)
					.OnGenerateRow(this, &SFileDatabaseManager::OnGenerateRow)
					.OnSelectionChanged(this, &SFileDatabaseManager::OnSelectionChanged)
					.HeaderRow(
						SNew(SHeaderRow)
						+ SHeaderRow::Column(Column_DateTime)
						.DefaultLabel(FText::FromString("DateTime"))
						.FixedWidth(140)
						+ SHeaderRow::Column(Column_Lable)
						.DefaultLabel(FText::FromString("Lable"))
						.FillWidth(1)
						+ SHeaderRow::Column(Column_Type)
						.DefaultLabel(FText::FromString(GetSlotTypeColumnName(SlotType)))
						.FillWidth(1)
						+ SHeaderRow::Column(Column_Version)
						.FixedWidth(24)
						.DefaultLabel(FText())
						+ SHeaderRow::Column(Column_OptButton)
						.FixedWidth(24)
						.DefaultLabel(FText())
					)
				]
			]
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
					.Text(FText::FromString("Lable"))
				]
				+ SHorizontalBox::Slot()
				.FillWidth(1.0)
				.Padding(0, 0, 5, 0)
				[
					SAssignNew(LableTextBox, SEditableTextBox)
					//.IsEnabled(TargetGuid.IsValid())
				]
			]
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
					SAssignNew(DescriptionTextBox, SEditableTextBox)
					//.IsEnabled(TargetGuid.IsValid())
				]
			]
		]
	];

	UpdateSlots();
}

TSharedRef<ITableRow> SFileDatabaseManager::OnGenerateRow(TSharedPtr<FFileDatabaseSlotInfo> Slot, const TSharedRef< STableViewBase >& OwnerTable)
{
	return SNew(SSlotRow, OwnerTable, Slot, Slot->GUID == TargetGuid, SharedThis(this));
}

TSharedRef<SWidget> SFileDatabaseManager::GetComboMenuContent()
{
	FMenuBuilder MenuBuilder(true, nullptr, nullptr, false, &FSodaStyle::Get());

	MenuBuilder.BeginSection(NAME_None);

	{
		FUIAction Action;
		Action.ExecuteAction.BindLambda([this]()
			{
				Source = NAME_None;
			});
		MenuBuilder.AddMenuEntry(
			FText::FromName(Name_LocalSource),
			FText::FromString(TEXT("Local Only")),
			FSlateIcon(),
			Action);
	}

	for (auto& [Key, Value] : SodaApp.GetFileDatabaseManager().GetSources())
	{
		FUIAction Action;
		Action.ExecuteAction.BindLambda([this, Key]()
			{
				Source = Key;
			});
		MenuBuilder.AddMenuEntry(
			FText::FromName(Key),
			FText::FromName(Key),
			FSlateIcon(),
			Action);
	}

	MenuBuilder.EndSection();

	return FRuntimeEditorUtils::MakeWidget_HackTooltip(MenuBuilder, nullptr, 1000);
}

void SFileDatabaseManager::UpdateSlots()
{
	Slots.Empty();
	SodaApp.GetFileDatabaseManager().GetSlots(SlotType, true, Source).GenerateValueArray(Slots);

	bool bWasSelected = false;
	if (TargetGuid.IsValid())
	{
		for (auto& It : Slots)
		{
			if (It->GUID == TargetGuid)
			{
				ListView->SetSelection(It);
				bWasSelected = true;
				break;
			}
		}
	}

	if (!bWasSelected && Slots.Num())
	{
		ListView->SetSelection(Slots[0]);
	}

	ListView->RequestListRefresh();
}

void SFileDatabaseManager::OnSelectionChanged(TSharedPtr<FFileDatabaseSlotInfo> Slot, ESelectInfo::Type SelectInfo)
{
	if (Slot)
	{
		LableTextBox->SetText(FText::FromString(Slot->Lable));
		DescriptionTextBox->SetText(FText::FromString(Slot->Description));
		//SaveButton->SetEnabled(true);
		//LoadButton->SetEnabled(true);
	}
	else
	{
		LableTextBox->SetText(FText::GetEmpty());
		DescriptionTextBox->SetText(FText::GetEmpty());
		//SaveButton->SetEnabled(false);
		//LoadButton->SetEnabled(false);
	}
}
TSharedPtr<FFileDatabaseSlotInfo> SFileDatabaseManager::GetSelectedSlot()
{
	if (ListView->GetSelectedItems().Num() == 1)
	{
		return *ListView->GetSelectedItems().begin();
	}
	else return nullptr;
}

FString SFileDatabaseManager::GetLableText() const
{
	return LableTextBox->GetText().ToString();
}

FString SFileDatabaseManager::GetDescriptionText() const
{
	return DescriptionTextBox->GetText().ToString();
}

} // namespace soda


#undef LOCTEXT_NAMESPACE
