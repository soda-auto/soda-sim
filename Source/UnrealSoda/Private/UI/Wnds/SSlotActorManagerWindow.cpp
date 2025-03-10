// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "UI/Wnds/SSlotActorManagerWindow.h"
#include "Soda/UI/SFileDatabaseManager.h"
#include "Soda/SodaApp.h"
#include "Soda/UI/SMessageBox.h"
#include "Soda/SodaSubsystem.h"
#include "Soda/Vehicles/SodaVehicle.h"
#include "Soda/LevelState.h"
#include "Soda/SodaActorFactory.h"
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

#define LOCTEXT_NAMESPACE "SSlotActorManagerWindow"

namespace soda
{


void SSlotActorManagerWindow::Construct( const FArguments& InArgs, EFileSlotType Type, AActor* InSelectedActor)
{
	SelectedActor = InSelectedActor;
	
	ISodaActor* SodaActor = Cast<ISodaActor>(SelectedActor);

	//USodaSubsystem* SodaSubsystem = USodaSubsystem::GetChecked();
	//check(SodaSubsystem->GetActorFactory()->CheckActorIsExist(SelectedActor));

	ChildSlot
	[
		SNew(SBox)
		.MinDesiredWidth(600)
		.MinDesiredHeight(600)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.FillHeight(1.0)
			.Padding(0, 0, 0, 5)
			[
				SAssignNew(FileDatabaseManager, SFileDatabaseManager, Type, SodaActor ? SodaActor->GetSlotGuid() : FGuid())
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0, 0, 0, 5)
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
						SNew( SButton)
						.HAlign(HAlign_Center)
						.Text(FText::FromString("Save"))
						.IsEnabled(this, &SSlotActorManagerWindow::CanSave)
						.OnClicked(this, &SSlotActorManagerWindow::OnSave)
					]
				]

				+ SHorizontalBox::Slot()
				.Padding(0, 0, 5, 0)
				.AutoWidth()
				[
					SNew(SBox)
					.MinDesiredWidth(100)
					[
						SNew(SButton)
						.Text(FText::FromString("New Slot"))
						.HAlign(HAlign_Center)
						.IsEnabled(this, &SSlotActorManagerWindow::CanNewSlot)
						.OnClicked(this, &SSlotActorManagerWindow::OnNewSave)
					]
				]
				+ SHorizontalBox::Slot()
				[
					SNew(SBox)
					.MinDesiredWidth(100)
					[
						SNew( SButton)
						.Text(FText::FromString("Respawn"))
						.HAlign(HAlign_Center)
						.ToolTipText(FText::FromString("Respawn the selected vehicle to a new one from the selected slot"))
						.IsEnabled(this, &SSlotActorManagerWindow::CanRespwn)
						.OnClicked(this, &SSlotActorManagerWindow::OnReplaceSelectedActor)
					]
				]
				
			]
		]
	];
}

FReply SSlotActorManagerWindow::OnNewSave()
{
	ISodaActor* SodaActor = Cast<ISodaActor>(SelectedActor);
	if (SodaActor)
	{
		if (!SodaActor->SaveToSlot(FileDatabaseManager->GetLabelText(), FileDatabaseManager->GetDescriptionText(), FGuid(), true))
		{
			soda::ShowNotification(ENotificationLevel::Error, 5.0, TEXT("Can't save vehilce to a new slot"));
		}
	}
	CloseWindow();
	return FReply::Handled();
}

FReply SSlotActorManagerWindow::OnSave()
{
	ISodaActor* SodaActor = Cast<ISodaActor>(SelectedActor);
	if (SodaActor)
	{
		if (TSharedPtr<FFileDatabaseSlotInfo> Slot = FileDatabaseManager->GetSelectedSlot())
		{
			USodaSubsystem* SodaSubsystem = USodaSubsystem::GetChecked();

			auto ProccSave = [Slot, SelectedVehicle = SelectedActor, Label = FileDatabaseManager->GetLabelText(), Description = FileDatabaseManager->GetDescriptionText(), SodaActor]()
			{
				if(SelectedVehicle.IsValid())
				{
					if (!SodaActor->SaveToSlot(Label, Description, Slot->GUID, true))
					{
						soda::ShowNotification(ENotificationLevel::Error, 5.0, TEXT("Can't save to the selected slot"));
					}
				}
			};

			if (!SodaActor->GetSlotGuid().IsValid() || SodaActor->GetSlotGuid() != Slot->GUID)
			{
				TSharedPtr<soda::SMessageBox> MsgBox = SodaSubsystem->ShowMessageBox(
					soda::EMessageBoxType::YES_NO_CANCEL,
					"Replace Slot",
					"Are you sure you want to replace the \"" + Slot->Label + "\" with new slot data ?");
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
	}
	CloseWindow();
	return FReply::Handled();
}

FReply SSlotActorManagerWindow::OnReplaceSelectedActor()
{
	USodaSubsystem* SodaSubsystem = USodaSubsystem::GetChecked();

	if (!SelectedActor.IsValid())
	{
		return FReply::Handled();
	}

	TSharedPtr<FFileDatabaseSlotInfo> Slot = FileDatabaseManager->GetSelectedSlot();
	if (!Slot)
	{
		return FReply::Handled();
	}

	if (!IsValid(Slot->DataClass))
	{
		return FReply::Handled();
	}

	ISodaActor* SodaActor = Cast<ISodaActor>(Slot->DataClass->GetDefaultObject());
	if (!SodaActor)
	{
		return FReply::Handled();
	}

	FTransform SpawnTransform = FTransform(FRotator(0.f, SelectedActor->GetActorRotation().Yaw, 0.f), SelectedActor->GetActorLocation());
	APawn* SelectedPawn = Cast<APawn>(SelectedActor);
	bool NeedPosses = SelectedPawn && SelectedPawn->IsPlayerControlled();
	UWorld* World = SelectedActor->GetWorld();
	
	AActor* StoredActorPtr = SelectedActor.Get();
	SelectedActor->Destroy();

	AActor* NewActor = SodaActor->SpawnActorFromSlot(World, Slot->GUID, SpawnTransform);

	if (NeedPosses)
	{
		if (APawn* NewPawn = Cast<APawn>(NewActor))
		{
			APlayerController* PlayerController = World->GetFirstPlayerController();
			PlayerController->Possess(NewPawn);
		}
	}

	if (ASodaActorFactory* ActorFactory = SodaSubsystem->GetActorFactory())
	{
		ActorFactory->ReplaceActor(NewActor, StoredActorPtr, false);
	}

	if (SodaSubsystem->UnpossesVehicle == StoredActorPtr)
	{
		SodaSubsystem->UnpossesVehicle = Cast<ASodaVehicle>(NewActor);
	}

	CloseWindow();

	return FReply::Handled();
}

bool SSlotActorManagerWindow::CanRespwn() const
{
	if (SelectedActor.IsValid())
	{
		if (TSharedPtr<FFileDatabaseSlotInfo> Slot = FileDatabaseManager->GetSelectedSlot())
		{
			return Slot->SourceVersion != EFileSlotSourceVersion::RemoteOnly;
		}
	}
	return false;
}

bool SSlotActorManagerWindow::CanSave() const
{
	if (ISodaActor* SodaActor = Cast<ISodaActor>(SelectedActor))
	{
		if (SodaActor->GetSlotGuid().IsValid())
		{
			if (TSharedPtr<FFileDatabaseSlotInfo> Slot = FileDatabaseManager->GetSelectedSlot())
			{
				return Slot->SourceVersion != EFileSlotSourceVersion::RemoteOnly;
			}
		}
	}
	return false;
}

bool SSlotActorManagerWindow::CanNewSlot() const
{
	return SelectedActor.IsValid();
}



} // namespace soda

