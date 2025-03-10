// Copyright 2023 SODA.AUTO UKLTD. All Rights Reserved.

#pragma once

#include "Soda/VehicleComponents/WheeledVehicleComponent.h"
#include "Soda/VehicleComponents/IOBus.h"
#include "UObject/StrongObjectPtr.h"
#include "OutputButtons.generated.h"

UENUM(BlueprintType)
enum class EIOButtonSwitchMode : uint8
{
	Momentary,
	Position, 
	Analogue
};

USTRUCT(BlueprintType)
struct FIOButtonArrayItemSetup
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, SaveGame, Category = IOButtonItemSetup, meta = (EditInRuntime))
	FKey Key{};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, SaveGame, Category = IOButtonItemSetup, meta = (EditInRuntime))
	float Voltage{};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, SaveGame, Category = IOButtonItemSetup, meta = (EditInRuntime))
	EIOButtonSwitchMode SwitchMode = EIOButtonSwitchMode::Momentary;
};

USTRUCT(BlueprintType)
struct FIOButtonArraySetup
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, SaveGame, Category = IOButtonArraySetup, meta = (EditInRuntime))
	TArray<FIOButtonArrayItemSetup> ButtonItmes;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, SaveGame, Category = IOButtonArraySetup, meta = (EditInRuntime))
	float ReleasedVoltage{};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, SaveGame, Category = IOButtonArraySetup, meta = (EditInRuntime))
	FIOPinSetup Pin;
};

UCLASS(ClassGroup = Soda, BlueprintType, meta = (BlueprintSpawnableComponent))
class UNREALSODA_API UOutputButtonsComponent : public UWheeledVehicleComponent
{
	GENERATED_UCLASS_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Link, SaveGame, meta = (EditInRuntime, ReactivateActor, AllowedClasses = "/Script/UnrealSoda.IOBusComponent"))
	FSubobjectReference LinkToIOBus;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = ThrottalPedal, SaveGame, meta = (EditInRuntime, ReactivateActor))
	TArray<FIOButtonArraySetup> ButtonSetups;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	virtual void OnPreActivateVehicleComponent() override;
	virtual bool OnActivateVehicleComponent() override;
	virtual void OnDeactivateVehicleComponent() override;

protected:
	UPROPERTY();
	UIOBusComponent* IOBus{};

	struct FIOButtonArrayItem
	{
		FIOButtonArraySetup Setup{};
		bool bIsPressed {};
		float Voltage {};
		TStrongObjectPtr<UIOPin> PinInterface{};
	};

	TArray<FIOButtonArrayItem> Buttons;
};
