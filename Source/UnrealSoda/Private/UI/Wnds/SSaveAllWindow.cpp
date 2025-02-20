// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "UI/Wnds/SSaveAllWindow.h"
#include "UI/Wnds/SLevelSaveLoadWindow.h"
#include "UI/Wnds/SSlotActorManagerWindow.h"
#include "SodaStyleSet.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Soda/SodaSubsystem.h"
#include "Soda/LevelState.h"
#include "UObject/WeakInterfacePtr.h"
#include "EngineUtils.h"
#include "Soda/ISodaActor.h"
#include "Soda/Vehicles/SodaVehicle.h"
#include "Soda/SodaApp.h"


#define LOCTEXT_NAMESPACE "SSaveAllWindow"

namespace soda
{
class SSaveAllWindowRow;

static const FName Column_ItemName("ItemName");
static const FName Column_ClassName("ClassName");
static const FName Column_Status("Status");
static const FName Column_SaveButton("SaveButton");


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
		if (InColumnName == Column_ItemName)
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
					.Text_Lambda([this]() { return FText::FromString(Item.Pin()->Label.Get()); })
				];
		}

		if (InColumnName == Column_ClassName)
		{
			return
				SNew(STextBlock)
				.Text(FText::FromName(Item.Pin()->ClassName));
		}

		if (InColumnName == Column_Status)
		{
			return SNew(SBox)
				.VAlign(VAlign_Center)
				.HAlign(HAlign_Center)
				[
					SNew(SImage)
					.Image_Lambda([this]()
					{
						return Item.Pin()->bIsDirty.Get() 
							? FSodaStyle::GetBrush("Icons.WarningWithColor")
							: FSodaStyle::GetBrush("Icons.SuccessWithColor");
					})
					.ToolTipText_Lambda([this]()
					{
						return Item.Pin()->bIsDirty.Get() 
							? LOCTEXT("ItmemStatus_Dirty", "The item has been modified")
							: LOCTEXT("ItmemStatus_NotDirty", "The item has not been modified");
							
					})
				];
		}

		if (InColumnName == Column_SaveButton)
		{
			TSharedRef<SButton> Button = SNew(SButton)
			.ButtonStyle(FSodaStyle::Get(), "MenuWindow.SaveButton")
			.OnClicked_Lambda([this]()
				{
					Item.Pin()->SaveAction.ExecuteIfBound();
					return FReply::Handled();
				}
			);
			return SNew(SBox)
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

void SSaveAllWindow::Construct(const FArguments& InArgs, ESaveAllWindowMode Mode)
{
	USodaSubsystem* SodaSubsystem = USodaSubsystem::GetChecked();
	check(SodaSubsystem);
	UWorld * World = SodaSubsystem->GetWorld();
	check(World);

	FString LevelName = World->GetMapName();
	LevelName.RemoveFromStart(World->StreamingLevelsPrefix);

	bool bFoundDirtySlot = false;

	// Add Level State
	ALevelState * LevelState = ALevelState::GetChecked();
	TSharedPtr<FSaveAllWindowItem> LevelItem = MakeShared<FSaveAllWindowItem>();
	LevelItem->Label = TAttribute<FString>::CreateLambda([LevelName=LevelName, LevelState=TWeakObjectPtr<ALevelState>(LevelState)]()
	{
		return LevelName + " [" + (LevelState->GetSlotGuid().IsValid() ? LevelState->GetSlotLabel() : FString(TEXT("New"))) + "]";
	});
	LevelItem->IconName = "Icons.Level";
	LevelItem->ClassName = "Level";
	LevelItem->bIsDirty = TAttribute<bool>::CreateLambda([]() 
	{ 
		ALevelState * LevelState = ALevelState::GetChecked();
		return LevelState->IsDirty() || !LevelState->GetSlotGuid().IsValid();
	});
	LevelItem->SaveAction.BindLambda([]()
	{
		USodaSubsystem::GetChecked()->OpenWindow("Level Save & Load", SNew(SLevelSaveLoadWindow));	
	});
	LevelItem->ResaveAction.BindLambda([]()
	{
		if(ALevelState::GetChecked()->GetSlotGuid().IsValid())
		{
			ALevelState::GetChecked()->Resave();
		}
		else
		{
			soda::ShowNotification(ENotificationLevel::Error, 8.0, TEXT("The level was not saved. You must manually create a slot for the level "));
		}
	});
	bFoundDirtySlot = LevelItem->bIsDirty.Get();
	Source.Add(LevelItem);
	

	// Add Pinned Actors
	for (FActorIterator It(World); It; ++It)
	{
		AActor* Actor = *It;
		TWeakInterfacePtr<ISodaActor> SodaActor = Cast<ISodaActor>(Actor);
		if (SodaActor.IsValid())
		{
			if (SodaActor->GetSlotGuid().IsValid())
			{
				if(SodaActor->IsDirty())
				{
					bFoundDirtySlot = true;
				}
				TSharedPtr<FSaveAllWindowItem> PinnedItem = MakeShared<FSaveAllWindowItem>();
				Source.Add(PinnedItem);
				const FSodaActorDescriptor & Desc = SodaSubsystem->GetSodaActorDescriptor(Actor->GetClass());
				PinnedItem->Label = TAttribute<FString>::CreateLambda([SodaActor=SodaActor]()
				{
					return SodaActor.IsValid() ? SodaActor->GetSlotLabel() : TEXT("");
				});
				PinnedItem->IconName = Desc.Icon;
				PinnedItem->ClassName = Actor->GetClass()->GetFName();
				PinnedItem->bIsDirty = TAttribute<bool>::CreateLambda([SodaActor]()
				{ 
					return SodaActor.IsValid() && SodaActor->IsDirty();
				});
				PinnedItem->SaveAction.BindLambda([SodaActor, Actor]()
				{
					if (SodaActor.IsValid())
					{
						USodaSubsystem* SodaSubsystem = USodaSubsystem::GetChecked();
						bool bIsVehicle = Cast<ASodaVehicle>(Actor) ? true : false;
						SodaSubsystem->OpenWindow(
							FString::Printf(TEXT("Save Actor \"%s\""), *Actor->GetName()), 
							SNew(soda::SSlotActorManagerWindow, bIsVehicle ? soda::EFileSlotType::Vehicle : soda::EFileSlotType::Actor, Actor));
					}
				});
				PinnedItem->ResaveAction.BindLambda([SodaActor, SodaSubsystem]()
				{
					if (SodaActor.IsValid())
					{
						SodaActor->Resave();
					}
				});
			}
		}
	}

	if(!bFoundDirtySlot)
	{
		switch (Mode)
		{
		case ESaveAllWindowMode::Restart:
			SodaSubsystem->RequestRestartLevel(true);
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
			SodaSubsystem->RequestQuit(true);
			ChildSlot
			[
				SNew(SBox)
				.Padding(30.0f)
				.VAlign(VAlign_Center)
				.HAlign(HAlign_Center)
				[
					SNew(STextBlock)
					.Text(FText::FromString("Good bye!"))
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
						+ SHeaderRow::Column(Column_ItemName)
						.DefaultLabel(FText::FromString("Item Name"))
						+ SHeaderRow::Column(Column_ClassName)
						.DefaultLabel(FText::FromString("Item Class"))
						+ SHeaderRow::Column(Column_Status)
						.FixedWidth(24)
						.DefaultLabel(FText::FromString(""))
						+ SHeaderRow::Column(Column_SaveButton)
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
					.Text(FText::FromString("SaveAll"))
					.OnClicked(this, &SSaveAllWindow::OnSaveAll)
				]
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
					.Text(FText::FromString("Close"))
					.OnClicked(this, &SSaveAllWindow::OnCancel)
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
	if (USodaSubsystem* SodaSubsystem = USodaSubsystem::Get())
	{
		SodaSubsystem->RequestQuit(true);
	}
	return FReply::Handled();
}

FReply SSaveAllWindow::OnRestart()
{
	if (USodaSubsystem* SodaSubsystem = USodaSubsystem::Get())
	{
		SodaSubsystem->RequestRestartLevel(true);
	}
	return FReply::Handled();
}

FReply SSaveAllWindow::OnCancel()
{
	CloseWindow();
	return FReply::Handled();
}

FReply SSaveAllWindow::OnSaveAll()
{
	for (auto& It : Source)
	{
		It->ResaveAction.Execute();
	}

	return FReply::Handled();
}

} // namespace soda


#undef LOCTEXT_NAMESPACE
