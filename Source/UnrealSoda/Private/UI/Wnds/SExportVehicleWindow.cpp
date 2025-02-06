// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "UI/Wnds/SExportVehicleWindow.h"
#include "SodaStyleSet.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "UI/Wnds/SVehcileManagerWindow.h"
#include "Soda/UI/SMessageBox.h"
#include "Soda/Vehicles/SodaVehicle.h"
#include "Soda/SodaSubsystem.h"
#include "Soda/SodaApp.h"
#include "DesktopPlatformModule.h"
#include "Misc/FileHelper.h"

namespace soda
{

void SExportVehicleWindow::Construct( const FArguments& InArgs, TWeakObjectPtr<ASodaVehicle> InVehicle)
{
	Vehicle = InVehicle;
	check(Vehicle.IsValid());

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
				SNew(STextBlock)
				.Text(FText::FromString("Export To:"))
			]
			+ SVerticalBox::Slot()
			.FillHeight(1)
			.Padding(0, 0, 0, 5)
			[
				SNew(SBorder)
				.BorderImage(FSodaStyle::GetBrush("MenuWindow.Content"))
				.Padding(5.0f)
				[
					SAssignNew(ListView, SListView<TSharedPtr<soda::ISodaVehicleExporter>>)
					.ListItemsSource(&Source)
					.SelectionMode(ESelectionMode::Single)
					.OnGenerateRow(this, &SExportVehicleWindow::OnGenerateRow)
					.OnSelectionChanged(this, &SExportVehicleWindow::OnSelectionChanged)
				]
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Left)
			[
				SAssignNew(ExportButton, SButton)
				.Text(FText::FromString("Export"))
				.OnClicked(this, &SExportVehicleWindow::OnExportAs)
			]
		]
	];

	UpdateSlots();
}

void SExportVehicleWindow::UpdateSlots()
{
	Source.Empty();
	ExportButton->SetEnabled(false);
	SodaApp.GetVehicleExporters().GenerateValueArray(Source);
}

TSharedRef<ITableRow> SExportVehicleWindow::OnGenerateRow(TSharedPtr<soda::ISodaVehicleExporter> Exporter, const TSharedRef< STableViewBase >& OwnerTable)
{
	return 
		SNew(STableRow<TSharedPtr<soda::ISodaVehicleExporter>>, OwnerTable)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.Padding(FMargin(0, 0, 5, 0))
			//.AutoWidth()
			[
				SNew(STextBlock)
				.Text(FText::FromName(Exporter->GetExporterName()))
			]
		];
}

void SExportVehicleWindow::OnSelectionChanged(TSharedPtr<soda::ISodaVehicleExporter> Exporter, ESelectInfo::Type SelectInfo)
{
	ExportButton->SetEnabled(Exporter.IsValid());
}

FReply SExportVehicleWindow::OnExportAs()
{
	TSharedPtr<ISodaVehicleExporter> Exporter;
	if (ListView->GetSelectedItems().Num() == 1) Exporter = *ListView->GetSelectedItems().begin();

	if (!Exporter)
	{
		FReply::Handled();
	}
		
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	if (!DesktopPlatform)
	{
		UE_LOG(LogSoda, Error, TEXT("SExportVehicleWindow::OnExportAs(); Can't get the IDesktopPlatform ref"));
		FReply::Handled();
	}

	const FString FileTypes = Exporter->GetFileTypes(); 

	TArray<FString> OutFilenames;
	if (!DesktopPlatform->SaveFileDialog(nullptr, FString(TEXT("Export to ")) + Exporter->GetExporterName().ToString(), TEXT(""), TEXT(""), FileTypes, EFileDialogFlags::None, OutFilenames) || OutFilenames.Num() <= 0)
	{
		UE_LOG(LogSoda, Warning, TEXT("SExportVehicleWindow::OnExportAs(); File isn't change"));
		FReply::Handled();
	}

	if (OutFilenames.Num() != 1)
	{
		UE_LOG(LogSoda, Warning, TEXT("SExportVehicleWindow::OnExportAs(); File isn't change"));
		FReply::Handled();
	}

	FString JsonString;
	if (!Exporter->ExportToString(Vehicle.Get(), JsonString))
	{
		UE_LOG(LogSoda, Error, TEXT("SExportVehicleWindow::OnExportAs(); Can't export to '%s'"), *Exporter->GetExporterName().ToString());
		FReply::Handled();
	}

	if (!FFileHelper::SaveStringToFile(JsonString, *OutFilenames[0]))
	{
		UE_LOG(LogSoda, Error, TEXT("SExportVehicleWindow::OnExportAs(); Can't write to '%s' file"), *OutFilenames[0]);
		FReply::Handled();
	}
	
	CloseWindow();
	return FReply::Handled();
}

} // namespace soda

