// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "Soda/VehicleComponents/GenericVehicleComponentHelpers.h"
#include "Soda/Misc/Time.h"
#include "GenericRadarPublisher.generated.h"

struct FRadarParams;
struct FRadarClusters;
struct FRadarObjects;

/**
 * UGenericRadarPublisher
 */
UCLASS(abstract, ClassGroup = Soda, EditInlineNew)
class UNREALSODA_API UGenericRadarPublisher: public UGenericPublisher
{
	GENERATED_BODY()

public:
	virtual bool Publish(float DeltaTime, const FSensorDataHeader& Header, const TArray<FRadarParams>& Params, const FRadarClusters& Clusters, const FRadarObjects& Objects) { return false; }
};


