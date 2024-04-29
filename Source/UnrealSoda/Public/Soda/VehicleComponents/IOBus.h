// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "Soda/VehicleComponents/VehicleBaseComponent.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "IOBus.generated.h"

class UIOBusComponent;
class UIOExchange;
class UIODevComponent;

/**
 * EIOExchangeMode
 */
UENUM(BlueprintType)
enum class EIOExchangeMode
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
enum class EIOPinDir
{
	Output,
	Input,
};


/**
 * FIOPinDescription
 */
USTRUCT(BlueprintType)
struct UNREALSODA_API FIOPinDescription
{
	GENERATED_USTRUCT_BODY()

public:
	/** Maximum 8 characters. Example: X1.20, X2.9 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = IOPinDescription, meta = (EditInRuntime))
	FName PinName;

	/** Example: HB30_OUT_A1, UIO1 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = IOPinDescription, meta = (EditInRuntime))
	FString PinFunction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = IOPinDescription, meta = (EditInRuntime))
	EIOPinDir PinDir;
};

/**
 * FIOExchangeValue
 */
USTRUCT(BlueprintType)
struct FIOExchangeValue
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(BlueprintReadWrite, Category = IOExchangeValue)
	EIOExchangeMode Mode = EIOExchangeMode::Undefined;

	UPROPERTY(BlueprintReadWrite, Category = IOExchangeValue)
	bool LogicalVal = false;

	/** [V] */
	UPROPERTY(BlueprintReadWrite, Category = IOExchangeValue)
	float Voltage = 0;

	/** [Hz] valid only if Mode=PWM */
	UPROPERTY(BlueprintReadWrite, Category = IOExchangeValue)
	float Frequency = 0;

	/** [0..1] valid only if Mode=PWM */
	UPROPERTY(BlueprintReadWrite, Category = IOExchangeValue)
	float Duty = 0;
};

/**
 * FIOPinSetup
 */
USTRUCT(BlueprintType)
struct UNREALSODA_API FIOPinSetup
{
	GENERATED_USTRUCT_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = IOPin, meta = (EditInRuntime, ReactivateComponent, AllowedClasses = "/Script/UnrealSoda.IOBusComponent"))
	FSubobjectReference LinkToIOBus;
	
	/** Connected pins must have similar  ExchangeName */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = IOPin, meta = (EditInRuntime, ReactivateComponent))
	FName ExchangeName; 
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = IOPin, meta = (EditInRuntime, ReactivateComponent))
	FIOPinDescription PinDescription;

	/** [V] */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = IOPin, meta = (EditInRuntime, ReactivateComponent))
	float MinVoltage = 0;

	/** [V] */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = IOPin, meta = (EditInRuntime, ReactivateComponent))
	float MaxVoltage = 1;

	/** [A] */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = IOPin, meta = (EditInRuntime, ReactivateComponent))
	float MaxCurrent = 1;
	
public:
	bool Bind(AActor* Owner, UIOBusComponent* IOBus = nullptr);
	void Unbind();
	bool IsBinded() const {return !!IOExchange; }
	UIOExchange * GetExchange() const { return IOExchange; }

protected:
	UPROPERTY()
	UIOExchange* IOExchange {};
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
	UIOExchange * RegistrerWeakExchange(FName ExchangeName);

	UFUNCTION(BlueprintCallable, Category = IOBus)
	bool UnregisterExchange(FName ExchangeName);

	UFUNCTION(BlueprintCallable, Category = IOBus)
	void UnregistrerAllExchanges();

	UFUNCTION(BlueprintCallable, Category = IOBus)
	const TMap<FName, UIOExchange*>& GetExchanges() const { return Exchanges; }

	virtual void RegisterIODev(UIODevComponent* IODev);
	virtual bool UnregisterIODev(UIODevComponent* IODev);

	virtual void OnSetExchangeValue(const UIOExchange* Exchange, const FIOExchangeValue& Value);
	virtual void OnRegistrerWeakExchange(UIOExchange* Exchange);
	virtual void OnUnregisterExchange(UIOExchange* Exchange);
	virtual void OnAddPin(UIOExchange* Exchange, const FIOPinDescription & PinDesc);
	virtual void OnRemovePin(UIOExchange* Exchange, const FIOPinDescription& PinDesc);

protected:
	virtual void OnPreActivateVehicleComponent() override;
	virtual bool OnActivateVehicleComponent() override;
	virtual void OnDeactivateVehicleComponent() override;
	virtual void DrawDebug(UCanvas* Canvas, float& YL, float& YPos) override;

protected:
	TMap<FName, UIOExchange*> Exchanges;

	UPROPERTY()
	TSet<UIODevComponent*> RegistredIODevs;
};

/**
 * UIOExchange
 */
UCLASS(ClassGroup = Soda, BlueprintType, Blueprintable)
class UNREALSODA_API UIOExchange : public UObject
{
	friend UIOBusComponent;

	GENERATED_UCLASS_BODY()

public:

	UFUNCTION(BlueprintCallable, Category = IOExchange)
	void SetValue(const FIOExchangeValue& Value);

	UFUNCTION(BlueprintCallable, Category = IOExchange)
	const FIOExchangeValue& GetValue() const { return IOExchangeValue; }

	/** [A] */
	UFUNCTION(BlueprintCallable, Category = IOExchange)
	void SetMeasuredCurrent(float Current, bool bValid);

	/** [V] */
	UFUNCTION(BlueprintCallable, Category = IOExchange)
	void SetMeasuredVoltage(float Voltage, bool bValid);

	UFUNCTION(BlueprintCallable, Category = IOExchange)
	float GetMeasuredCurrent() const { return MeasuredCurrent.Get(0); }

	UFUNCTION(BlueprintCallable, Category = IOExchange)
	float GetMeasuredVoltage() const { return MeasuredVoltage.Get(0); }

	UFUNCTION(BlueprintCallable, Category = IOExchange)
	bool IsMeasuredCurrentValid() const { return MeasuredCurrent.IsSet(); }

	UFUNCTION(BlueprintCallable, Category = IOExchange)
	bool IsMeasuredVoltageValid() const { return MeasuredVoltage.IsSet(); }

	UFUNCTION(BlueprintCallable, Category = IOExchange)
	UIOBusComponent* GetIOBus() const { return IOBus; }

	UFUNCTION(BlueprintCallable, Category = IOExchange)
	FName GetExchangeName() const { return ExchangeName; }

	UFUNCTION(BlueprintCallable, Category = IOExchange)
	bool IsIOExchangeRegistred() const { return bIOExchangeIsRegistred; }

	UFUNCTION(BlueprintCallable, Category = IOExchange)
	void AddPin(const FIOPinDescription& Desc);

	UFUNCTION(BlueprintCallable, Category = IOExchange)
	bool RemovePin(FName PinName);

	UFUNCTION(BlueprintCallable, Category = IOExchange)
	const TMap<FName, FIOPinDescription> & GetPins() const { return PinDescriptions; }

protected:
	virtual void BeginDestroy() override;

protected:
	virtual void RigisterExchange(UIOBusComponent* IOBus, FName ExchangeName);
	virtual void UnregisterExchange();

protected:
	FName ExchangeName;
	UPROPERTY()
	UIOBusComponent * IOBus = nullptr;
	TTimestamp TimestampValue{};
	TTimestamp TimestampMeasuredCurrent{};
	TTimestamp TimestampMeasuredVoltage{};
	TOptional<float> MeasuredCurrent;
	TOptional<float> MeasuredVoltage;
	FIOExchangeValue IOExchangeValue;
	bool bIOExchangeIsRegistred = false;

	TMap<FName, FIOPinDescription> PinDescriptions;
};


/**
 * UIOBusFunctionLibrary
 */
UCLASS(Category = Soda)
class UNREALSODA_API UIOBusFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_UCLASS_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = IOBus)
	static bool Bind(UPARAM(ref) FIOPinSetup& Pin, AActor* Owner) { return Pin.Bind(Owner); }

	UFUNCTION(BlueprintCallable, Category = IOBus)
	static void Unbind(UPARAM(ref) FIOPinSetup& Pin) { return Pin.Unbind(); }

	UFUNCTION(BlueprintCallable, Category = IOBus)
	static bool IsBinded(const FIOPinSetup& Pin) { return Pin.IsBinded(); }

	UFUNCTION(BlueprintCallable, Category = IOBus)
	static UIOExchange* GetExchange(const FIOPinSetup& Pin) { return Pin.GetExchange(); }
};
 