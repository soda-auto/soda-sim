// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "UI/Wnds/SVehcileManagerWindow.h"
#include "SodaStyleSet.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SComboButton.h"
#include "Soda/Vehicles/SodaVehicle.h"
#include "GenericPlatform/GenericPlatformFile.h"
#include "Soda/UI/SMessageBox.h"
#include "Soda/SodaSubsystem.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "RuntimeEditorUtils.h"

namespace soda
{

void SVehcileManagerWindow::Construct( const FArguments& InArgs, ASodaVehicle* InVehicle)
{
	Vehicle = InVehicle;

	/*
	if (Vehicle.IsValid())
	{
		if (FDBGateway::Instance().IsConnected() && Vehicle->GetSaveAddress().Source == EVehicleSaveSource::DB)
		{
			SetSource(EVehicleManagerSource::Remote);
		}
	}
	*/
	
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
					.IsEnabled(Vehicle.IsValid())
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SButton)
					.Text(FText::FromString("New Slot"))
					.IsEnabled(Vehicle.IsValid())
					.OnClicked(this, &SVehcileManagerWindow::OnNewSave)
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
					SAssignNew(ListView, SListView<TSharedPtr<FVechicleSaveAddress>>)
					.ListItemsSource(&SavedVehicles)
					.SelectionMode(ESelectionMode::Single)
					.OnGenerateRow(this, &SVehcileManagerWindow::OnGenerateRow)
					.OnSelectionChanged(this, &SVehcileManagerWindow::OnSelectionChanged)
				]
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Right)
			[
				SNew(SComboButton)
				.ContentPadding(3)
				.OnGetMenuContent(this, &SVehcileManagerWindow::GetComboMenuContent)
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
							if(this->GetSource() == EVehicleManagerSource::Local)
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
							if(this->GetSource() == EVehicleManagerSource::Local)
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
					SNew( SButton)
					.Text(FText::FromString("Save"))
					.IsEnabled(Vehicle.IsValid())
					.OnClicked(this, &SVehcileManagerWindow::OnSave)
				]
				+ SHorizontalBox::Slot()
				[
					SNew( SButton)
					.Text(FText::FromString("Load"))
					.IsEnabled(Vehicle.IsValid())
					.OnClicked(this, &SVehcileManagerWindow::OnLoad)
				]
			]
		]
	];

	UpdateSlots();
}

TSharedRef<ITableRow> SVehcileManagerWindow::OnGenerateRow(TSharedPtr<FVechicleSaveAddress> Address, const TSharedRef< STableViewBase >& OwnerTable)
{
	auto Row = SNew(STableRow<TSharedPtr<FString>>, OwnerTable);
	Row->ConstructChildren(OwnerTable->TableViewMode, FMargin(1),
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.FillWidth(1.0)
		[
			SNew(STextBlock)
			.Text(FText::FromString(Address->ToVehicleName()))
		]
		+ SHorizontalBox::Slot()
		.AutoWidth()
		[
			SNew(SButton)
			.ButtonStyle(FSodaStyle::Get(), "MenuWindow.DeleteButton")
			.OnClicked(this, &SVehcileManagerWindow::OnDeleteSlot, Address)
			.ButtonColorAndOpacity_Lambda([Row]()
			{
				return Row->IsHovered() ? FLinearColor(1, 1, 1, 1) : FLinearColor(1, 1, 1, 0);
			})
		]
	);

	return Row;
}

FReply SVehcileManagerWindow::OnDeleteSlot(TSharedPtr<FVechicleSaveAddress> Address)
{
	if (USodaSubsystem* SodaSubsystem = USodaSubsystem::Get())
	{
		TSharedPtr<soda::SMessageBox> MsgBox = SodaSubsystem->ShowMessageBox(
			soda::EMessageBoxType::YES_NO_CANCEL, 
			"Delete Vehicle", 
			"Are you sure you want to delete the \"" + Address->ToVehicleName() + "\" vehicle?");
		MsgBox->SetOnMessageBox(soda::FOnMessageBox::CreateLambda([this, Address](soda::EMessageBoxButton Button)
		{
			if (Button == soda::EMessageBoxButton::YES)
			{
				if (!ASodaVehicle::DeleteVehicleSave(*Address))
				{
					FSlateNotificationManager& NotificationManager = FSlateNotificationManager::Get();
					FNotificationInfo Info(FText::FromString(TEXT("Can't delete the vehicle data: ") + Address->ToUrl()));
					Info.ExpireDuration = 5.0f;
					Info.Image = FCoreStyle::Get().GetBrush(TEXT("Icons.WarningWithColor"));
					NotificationManager.AddNotification(Info);
				}
				UpdateSlots();
			}
		}));
	}
	return FReply::Handled();
}

FReply SVehcileManagerWindow::OnNewSave()
{
	if (Vehicle.IsValid())
	{
		FVechicleSaveAddress Address =
			Source == EVehicleManagerSource::Local ?
			FVechicleSaveAddress(EVehicleSaveSource::JsonLocal, EditableTextBox->GetText().ToString()) : 
			FVechicleSaveAddress(EVehicleSaveSource::DB, EditableTextBox->GetText().ToString());

		if (!Vehicle->SaveToAddress(Address, true))
		{
			if (USodaSubsystem* SodaSubsystem = USodaSubsystem::Get())
			{
				TSharedPtr<soda::SMessageBox> MsgBox = SodaSubsystem->ShowMessageBox(
					soda::EMessageBoxType::OK, "Error", "Can't save vehilce");
			}
		}
	}
	CloseWindow();
	return FReply::Handled();
}

void SVehcileManagerWindow::UpdateSlots()
{
	SavedVehicles.Empty();

	TArray<FVechicleSaveAddress> Addresses;
	if (Source == EVehicleManagerSource::Local)
	{
		ASodaVehicle::GetSavedVehiclesLocal(Addresses);
	}
	else
	{
		if (!ASodaVehicle::GetSavedVehiclesDB(Addresses))
		{
			USodaSubsystem::GetChecked()->ShowMessageBox(soda::EMessageBoxType::OK, "Error", "MongoDB error. See log for more information");
		}
	}

	for (auto& It : Addresses)
	{
		SavedVehicles.Add(MakeShared<FVechicleSaveAddress>(It));
	}

	if (Vehicle.IsValid())
	{
		for (auto& It : SavedVehicles)
		{
			if (*It == Vehicle->GetSaveAddress()) 
			{
				ListView->SetSelection(It);
				break;
			}
		}
	}

	ListView->RequestListRefresh();
}

TSharedRef<SWidget> SVehcileManagerWindow::GetComboMenuContent()
{
	FMenuBuilder MenuBuilder(
		/*bShouldCloseWindowAfterMenuSelection*/ true, 
		/* InCommandList = */nullptr, 
		/* InExtender = */nullptr, 
		/*bCloseSelfOnly*/ false,
		&FSodaStyle::Get());

	MenuBuilder.BeginSection(NAME_None);

	{
		FUIAction Action;
		Action.ExecuteAction.BindLambda([this]() 
		{
			SetSource(EVehicleManagerSource::Remote);
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
			SetSource(EVehicleManagerSource::Local);
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

void SVehcileManagerWindow::OnSelectionChanged(TSharedPtr<FVechicleSaveAddress> Address, ESelectInfo::Type SelectInfo)
{
	if (Address)
	{
		EditableTextBox->SetText(FText::FromString(Address->ToVehicleName()));
		//SaveButton->SetEnabled(true);
		//LoadButton->SetEnabled(true);
	}
	else
	{
		EditableTextBox->SetText(FText::GetEmpty());
		//SaveButton->SetEnabled(false);
		//LoadButton->SetEnabled(false);
	}
}

FReply SVehcileManagerWindow::OnSave()
{
	if (Vehicle.IsValid())
	{
		TSharedPtr<FVechicleSaveAddress> Address;
		if (ListView->GetSelectedItems().Num() == 1) Address = *ListView->GetSelectedItems().begin();
		if (Address)
		{
			if (!Vehicle->Resave())
			{
				if (USodaSubsystem* SodaSubsystem = USodaSubsystem::Get())
				{
					TSharedPtr<soda::SMessageBox> MsgBox = SodaSubsystem->ShowMessageBox(
						soda::EMessageBoxType::OK, "Error", "Can't save vehilce");
				}
			}
		}
	}
	CloseWindow();
	return FReply::Handled();
}

FReply SVehcileManagerWindow::OnLoad()
{
	if (Vehicle.IsValid())
	{
		TSharedPtr<FVechicleSaveAddress> Address;
		if (ListView->GetSelectedItems().Num() == 1) Address = *ListView->GetSelectedItems().begin();
		if (Address)
		{
			Vehicle->RespawnVehcileFromAddress(*Address, FVector(0, 0, 20), FRotator::ZeroRotator, true);
		}
	}
	CloseWindow();
	return FReply::Handled();
}

} // namespace soda

