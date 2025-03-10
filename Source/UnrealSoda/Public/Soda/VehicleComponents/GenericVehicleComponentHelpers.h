// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Soda/VehicleComponents/VehicleSensorComponent.h"
#include "UObject/WeakObjectPtrTemplates.h"
#include "Templates/SubclassOf.h"
#include "GenericVehicleComponentHelpers.generated.h"

//class UCanvas;
class UVehicleBaseComponent;
struct FSensorDataHeader;

/**
 * UGenericPublisher
 * Base class for all generic publishers
 */
UCLASS(abstract, ClassGroup = Soda)
class UNREALSODA_API UGenericPublisher : public UObject
{
	GENERATED_BODY()

public:
	virtual bool Advertise(UVehicleBaseComponent* Parent) { return false; }
	virtual void Shutdown() {}
	virtual bool IsInitializing() const { return false; }
	virtual bool IsOk() const { return false; }
	virtual void DrawDebug(UCanvas* Canvas, float& YL, float& YPos) {}
	virtual FString GetRemark() const { return ""; }

	virtual bool AdvertiseAndSetHealth(UVehicleBaseComponent* Parent)
	{
		if (Advertise(Parent))
		{
			return true;
		}
		Parent->SetHealth(EVehicleComponentHealth::Warning, "Can't Initialize publisher");
		return false;
	}


	virtual void CheckStatus(UVehicleBaseComponent* Parent) const
	{
		if (Parent->HealthIsWorkable() && Parent->GetHealth() != EVehicleComponentHealth::Warning)
		{
			if (!IsOk() && !IsInitializing())
			{
				Parent->SetHealth(EVehicleComponentHealth::Warning, "Can't Initialize publisher");
			}
		}
	}

	virtual void BeginDestroy() override
	{
		Super::BeginDestroy();
		Shutdown();
	}
};


/**
 * UGenericPublisher
 * Base class for all generic listener
 */
UCLASS(abstract, ClassGroup = Soda)
class UNREALSODA_API UGenericListener : public UObject
{
	GENERATED_BODY()

public:
	virtual bool StartListen(UVehicleBaseComponent * Parent) { return false; }
	virtual void StopListen() {}
	virtual bool IsOk() const { return false; }
	virtual void DrawDebug(UCanvas* Canvas, float& YL, float& YPos) {}
	virtual FString GetRemark() const { return ""; }
};



