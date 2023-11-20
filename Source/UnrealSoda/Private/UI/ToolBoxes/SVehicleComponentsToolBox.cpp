// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "SVehicleComponentsToolBox.h"
#include "Soda/UI/SVehicleComponentsList.h"
#include "Soda/Vehicles/SodaVehicle.h"
#include "Soda/SodaGameViewportClient.h"

#define LOCTEXT_NAMESPACE "VehicleComponentsBar"

namespace soda
{

void SVehicleComponentsToolBox::Construct( const FArguments& InArgs, ASodaVehicle* InVehicle)
{
	check(InVehicle);

	Vehicle = InVehicle;
	Caption = FText::FromString(InVehicle->GetName());

	ChildSlot
	[
		SNew(SVehicleComponentsList, InVehicle, true)
	];
}

} // namespace soda

#undef LOCTEXT_NAMESPACE
