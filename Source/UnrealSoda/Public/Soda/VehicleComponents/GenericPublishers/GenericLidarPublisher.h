// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "Soda/VehicleComponents/GenericVehicleComponentHelpers.h"
#include "Soda/Misc/Time.h"
#include "GenericLidarPublisher.generated.h"

namespace soda
{
	struct FLidarScan;
}

/**
 * UGenericLidarPublisher
 */
UCLASS(abstract, ClassGroup = Soda)
class UNREALSODA_API UGenericLidarPublisher : public UGenericPublisher
{
	GENERATED_BODY()

public:
	virtual bool Publish(float DeltaTime, const FSensorDataHeader& Header, const soda::FLidarScan& Scan) { return false; }
};



