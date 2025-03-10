// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "Soda/VehicleComponents/GenericVehicleComponentHelpers.h"
#include "Soda/Misc/Time.h"
#include "Soda/Misc/Extent.h"
#include "Soda/SodaTypes.h"
#include "GenericV2XPublisher.generated.h"

class UV2XMarkerSensor;

/**
 * UGenericV2XPublisher
 */
UCLASS(abstract, ClassGroup = Soda, EditInlineNew)
class UNREALSODA_API UGenericV2XPublisher: public UGenericPublisher
{
	GENERATED_BODY()

public:
	virtual bool Publish(float DeltaTime, const FSensorDataHeader& Header, const TArray<UV2XMarkerSensor*>& Transmitters) { return false; }

};


