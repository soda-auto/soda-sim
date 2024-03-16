// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "Soda/VehicleComponents/GenericVehicleComponentHelpers.h"
#include "Soda/Misc/Time.h"
#include "Soda/Misc/Extent.h"
#include "Soda/SodaTypes.h"
#include "GenericRacingPublisher.generated.h"

namespace soda
{
	struct FRacingSensorData;
}

/**
 * UGenericRacingPublisher
 */
UCLASS(abstract, ClassGroup = Soda)
class UNREALSODA_API UGenericRacingPublisher : public UGenericPublisher
{
	GENERATED_BODY()

public:
	virtual bool Publish(float DeltaTime, const FSensorDataHeader& Header, const soda::FRacingSensorData& SensorData) { return false; }
};


