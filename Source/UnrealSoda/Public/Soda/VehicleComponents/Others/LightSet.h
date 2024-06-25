// Copyright 2023 SODA.AUTO UKLTD. All Rights Reserved.

#pragma once

#include "Soda/VehicleComponents/WheeledVehicleComponent.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Soda/VehicleComponents/IOBus.h"
#include "LightSet.generated.h"

class UMaterialInstanceDynamic;
class ULightComponent;

USTRUCT(BlueprintType)
struct UNREALSODA_API FVehicleLightItemSetup
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = VehicleLightItem, SaveGame, meta = (EditInRuntime, ReactivateComponent))
	FName LightName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = VehicleLightItem, SaveGame, meta = (EditInRuntime, ReactivateComponent))
	FName MaterialPropertyName = TEXT("Intensity");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = VehicleLightItem, SaveGame, meta = (EditInRuntime, ReactivateComponent))
	float IntensityMultiplayer = 10;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = VehicleLightItem, Transient)
	TArray<UMaterialInstanceDynamic*> Materials;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = VehicleLightItem, Transient)
	TArray<ULightComponent*> Lights;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = VehicleLightItem, SaveGame, meta = (EditInRuntime, ReactivateComponent))
	FIOPinSetup IOPinSetup;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = VehicleLightItem, SaveGame, meta = (EditInRuntime, ReactivateComponent))
	bool bUseIO = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = VehicleLightItem, SaveGame, meta = (EditInRuntime, ReactivateComponent))
	bool bSendIOFeedback = false;

	/** [Ohm], uses for calculate current for IOPin */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = VehicleLightItem, SaveGame, meta = (EditInRuntime, ReactivateComponent))
	float Resistanse = 4;
};

USTRUCT(BlueprintType)
struct UNREALSODA_API FVehicleLightItemSetupHelper
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = VehicleLightItem)
	TArray<UMaterialInstanceDynamic*> Materials;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = VehicleLightItem)
	TArray<ULightComponent*> Lights;
};

UCLASS(ClassGroup = Soda, BlueprintType, meta = (BlueprintSpawnableComponent))
class UNREALSODA_API UVehicleLightItem : public UObject
{
	GENERATED_UCLASS_BODY()

public:
	void Setup(const FVehicleLightItemSetup& ItemSetup);
	bool SetupIO(UIOBusNode* Node);

	/** Intensity = [0..1] */
	UFUNCTION(BlueprintCallable, Category = LightSet)
	void SetEnabled(bool bNewEnabled, float Intensity);

	UFUNCTION(BlueprintCallable, Category = LightSet)
	bool IsEnabled() const { return bEnabled; }

	UFUNCTION(BlueprintCallable, Category = LightSet)
	const FVehicleLightItemSetup& GetSetup() const { return ItemSetup; }

	UFUNCTION(BlueprintCallable, Category = LightSet)
	UIOExchange * GetExchange() const { return Exchange; }

protected:
	bool bEnabled = false;
	FVehicleLightItemSetup ItemSetup;

	UPROPERTY();
	UIOExchange* Exchange {};
};

UCLASS(ClassGroup = Soda, BlueprintType, meta = (BlueprintSpawnableComponent))
class UNREALSODA_API ULightSetComponent : public UWheeledVehicleComponent
{
	GENERATED_UCLASS_BODY()
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = LightSet, SaveGame, meta = (EditInRuntime))
	FName IOBusNodeName = TEXT("LightSetNode");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Link, SaveGame, meta = (EditInRuntime, ReactivateActor, AllowedClasses = "/Script/UnrealSoda.IOBusComponent"))
	FSubobjectReference LinkToIOBus;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = LightSet, SaveGame, meta = (EditInRuntime))
	bool bUseIOs = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = LightSet, SaveGame, meta = (EditInRuntime, ReactivateActor))
	TArray<FVehicleLightItemSetup> DefaultLightSetups;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = LightSet)
	TMap<FName, FVehicleLightItemSetupHelper> LightItemSetupHelper;

public:
	UFUNCTION(BlueprintCallable, Category = LightSet)
	void SetEnabledAllLights(bool bEnabled);

	UFUNCTION(BlueprintCallable, Category = LightSet)
	UVehicleLightItem* AddLightItem(const FVehicleLightItemSetup& Setup);

	UFUNCTION(BlueprintCallable, Category = LightSet)
	void AddLightItems(const TArray<FVehicleLightItemSetup>& Setups);

	UFUNCTION(BlueprintCallable, Category = LightSet)
	bool RemoveLightItemByName(const FName& LightName);

	UFUNCTION(BlueprintCallable, Category = LightSet)
	bool RemoveLightItem(const UVehicleLightItem * LightItem);

	UFUNCTION(BlueprintCallable, Category = LightSet)
	const TMap<FName, UVehicleLightItem*>& GetLightItems() const { return LightItems; }
	
public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	virtual void OnPreActivateVehicleComponent() override;
	virtual bool OnActivateVehicleComponent() override;
	virtual void OnDeactivateVehicleComponent() override;

protected:
	UPROPERTY();
	TMap<FName, UVehicleLightItem*> LightItems;

	UPROPERTY();
	UIOBusComponent* IOBus{};

	UPROPERTY();
	UIOBusNode* Node{};
};