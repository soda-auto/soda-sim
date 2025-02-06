// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "UI/Wnds/SVehcileManagerWindow.h"
#include "Soda/UI/SFileDatabaseManager.h"
#include "Soda/FileDatabaseManager.h"
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

#define LOCTEXT_NAMESPACE "SVehcileManagerWindow"

namespace soda
{


void SVehcileManagerWindow::Construct( const FArguments& InArgs, ASodaVehicle* InVehicle)
{
	SelectedVehicle = InVehicle;
	
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
			.Padding(0, 0, 0, 5)
			[
				SAssignNew(FileDatabaseManager, SFileDatabaseManager, EFileSlotType::Vehicle, SelectedVehicle.IsValid() ? SelectedVehicle->GetSlotGuid() : FGuid())
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
						SNew( SButton)
						.HAlign(HAlign_Center)
						.Text(FText::FromString("Save"))
						.IsEnabled(this, &SVehcileManagerWindow::CanSave)
						.OnClicked(this, &SVehcileManagerWindow::OnSave)
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
						.IsEnabled(this, &SVehcileManagerWindow::CanNewSlot)
						.OnClicked(this, &SVehcileManagerWindow::OnNewSave)
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
						.IsEnabled(this, &SVehcileManagerWindow::CanRespwn)
						.OnClicked(this, &SVehcileManagerWindow::OnReplaceSelectedVehicle)
					]
				]
				
			]
		]
	];
}

FReply SVehcileManagerWindow::OnNewSave()
{
	if (SelectedVehicle.IsValid())
	{
		if (!SelectedVehicle->SaveToSlot(FileDatabaseManager->GetLableText(), FileDatabaseManager->GetDescriptionText(), FGuid(), true))
		{
			soda::ShowNotification(ENotificationLevel::Error, 5.0, TEXT("Can't save vehilce to a new slot"));
		}
	}
	CloseWindow();
	return FReply::Handled();
}

FReply SVehcileManagerWindow::OnSave()
{
	if (SelectedVehicle.IsValid())
	{
		if (TSharedPtr<FFileDatabaseSlotInfo> Slot = FileDatabaseManager->GetSelectedSlot())
		{
			USodaSubsystem* SodaSubsystem = USodaSubsystem::GetChecked();

			auto ProccSave = [this, Slot]()
			{
				if (!SelectedVehicle->SaveToSlot(FileDatabaseManager->GetLableText(), FileDatabaseManager->GetDescriptionText(), Slot->GUID, true))
				{
					soda::ShowNotification(ENotificationLevel::Error, 5.0, TEXT("Can't save vehilce to the selected slot"));
				}
			};

			if (!SelectedVehicle->GetSlotGuid().IsValid() || SelectedVehicle->GetSlotGuid() != Slot->GUID)
			{
				TSharedPtr<soda::SMessageBox> MsgBox = SodaSubsystem->ShowMessageBox(
					soda::EMessageBoxType::YES_NO_CANCEL,
					"Replace Slot",
					"Are you sure you want to replace the \"" + Slot->Lable + "\" with new vehicle data ?");
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

FReply SVehcileManagerWindow::OnReplaceSelectedVehicle()
{
	if (SelectedVehicle.IsValid())
	{
		if (TSharedPtr<FFileDatabaseSlotInfo> Slot = FileDatabaseManager->GetSelectedSlot())
		{
			FTransform SpawnTransform = FTransform(FRotator(0.f, SelectedVehicle->GetActorRotation().Yaw, 0.f), SelectedVehicle->GetActorLocation());
			bool NeedPosses = SelectedVehicle->IsPlayerControlled();
			UWorld* World = SelectedVehicle->GetWorld();
			ASodaVehicle* SelectedVehiclePtr = SelectedVehicle.Get();

			SelectedVehicle->Destroy();
			ASodaVehicle* NewVehicle = ASodaVehicle::SpawnVehicleFormSlot(World, Slot->GUID, SpawnTransform.GetTranslation(), SpawnTransform.GetRotation().Rotator(), NeedPosses);

			USodaSubsystem* SodaSubsystem = USodaSubsystem::GetChecked();
			SodaSubsystem->NotifyLevelIsChanged();
			if (SodaSubsystem->LevelState && SodaSubsystem->LevelState->ActorFactory)
			{
				SodaSubsystem->LevelState->ActorFactory->ReplaceActor(NewVehicle, SelectedVehiclePtr, false);
			}
			if (SodaSubsystem->UnpossesVehicle == SelectedVehiclePtr) SodaSubsystem->UnpossesVehicle = NewVehicle;
		}
	}
	CloseWindow();
	return FReply::Handled();
}

bool SVehcileManagerWindow::CanRespwn() const
{
	if (SelectedVehicle.IsValid())
	{
		if (TSharedPtr<FFileDatabaseSlotInfo> Slot = FileDatabaseManager->GetSelectedSlot())
		{
			return Slot->SourceVersion != EFileSlotSourceVersion::RemoteOnly;
		}
	}
	return false;
}

bool SVehcileManagerWindow::CanSave() const
{
	if (SelectedVehicle.IsValid())
	{
		if (TSharedPtr<FFileDatabaseSlotInfo> Slot = FileDatabaseManager->GetSelectedSlot())
		{
			return Slot->SourceVersion != EFileSlotSourceVersion::RemoteOnly;
		}
	}
	return false;
}

bool SVehcileManagerWindow::CanNewSlot() const
{
	return SelectedVehicle.IsValid();
}



} // namespace soda

