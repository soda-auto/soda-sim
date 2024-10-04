// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "UI/Wnds/SPinToolActorWindow.h"
#include "SodaStyleSet.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Soda/UI/SMessageBox.h"
#include "Soda/SodaSubsystem.h"
#include "Soda/LevelState.h"
#include "Soda/SodaActorFactory.h"

namespace soda
{

void SPinToolActorWindow::Construct( const FArguments& InArgs, IToolActor* InToolActor)
{
	ToolActor = InToolActor;
	check(ToolActor.IsValid());

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
					.Text(FText::FromString("Name"))
				]
				+ SHorizontalBox::Slot()
				.FillWidth(1.0)
				.Padding(0, 0, 5, 0)
				[
					SAssignNew(EditableTextBox, SEditableTextBox)
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SButton)
					.Text(FText::FromString("New Slot"))
					.OnClicked(this, &SPinToolActorWindow::OnNewSave)
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
					SAssignNew(ListView, SListView<TSharedPtr<FString>>)
					.ListItemsSource(&Source)
					.SelectionMode(ESelectionMode::Single)
					.OnGenerateRow(this, &SPinToolActorWindow::OnGenerateRow)
					.OnMouseButtonDoubleClick(this, &SPinToolActorWindow::OnDoubleClickRow)
				]
			]
		]
	];

	UpdateSlots();
}

void SPinToolActorWindow::UpdateSlots()
{
	USodaSubsystem* SodaSubsystem = USodaSubsystem::Get();
	if (!SodaSubsystem || !SodaSubsystem->LevelState)
	{
		return;
	}

	Source.Empty();

	if (ToolActor.IsValid())
	{
		if (UPinnedToolActorsSaveGame* SaveGame = ToolActor->GetSaveGame())
		{
			for (auto& It : SaveGame->ActorRecords)
			{
				Source.Add(MakeShared<FString>(It.Key));
			}
		}
	}

	ListView->RequestListRefresh();
}

TSharedRef<ITableRow> SPinToolActorWindow::OnGenerateRow(TSharedPtr<FString> Slot, const TSharedRef< STableViewBase >& OwnerTable)
{
	auto Row = SNew(STableRow<TSharedPtr<FString>>, OwnerTable);
	Row->ConstructChildren(OwnerTable->TableViewMode, FMargin(1),
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.Padding(FMargin(0, 0, 5, 0))
		.FillWidth(1.0)
		[
			SNew(STextBlock)
			.Text(FText::FromString(*Slot.Get()))
		]
		+ SHorizontalBox::Slot()
		.AutoWidth()
		[
			SNew(SButton)
			.ButtonStyle(FSodaStyle::Get(), "MenuWindow.DeleteButton")
			.OnClicked(this, &SPinToolActorWindow::OnDeleteSlot, Slot)
			.ButtonColorAndOpacity_Lambda([Row]()
			{
				return Row->IsHovered() ? FLinearColor(1, 1, 1, 1) : FLinearColor(1, 1, 1, 0);
			})
		]
	);
	return Row;
}

FReply SPinToolActorWindow::OnNewSave()
{
	if (ToolActor.IsValid())
	{
		if (ToolActor->PinActor(EditableTextBox->GetText().ToString()))
		{
			ToolActor->SavePinnedActor();
		}
	}
	CloseWindow();
	return FReply::Handled();
}

FReply SPinToolActorWindow::OnDeleteSlot(TSharedPtr<FString> Slot)
{
	if (USodaSubsystem* SodaSubsystem = USodaSubsystem::Get())
	{
		TSharedPtr<soda::SMessageBox> MsgBox = SodaSubsystem->ShowMessageBox(
			soda::EMessageBoxType::YES_NO_CANCEL,
			"Delete Slot",
			"Are you sure you want to delete the \"" + *Slot.Get() + "\" slot?");
		MsgBox->SetOnMessageBox(soda::FOnMessageBox::CreateLambda([this, Slot](soda::EMessageBoxButton Button)
		{
			if (Button == soda::EMessageBoxButton::YES)
			{
				if (ToolActor.IsValid())
				{
					ToolActor->DeleteSlot(ToolActor->AsActor()->GetWorld(), *Slot);
					this->UpdateSlots();
				}
			}
		}));
	}
	return FReply::Handled();
}

void SPinToolActorWindow::OnDoubleClickRow(TSharedPtr<FString> Slot)
{
	if (ToolActor.IsValid())
	{
		AActor* Actor = ToolActor->AsActor();
		if (IToolActor* DefaultToolActor = Cast<IToolActor>(Actor->GetClass()->GetDefaultObject()))
		{
			UWorld* World = Actor->GetWorld();
			FTransform Transform = ToolActor->AsActor()->GetActorTransform();
			FName ActorName = ToolActor->AsActor()->GetFName();
			Actor->Destroy();
			AActor* NewActor = DefaultToolActor->LoadPinnedActor(World, Transform, *Slot.Get(), false, ActorName);
			if (USodaSubsystem* SodaSubsystem = USodaSubsystem::Get())
			{
				if (SodaSubsystem->LevelState)
				{
					if(SodaSubsystem->LevelState->ActorFactory) SodaSubsystem->LevelState->ActorFactory->ReplaceActor(NewActor, Actor, false);
					SodaSubsystem->LevelState->MarkAsDirty();
				}
			}
		}
	}
	CloseWindow();
}

} // namespace soda

