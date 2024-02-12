// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "Soda/VehicleComponents/GenericVehicleComponentHelpers.h"
#include "Soda/Vehicles/VehicleBaseTypes.h"
#include "GenericWheeledVehicleControl.generated.h"

namespace soda
{
	struct FWheeledVehiclControlMode1;
}

/**
 * UGenericWheeledVehiclePublisher
 */
UCLASS(abstract, ClassGroup = Soda)
class UNREALSODA_API UGenericWheeledVehicleControlListener: public UGenericListener
{
	GENERATED_BODY()

public:
	virtual bool GetControl(soda::FWheeledVehiclControlMode1& Control) const { return false; }
};


