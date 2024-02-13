// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "Soda/VehicleComponents/GenericVehicleComponentHelpers.h"
#include "Soda/Misc/Time.h"
#include "GenericUltrasoncPublisher.generated.h"

struct FUltrasonicEchos;

/**
 * UGenericUltrasoncHubPublisher
 */
UCLASS(abstract, ClassGroup = Soda)
class UNREALSODA_API UGenericUltrasoncHubPublisher: public UGenericPublisher
{
	GENERATED_BODY()

public:
	virtual bool Publish(float DeltaTime, const FSensorDataHeader& Header, const TArray < FUltrasonicEchos >& InEchoCollections) { return false; }
};
