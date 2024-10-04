// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "Soda/VehicleComponents/VehicleBaseComponent.h"
#include "IODev.generated.h"

class UIOBusComponent;
class UIOExchange;
struct FIOExchangeSourceValue;
struct FIOPinDescription;

/**
 * UIODevComponent
 */
UCLASS(abstract, ClassGroup = Soda, BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class UNREALSODA_API UIODevComponent : public UVehicleBaseComponent
{
	GENERATED_UCLASS_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Link, SaveGame, meta = (EditInRuntime, ReactivateActor, AllowedClasses = "/Script/UnrealSoda.IOBusComponent"))
	FSubobjectReference LinkToIOBus;

	virtual void OnSetExchangeValue(const UIOExchange* Exchange, const FIOExchangeSourceValue& Value) {}
	virtual void OnChangePins() {}


protected:
	virtual void OnPreActivateVehicleComponent() override;
	virtual bool OnActivateVehicleComponent() override;
	virtual void OnDeactivateVehicleComponent() override;
	virtual FString GetRemark() const override;
	virtual void DrawDebug(UCanvas* Canvas, float& YL, float& YPos) override;

protected:
	UPROPERTY();
	UIOBusComponent* IOBus{};

};
