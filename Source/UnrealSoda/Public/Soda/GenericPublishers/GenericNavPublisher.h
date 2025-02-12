// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "Soda/VehicleComponents/GenericVehicleComponentHelpers.h"
#include "Soda/Misc/Time.h"
#include "GenericNavPublisher.generated.h"

struct FImuNoiseParams;
struct FPhysBodyKinematic;

/**
 * UGenericNavPublisher
 */
UCLASS(abstract, ClassGroup = Soda, EditInlineNew, EditInlineNew)
class UNREALSODA_API UGenericNavPublisher: public UGenericPublisher
{
	GENERATED_BODY()

public:
	virtual bool Publish(float DeltaTime, const FSensorDataHeader& Header, const FTransform& RelativeTransform, const FPhysBodyKinematic& VehicleKinematic, const FImuNoiseParams& Covariance) { return false; }
	virtual bool GetDefaultNoiseParams(const FImuNoiseParams& Params) { return false; }
	
};


