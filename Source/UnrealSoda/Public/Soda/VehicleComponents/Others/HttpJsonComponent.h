// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Soda/VehicleComponents/WheeledVehicleComponent.h"
#include "Http.h"
#include "HttpJsonComponent.generated.h"

/**
 * 
 */
UCLASS()
class UNREALSODA_API UHttpJsonComponent : public UWheeledVehicleComponent
{
	GENERATED_BODY()

protected:
	virtual bool OnActivateVehicleComponent() override;
	virtual void OnDeactivateVehicleComponent() override;
	
private:
	void OnResponceRecieved(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully);
};
