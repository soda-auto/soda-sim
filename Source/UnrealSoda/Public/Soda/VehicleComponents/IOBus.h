// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "Soda/VehicleComponents/VehicleBaseComponent.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "IOBus.generated.h"

class UIOExchange;
class UIODevComponent;

/**
 * EIOExchangeMode
 */
UENUM(BlueprintType)
enum class EIOExchangeMode: uint8
{
    Undefined,
    Fixed,
    Analogue,
    PWM,
};

/**
 * EIOPinDir
 */
UENUM(BlueprintType)
enum class EIOPinDir : uint8
{
	Output,
	Input,
};

/**
 * FIOExchangeSourceValue
 */
USTRUCT(BlueprintType)
struct FIOExchangeSourceValue
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(BlueprintReadWrite, Category = SourceValue)
	EIOExchangeMode Mode = EIOExchangeMode::Undefined;

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
 * FIOExchangeSourceValue
 */
USTRUCT(BlueprintType)
struct FIOExchangeFeedbackValue
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
 * FIOPinDescription
 */
USTRUCT(BlueprintType)
struct UNREALSODA_API FIOPinSetup
{
	GENERATED_USTRUCT_BODY()

public:
	/** Example: Pedl_Ch1, Whl_Enc_FL */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = IOPin, meta = (EditInRuntime, ReactivateComponent))
	FName ExchangeName;

	/** Example: X1.20, X2.9 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = IOPinDescription, meta = (EditInRuntime))
	FName PinName;

	/** Example: HB30_OUT_A1, UIO1 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = IOPinDescription, meta = (EditInRuntime))
	FName PinFunction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = IOPinDescription, meta = (EditInRuntime))
	EIOPinDir PinDir{};

	/** [V] */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = IOPinDescription, meta = (EditInRuntime))
	float MinVoltage = 0;

	/** [V] */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = IOPinDescription, meta = (EditInRuntime))
	float MaxVoltage = 1;

	/** [A] */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = IOPinDescription, meta = (EditInRuntime))
	float MaxCurrent = 1;
};

/**
 * FIOPin
 */
USTRUCT(BlueprintType)
struct UNREALSODA_API FIOPin
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(BlueprintReadOnly, Category = IOPin)
	FIOPinSetup Setup {};

	UPROPERTY(BlueprintReadOnly, Category = IOPin)
	UIOExchange* Exchange {};
};

/**
 * UIOBusNode
 */
UCLASS(BlueprintType)
class UNREALSODA_API UIOBusNode: public UObject
{
	GENERATED_UCLASS_BODY()

public:
	void Setup(UIOBusComponent * Bus, FName NodeName);

	UFUNCTION(BlueprintCallable, Category = IOBus)
	FName GetNodeName() { return NodeName; }

	UFUNCTION(BlueprintCallable, Category = IOBus)
	UIOBusComponent* GetBus() const { return BusComponent; }

	UFUNCTION(BlueprintCallable, Category = IOBus)
	UIOExchange * RegisterPin(const FIOPinSetup& PinSetup);

	UFUNCTION(BlueprintCallable, Category = IOBus)
	bool UnregisterPinByExchange(UIOExchange * Exchange);

	UFUNCTION(BlueprintCallable, Category = IOBus)
	bool UnregisterPinByExchangeName(FName ExchangeName);

	UFUNCTION(BlueprintCallable, Category = IOBus)
	bool UnregisterPinByPinName(FName PinName);

	UFUNCTION(BlueprintCallable, Category = IOBus)
	bool UnregisterPinByPinFunction(FName PinFunction);

	UFUNCTION(BlueprintCallable, Category = IOBus)
	const TArray<FIOPin>& GetPins() const { return Pins; }

	UFUNCTION(BlueprintCallable, Category = IOBus)
	UIOExchange* FindExchange(FName ExchangeName) const;

private:
	FName NodeName;

	UPROPERTY()
	TArray<FIOPin> Pins;

	UPROPERTY()
	UIOBusComponent* BusComponent{};
};


/**
 * UIOBusComponent
 */
UCLASS(ClassGroup = Soda, BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class UNREALSODA_API UIOBusComponent : public UVehicleBaseComponent
{
	GENERATED_UCLASS_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = IOBus)
	virtual UIOBusNode* RegisterNode(FName NodeName);

	UFUNCTION(BlueprintCallable, Category = IOBus)
	virtual bool UnregisterNodeByName(FName NodeName);

	UFUNCTION(BlueprintCallable, Category = IOBus)
	virtual bool UnregisterNode(const UIOBusNode * Node);

	const TMap<FName, UIOBusNode*>& GetNodes() const { return Nodes; }

	UFUNCTION(BlueprintCallable, Category = IOBus)
	UIOExchange* FindExchange(FName ExchaneName);


	virtual void RegisterIODev(UIODevComponent* IODev);
	virtual bool UnregisterIODev(UIODevComponent* IODev);

	//virtual void OnSetExchangeValue(const UIOExchange* Exchange, const FIOExchangeSourceValue& Value);
	virtual void OnChangePins();

protected:
	virtual void OnPreActivateVehicleComponent() override;
	virtual bool OnActivateVehicleComponent() override;
	virtual void OnDeactivateVehicleComponent() override;
	virtual void DrawDebug(UCanvas* Canvas, float& YL, float& YPos) override;

protected:
	UPROPERTY()
	TMap<FName, UIOBusNode*> Nodes;

	UPROPERTY()
	TSet<UIODevComponent*> RegistredIODevs;
};

/**
 * UIOExchange
 */
UCLASS(ClassGroup = Soda, BlueprintType, Blueprintable)
class UNREALSODA_API UIOExchange : public UObject
{
	friend UIOBusNode;

	GENERATED_UCLASS_BODY()

public:

	UFUNCTION(BlueprintCallable, Category = IOExchange)
	void SetSourceValue(const FIOExchangeSourceValue& Value);

	UFUNCTION(BlueprintCallable, Category = IOExchange)
	void SetFeedbackValue(const FIOExchangeFeedbackValue& Value);

	UFUNCTION(BlueprintCallable, Category = IOExchange)
	const FIOExchangeSourceValue& GetSourceValue() const { return SourceValue; }

	UFUNCTION(BlueprintCallable, Category = IOExchange)
	const FIOExchangeFeedbackValue& GetFeedbackValue() const { return FeedbackValue; }

	//FIOExchangeSourceValue& GetValue() { return SourceValue; }

	UFUNCTION(BlueprintCallable, Category = IOExchange)
	UIOBusComponent* GetIOBus() const { return IOBus; }

	UFUNCTION(BlueprintCallable, Category = IOExchange)
	FName GetExchangeName() const { return ExchangeName; }

	UFUNCTION(BlueprintCallable, Category = IOExchange)
	bool IsIOExchangeRegistred() const { return !!IOBus; }

protected:
	virtual void BeginDestroy() override;

protected:
	virtual void RigisterExchange(UIOBusComponent* IOBus, FName ExchangeName);
	virtual void UnregisterExchange();

protected:
	FName ExchangeName;
	UPROPERTY()
	UIOBusComponent * IOBus = nullptr;
	TTimestamp SetSourceValue_Ts{};
	TTimestamp SetFeedbackValue_Ts{};

	FIOExchangeSourceValue SourceValue;
	FIOExchangeFeedbackValue FeedbackValue;

};


/**
 * UIOBusFunctionLibrary
 */
/*
UCLASS(Category = Soda)
class UNREALSODA_API UIOBusFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_UCLASS_BODY()

public:

};
*/
 