// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "UI/Wnds/SSaveVehicleRequestWindow.h"
#include "SodaStyleSet.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "UI/Wnds/SVehcileManagerWindow.h"
#include "Soda/UI/SMessageBox.h"
#include "Soda/Vehicles/SodaVehicle.h"
#include "Soda/SodaGameMode.h"

namespace soda
{

void SSaveVehicleRequestWindow::Construct( const FArguments& InArgs, TWeakObjectPtr<ASodaVehicle> InVehicle)
{
	Vehicle = InVehicle;
	check(Vehicle.IsValid());

	ChildSlot
	[
		SNew(SBox)
		.Padding(5)
		.MinDesiredWidth(300)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0, 2, 0, 2)
			[
				SNew(STextBlock)
				.Text(FText::FromString(FString::Printf(TEXT("Save vehicle \"%s\" as: "), *FPaths::GetBaseFilename(Vehicle->GetSaveAddress().ToVehicleName()))))
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0, 2, 0, 2)
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Center)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.Padding(3)
				.AutoWidth()
				[
					SNew(SButton)
					.Text(FText::FromString("Save As New"))
					.OnClicked(this, &SSaveVehicleRequestWindow::OnSaveAs)
				]
				+ SHorizontalBox::Slot()
				.Padding(3)
				.AutoWidth()
				[
					SNew(SButton)
					.Text(FText::FromString("Resave"))
					.OnClicked(this, &SSaveVehicleRequestWindow::OnResave)
				]
				+ SHorizontalBox::Slot()
				.Padding(3)
				.AutoWidth()
				[
					SNew(SButton)
					.Text(FText::FromString("Cancel"))
					.OnClicked(this, &SSaveVehicleRequestWindow::OnCancel)
				]
			]
		]
	];
}

FReply SSaveVehicleRequestWindow::OnSaveAs()
{
	if (USodaGameModeComponent* GameMode = USodaGameModeComponent::Get())
	{
		if (Vehicle.IsValid())
		{
			GameMode->OpenWindow("Save Vehicle As", SNew(SVehcileManagerWindow, Vehicle.Get()));
		}
	}
	CloseWindow();
	return FReply::Handled();
}

FReply SSaveVehicleRequestWindow::OnResave()
{
	if (USodaGameModeComponent* GameMode = USodaGameModeComponent::Get())
	{
		if (!(Vehicle.IsValid() && Vehicle->Resave()))
		{
			TSharedPtr<soda::SMessageBox> MsgBox = GameMode->ShowMessageBox(
				soda::EMessageBoxType::OK, "Error", "Can't save vehilce");
		}
	}
	CloseWindow();
	return FReply::Handled();
}

FReply SSaveVehicleRequestWindow::OnCancel()
{
	CloseWindow();
	return FReply::Handled();
}

} // namespace soda

