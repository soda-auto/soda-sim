// Copyright 2023 SODA.AUTO UKLTD. All Rights Reserved.

#pragma once

#include "Soda/VehicleComponents/WheeledVehicleComponent.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Soda/VehicleComponents/IOBus.h"
#include "LightSet.generated.h"

class UMaterialInstanceDynamic;
class ULightComponent;

USTRUCT(BlueprintType)
struct UNREALSODA_API FVehicleLightItem
{
	GENERATED_USTRUCT_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = VehicleLightItem)
	TArray<UMaterialInstanceDynamic*> Materials;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = VehicleLightItem)
	TArray<ULightComponent*> Lights;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = VehicleLightItem, SaveGame, meta = (EditInRuntime, ReactivateComponent))
	FIOPinSetup IOPinSetup;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = VehicleLightItem, SaveGame, meta = (EditInRuntime, ReactivateComponent))
	bool bIOPinUsed = false;
	
	void Enable(bool bNewEnabled, bool bForce);
	bool IsEnabled() const { return bEnabled; }

protected:
	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = VehicleLightItem, meta = (EditInRuntime))
	bool bEnabled = false;
};

UCLASS(ClassGroup = Soda, BlueprintType, meta = (BlueprintSpawnableComponent))
class UNREALSODA_API ULightSetMode1Component : public UWheeledVehicleComponent
{
	GENERATED_UCLASS_BODY()
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = LightSet, SaveGame, meta = (EditInRuntime))
	FVehicleLightItem HeightL;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = LightSet, SaveGame, meta = (EditInRuntime))
	FVehicleLightItem HeightR;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = LightSet, SaveGame, meta = (EditInRuntime))
	FVehicleLightItem LowL;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = LightSet, SaveGame, meta = (EditInRuntime))
	FVehicleLightItem LowR;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = LightSet, SaveGame, meta = (EditInRuntime))
	FVehicleLightItem FogL;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = LightSet, SaveGame, meta = (EditInRuntime))
	FVehicleLightItem FogR;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = LightSet, SaveGame, meta = (EditInRuntime))
	FVehicleLightItem DaytimeFL;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = LightSet, SaveGame, meta = (EditInRuntime))
	FVehicleLightItem DaytimeFR;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = LightSet, SaveGame, meta = (EditInRuntime))
	FVehicleLightItem DaytimeRL;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = LightSet, SaveGame, meta = (EditInRuntime))
	FVehicleLightItem DaytimeRR;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = LightSet, SaveGame, meta = (EditInRuntime))
	FVehicleLightItem BrakeL;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = LightSet, SaveGame, meta = (EditInRuntime))
	FVehicleLightItem BrakeR;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = LightSet, SaveGame, meta = (EditInRuntime))
	FVehicleLightItem ReversL;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = LightSet, SaveGame, meta = (EditInRuntime))
	FVehicleLightItem ReversR;

	UFUNCTION(BlueprintCallable, Category = LightSet)
	void EnableAll(bool bEnabled, bool bForce);
	
public:
	//virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	virtual bool OnActivateVehicleComponent() override;
	virtual void OnDeactivateVehicleComponent() override;

protected:
	TSet<FVehicleLightItem*> LightItems;
	
};

/**
 * ULightSetFunctionLibrary
 */
UCLASS(Category = Soda)
class UNREALSODA_API ULightSetFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_UCLASS_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = LightSet)
	static void EnableLight(UPARAM(ref) FVehicleLightItem& VehicleLightItem, bool bEnabled, bool bForce) { VehicleLightItem.Enable(bEnabled, bForce); }

	UFUNCTION(BlueprintCallable, Category = LightSet)
	static bool IsLightEnable(const FVehicleLightItem& VehicleLightItem) { return VehicleLightItem.IsEnabled(); }
};
