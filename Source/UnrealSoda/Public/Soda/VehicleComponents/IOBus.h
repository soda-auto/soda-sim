// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "Soda/VehicleComponents/VehicleBaseComponent.h"
#include "IOBus.generated.h"

/**
 * EIOPinMode
 */
UENUM(BlueprintType)
enum class EIOPinMode : uint8
{
	Undefined,
	Fixed,
	Analogue,
	PWM,
	Input
};

/**
 * FIOPinSourceValue
 */
USTRUCT(BlueprintType)
struct UNREALSODA_API FIOPinSourceValue
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(BlueprintReadWrite, Category = SourceValue)
	EIOPinMode Mode = EIOPinMode::Undefined;

	UPROPERTY(BlueprintReadWrite, Category = SourceValue)
	bool LogicalVal = false;

	/** [V] */
	UPROPERTY(BlueprintReadWrite, Category = SourceValue)
	float Voltage = 0;

	/** [Hz] valid only if Mode=PWM */
	UPROPERTY(BlueprintReadWrite, Category = SourceValue)
	float Frequency = 0;

	/** [0..1] valid only if Mode=PWM */
	UPROPERTY(BlueprintReadWrite, Category = SourceValue)
	float Duty = 0;
};

/**
 * FIOPinSourceValue
 */
USTRUCT(BlueprintType)
struct UNREALSODA_API FIOPinFeedbackValue
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(BlueprintReadWrite, Category = SourceValue)
	float MeasuredCurrent = 0;

	UPROPERTY(BlueprintReadWrite, Category = SourceValue)
	float MeasuredVoltage = 0;

	UPROPERTY(BlueprintReadWrite, Category = SourceValue)
	bool bIsMeasuredCurrentValid = false;

	UPROPERTY(BlueprintReadWrite, Category = SourceValue)
	bool bIsMeasuredVoltageValid = false;
};

/**
 * FIOPinSetup
 */
USTRUCT(BlueprintType)
struct UNREALSODA_API FIOPinSetup
{
	GENERATED_USTRUCT_BODY()

public:
	/** Wire name. Used to connect two pins. Example: wire0, wire1 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = PinSetup, meta = (EditInRuntime, ReactivateComponent))
	FName WireKey;

	/** Example: X1.20, X2.9 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = PinSetup, meta = (EditInRuntime))
	FName PinName;

	/** Example: HB30_OUT_A1, UIO1 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = PinSetup, meta = (EditInRuntime))
	FName PinFunction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = PinSetup, meta = (EditInRuntime))
	int PinIndex = INDEX_NONE;

	/** Example: WAGO_202 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = PinSetup, meta = (EditInRuntime))
	FName DeviceName;

	/** [V] */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = PinSetup, meta = (EditInRuntime))
	float MinVoltage = 0;

	/** [V] */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = PinSetup, meta = (EditInRuntime))
	float MaxVoltage = 1;

	/** [A] */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = PinSetup, meta = (EditInRuntime))
	float MaxCurrent = 1;
};

/**
 * FIOPinInputData
 */
USTRUCT(BlueprintType)
struct UNREALSODA_API FIOPinInputData
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = IOPinDescription, meta = (EditInRuntime))
	FIOPinSourceValue SourceValuel;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = IOPinDescription, meta = (EditInRuntime))
	FIOPinFeedbackValue FeedbackValue;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = IOPinDescription, meta = (EditInRuntime))
	FIOPinSetup RemotePinSetup;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = IOPinDescription, meta = (EditInRuntime))
	FTimestamp SourceTimestamp;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = IOPinDescription, meta = (EditInRuntime))
	FTimestamp FeedbackTimestamp;
};

/**
 * UIOPin
 */
UCLASS(abstract, BlueprintType, Blueprintable)
class UNREALSODA_API UIOPin: public UObject
{
	GENERATED_UCLASS_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = IOPin)
	virtual void PublishSource(FIOPinSourceValue& Value) {} 

	UFUNCTION(BlueprintCallable, Category = IOPin)
	virtual void PublishFeedback(FIOPinFeedbackValue& Value) {}

	UFUNCTION(BlueprintCallable, Category = IOPin)
	virtual const FIOPinSetup& GetLocalPinSetup() const { static FIOPinSetup Dummy{}; return Dummy; }

	UFUNCTION(BlueprintCallable, Category = IOPin)
	virtual const FIOPinSourceValue& GetInputSourceValue(FTimestamp& OutTimestamp) const { return GetInputSourceValue(&OutTimestamp); }

	UFUNCTION(BlueprintCallable, Category = IOPin)
	virtual const FIOPinFeedbackValue& GetInputFeedbackValue(FTimestamp& OutTimestamp) const { return GetInputFeedbackValue(&OutTimestamp); }

	UFUNCTION(BlueprintCallable, Category = IOPin)
	virtual const FIOPinSetup& GetRemotePinSetup(FTimestamp& OutTimestamp) const { return GetRemotePinSetup(&OutTimestamp); }

	virtual const FIOPinSourceValue& GetInputSourceValue(FTimestamp* OutTimestamp = nullptr) const { static FIOPinSourceValue Dummy{}; return Dummy; }
	virtual const FIOPinFeedbackValue& GetInputFeedbackValue(FTimestamp* OutTimestamp = nullptr) const { static FIOPinFeedbackValue Dummy{}; return Dummy; }
	virtual const FIOPinSetup& GetRemotePinSetup(FTimestamp* OutTimestam = nullptr) const { static FIOPinSetup Dummy{}; return Dummy; }
};

/**
 * UIOBusComponent
 */
UCLASS(abstract, ClassGroup = Soda, BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class UNREALSODA_API UIOBusComponent : public UVehicleBaseComponent
{
	GENERATED_UCLASS_BODY()

public:

	/** CreatePin() store only weak pointer to the created UIOPin object. */
	virtual UIOPin * CreatePin(const FIOPinSetup& Setup) { return nullptr; }
	virtual void ClearUnusedPins() {}

	//virtual void OnSetExchangeValue(const UIOExchange* Exchange, const FIOPinSourceValue& Value) {}
	//virtual void OnChangePins() {}

protected:
	virtual void OnPreActivateVehicleComponent() override;
	virtual bool OnActivateVehicleComponent() override;
	virtual void OnDeactivateVehicleComponent() override;
	virtual void DrawDebug(UCanvas* Canvas, float& YL, float& YPos) override;
};
