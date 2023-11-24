// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "UI/Wnds/SSaveAllWindow.h"
#include "UI/Wnds/SLevelSaveLoadWindow.h"
#include "SodaStyleSet.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Soda/SodaGameMode.h"
#include "Soda/LevelState.h"
#include "UObject/WeakInterfacePtr.h"
#include "EngineUtils.h"
#include "Soda/ISodaActor.h"
#include "SodaStyleSet.h"

namespace soda
{
class SSaveAllWindowRow;

static const FName Column_SG_ItemName("ItemName");
static const FName Column_SG_ClassName("ClassName");
static const FName Column_SG_SaveButton("SaveButton");


class SSaveAllWindowRow : public SMultiColumnTableRow<TSharedPtr<FSaveAllWindowItem>>
{
public:
	SLATE_BEGIN_ARGS(SSaveAllWindowRow)
	{}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTable, TWeakPtr<FSaveAllWindowItem> InItem)
	{
		Item = InItem;
		FSuperRowType::FArguments OutArgs;
		//OutArgs._SignalSelectionMode = ETableRowSignalSelectionMode::Instantaneous;
		SMultiColumnTableRow<TSharedPtr<FSaveAllWindowItem>>::Construct(OutArgs, InOwnerTable);
	}

	virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& InColumnName) override
	{
		if (InColumnName == Column_SG_ItemName)
		{
			return
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(5, 1, 1, 1)
				.VAlign(VAlign_Center)
				.HAlign(HAlign_Center)
				[
					SNew(SImage)
					.Image(FSodaStyle::GetBrush(Item.Pin()->IconName))
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(5, 1, 1, 1)
				[
					SNew(STextBlock)
					.Text(FText::FromString(Item.Pin()->Caption))
				];
		}

		if (InColumnName == Column_SG_ClassName)
		{
			return
				SNew(STextBlock)
				.Text(FText::FromName(Item.Pin()->ClassName));
		}

		if (InColumnName == Column_SG_SaveButton)
		{
			TSharedRef<SButton> Button = SNew(SButton)
			.ButtonStyle(FSodaStyle::Get(), "MenuWindow.SaveButton")
			.OnClicked_Lambda([this]()
				{
					Item.Pin()->Action.Execute();
					return FReply::Handled();
				}
			);

			Button->SetEnabled(TAttribute<bool>::CreateLambda([this]()
				{
					return Item.Pin()->Action.CanExecute();
				}
			));
			return
				SNew(SBox)
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				[
					Button
				];
		}

		return SNullWidget::NullWidget;
	}

	TWeakPtr<FSaveAllWindowItem> Item;
};

/***********************************************************************************/

void SSaveAllWindow::Construct(const FArguments& InArgs, ESaveAllWindowMode Mode, bool bExecuteModeIfNothingToSave)
{
	USodaGameModeComponent* GameMode = USodaGameModeComponent::Get();
	check(GameMode);
	UWorld * World = GameMode->GetWorld();
	check(World);

	// Add Level State
	TWeakObjectPtr<ALevelState> LevelState = GameMode->LevelState;
	if(LevelState.IsValid() && LevelState->IsDirty())
	{
		TSharedPtr<FSaveAllWindowItem> LevelItem = MakeShared<FSaveAllWindowItem>();
		Source.Add(LevelItem);
		LevelItem->Caption = World->GetMapName() + " [" + ((LevelState->Slot.SlotSource == ELeveSlotSource::NoSlot) ? LevelState->Slot.Description : FString("New")) + "]";
		LevelItem->Caption.RemoveFromStart(World->StreamingLevelsPrefix);
		LevelItem->IconName = "Icons.Level";
		LevelItem->ClassName = "Level";
		LevelItem->Action.CanExecuteAction.BindLambda([LevelState]()
		{
			if (LevelState.IsValid())
			{
				return LevelState->IsDirty();
			}
			return false;
		});
		LevelItem->Action.ExecuteAction.BindLambda([LevelState, GameMode]()
		{
			if (LevelState.IsValid())
			{
				if (LevelState->Slot.SlotSource == ELeveSlotSource::Local)
				{
					LevelState->SaveLevelLocallyAs(-1, LevelState->Slot.Description);
				}
				else
				{
					GameMode->OpenWindow("Level Save & Load", SNew(SLevelSaveLoadWindow));
				}
			}
		});
	}

	// Add Pinned Actors
	for (FActorIterator It(World); It; ++It)
	{
		AActor* Actor = *It;
		TWeakInterfacePtr<ISodaActor> SodaActor = Cast<ISodaActor>(Actor);
		if (SodaActor.IsValid())
		{
			if (SodaActor->IsPinnedActor() && SodaActor->IsDirty())
			{
				TSharedPtr<FSaveAllWindowItem> PinnedItem = MakeShared<FSaveAllWindowItem>();
				Source.Add(PinnedItem);
				const FSodaActorDescriptor & Desc = GameMode->GetSodaActorDescriptor(Actor->GetClass());
				PinnedItem->Caption = SodaActor->GetPinnedActorName();
				PinnedItem->IconName = Desc.Icon;
				PinnedItem->ClassName = Actor->GetClass()->GetFName();
				PinnedItem->Action.CanExecuteAction.BindLambda([SodaActor]()
				{
					if (SodaActor.IsValid())
					{
						return SodaActor->IsDirty();
					}
					return false;
				});
				PinnedItem->Action.ExecuteAction.BindLambda([SodaActor, GameMode]()
				{
					if (SodaActor.IsValid())
					{
						SodaActor->SavePinnedActor();
					}
				});
			}
		}
	}

	if (bExecuteModeIfNothingToSave && Source.Num() == 0)
	{
		switch (Mode)
		{
		case ESaveAllWindowMode::Restart:
			GameMode->RequestRestartLevel(true);
			ChildSlot
			[
				SNew(SBox)
				.Padding(30.0f)
				.VAlign(VAlign_Center)
				.HAlign(HAlign_Center)
				[
					SNew(STextBlock)
					.Text(FText::FromString("Restarting..."))
				]
			];
			return;
		case ESaveAllWindowMode::Quit:
			GameMode->RequestQuit(true);
			ChildSlot
			[
				SNew(SBox)
				.Padding(30.0f)
				.VAlign(VAlign_Center)
				.HAlign(HAlign_Center)
				[
					SNew(STextBlock)
					.Text(FText::FromString("Good bay!"))
				]
			];
			return;
		}
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
			.FillHeight(1)
			.Padding(0, 0, 0, 5)
			[
				SNew(SBorder)
				.BorderImage(FSodaStyle::GetBrush(TEXT("Brushes.Recessed")))
				.Padding(1.0f)
				[
					SNew(SListView<TSharedPtr<FSaveAllWindowItem>>)
					.ListItemsSource(&Source)
					.SelectionMode(ESelectionMode::Single)
					.OnGenerateRow(this, &SSaveAllWindow::OnGenerateRow)
					.HeaderRow(
						SNew(SHeaderRow)
						+ SHeaderRow::Column(Column_SG_ItemName)
						.DefaultLabel(FText::FromString("Item Name"))
						+ SHeaderRow::Column(Column_SG_ClassName)
						.DefaultLabel(FText::FromString("Item Class"))
						+ SHeaderRow::Column(Column_SG_SaveButton)
						.FixedWidth(24)
						.DefaultLabel(FText::FromString(""))
					)
				]
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Center)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.Padding(3)
				.AutoWidth()
				[
					SNew(SButton)
					.Text(FText::FromString("Restart"))
					.Visibility(Mode == ESaveAllWindowMode::Restart ? EVisibility::Visible : EVisibility::Collapsed)
					.OnClicked(this, &SSaveAllWindow::OnRestart)
				]
				+ SHorizontalBox::Slot()
				.Padding(3)
				.AutoWidth()
				[
					SNew(SButton)
					.Text(FText::FromString("Quit"))
					.Visibility(Mode == ESaveAllWindowMode::Quit ? EVisibility::Visible : EVisibility::Collapsed)
					.OnClicked(this, &SSaveAllWindow::OnQuit)
				]
				+ SHorizontalBox::Slot()
				.Padding(3)
				.AutoWidth()
				[
					SNew(SButton)
					.Text(FText::FromString("Cancle"))
					.OnClicked(this, &SSaveAllWindow::OnCancle)
				]
			]
		]
	];
}

TSharedRef<ITableRow> SSaveAllWindow::OnGenerateRow(TSharedPtr<FSaveAllWindowItem> Item, const TSharedRef< STableViewBase >& OwnerTable)
{
	return SNew(SSaveAllWindowRow, OwnerTable, Item);
}

FReply SSaveAllWindow::OnQuit()
{
	if (USodaGameModeComponent* GameMode = USodaGameModeComponent::Get())
	{
		GameMode->RequestQuit(true);
	}
	return FReply::Handled();
}

FReply SSaveAllWindow::OnRestart()
{
	if (USodaGameModeComponent* GameMode = USodaGameModeComponent::Get())
	{
		GameMode->RequestRestartLevel(true);
	}
	return FReply::Handled();
}

FReply SSaveAllWindow::OnCancle()
{
	CloseWindow();
	return FReply::Handled();
}

} // namespace soda

