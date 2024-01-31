// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "Soda/VehicleComponents/GenericVehicleComponentHelpers.h"
#include "Soda/Vehicles/VehicleBaseTypes.h"
#include "GenericWheeledVehiclePublisher.generated.h"


struct FWheeledVehicleStateExtra;

/**
 * UGenericWheeledVehiclePublisher
 */
UCLASS(abstract, ClassGroup = Soda)
class UNREALSODA_API UGenericWheeledVehiclePublisher: public UGenericPublisher
{
	GENERATED_BODY()

public:
	virtual bool Publish(float DeltaTime, const FSensorDataHeader& Header, const FWheeledVehicleStateExtra& VehicleState) { return false; }
};


