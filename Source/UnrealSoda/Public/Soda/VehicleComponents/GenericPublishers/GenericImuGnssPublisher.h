// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "Soda/VehicleComponents/GenericVehicleComponentHelpers.h"
#include "Soda/Misc/Time.h"
#include "GenericImuGnssPublisher.generated.h"


struct FImuNoiseParams;
struct FPhysBodyKinematic;

/**
 * UGenericImuGnssPublisher
 */
UCLASS(abstract, ClassGroup = Soda)
class UNREALSODA_API UGenericImuGnssPublisher: public UGenericPublisher
{
	GENERATED_BODY()

public:
	virtual bool Publish(float DeltaTime, const FSensorDataHeader& Header, const FTransform& RelativeTransform, const FPhysBodyKinematic& VehicleKinematic, const FImuNoiseParams& Covariance) { return false; }
	virtual bool GetDefaultNoiseParams(const FImuNoiseParams& Params) { return false; }
	
};


