// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "Soda/VehicleComponents/GenericVehicleComponentHelpers.h"
#include "Soda/SodaTypes.h"
#include "Soda/Misc/Time.h"
#include "GenericCameraPublisher.generated.h"

struct FCameraFrame;

/**
 * UGenericCameraPublisher
 */
UCLASS(abstract, ClassGroup = Soda, EditInlineNew)
class UNREALSODA_API UGenericCameraPublisher : public UGenericPublisher
{
	GENERATED_BODY()

public:
	/** Invoke in render thread */
	//virtual bool Publish(const FCameraFrame& CameraFrame, const void* DataPtr, uint32 Size) { return false; }

	/** ImageStride - in Original image width in pixels */
	virtual bool Publish(float DeltaTime, const FSensorDataHeader& Header, const FCameraFrame& CameraFrame, const TArray<FColor> & BGRA8, uint32 ImageStride) { return false; }

	//virtual TArray<FColor>& LockBuffer() { static TArray<FColor> Dummy; return Dummy; }
	//virtual void UnlockBuffer(const FCameraFrame& CameraFrame) {}
};
