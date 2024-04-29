// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/VehicleComponents/IOBus.h"
#include "Soda/VehicleComponents/IODev.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"

UIOExchange::UIOExchange(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UIOExchange::RigisterExchange(UIOBusComponent* InIOBus, FName InExchangeName)
{
	check(IsValid(InIOBus));

	if (InExchangeName.IsNone())
	{
		return;
	}

	IOBus = InIOBus;
	ExchangeName = InExchangeName;
	bIOExchangeIsRegistred = true;
}

void UIOExchange::UnregisterExchange()
{
	auto* IOBusCpy = IOBus;
	auto ExchangeNameCpy = ExchangeName;

	IOBus = nullptr;
	ExchangeName = NAME_None;
	bIOExchangeIsRegistred = false;

	if (IsValid(IOBusCpy) && !ExchangeNameCpy.IsNone())
	{
		IOBusCpy->UnregisterExchange(ExchangeNameCpy);
	}
}

void UIOExchange::SetValue(const FIOExchangeValue& Value)
{
	IOExchangeValue = Value;
	if (IOBus) IOBus->OnSetExchangeValue(this, Value);
	//TimestampValue = 
}

void UIOExchange::SetMeasuredCurrent(float Current, bool bValid)
{
	if (bValid)
	{
		MeasuredCurrent = Current;
		//TimestampMeasuredCurrent = 
	}
	else
	{
		MeasuredCurrent.Reset();
	}

}

void UIOExchange::SetMeasuredVoltage(float Voltage, bool bValid)
{
	if (bValid)
	{
		MeasuredVoltage = Voltage;
		//TimestampMeasuredVoltage = 
	}
	else
	{
		MeasuredVoltage.Reset();
	}
}

void UIOExchange::AddPin(const FIOPinDescription& Desc)
{ 
	if (!Desc.PinName.IsNone())
	{
		PinDescriptions.FindOrAdd(Desc.PinName) = Desc;
		if (IsValid(IOBus))
		{
			IOBus->OnAddPin(this, Desc);
		}
	}
}

bool UIOExchange::RemovePin(FName PinName)
{ 
	FIOPinDescription Desc;
	if (PinDescriptions.RemoveAndCopyValue(PinName, Desc))
	{
		if (IsValid(IOBus))
		{
			IOBus->OnRemovePin(this, Desc);
		}
		return true;
	}

	return false;
}

void UIOExchange::BeginDestroy()
{
	Super::BeginDestroy();
	UnregisterExchange();
}

//----------------------------------------------------------------------------
bool FIOPinSetup::Bind(AActor* Owner, UIOBusComponent* IOBus)
{
	if (!IOBus)
	{
		IOBus = LinkToIOBus.GetObject<UIOBusComponent>(Owner);
	}
	if (!IOBus)
	{
		return false;
	}

	IOExchange = IOBus->RegistrerWeakExchange(ExchangeName);
	check(IOExchange);

	IOExchange->AddPin(PinDescription);

	return true;
}

void FIOPinSetup::Unbind()
{
	if (IsValid(IOExchange) && IsValid(IOExchange->GetIOBus()))
	{
		IOExchange->RemovePin(PinDescription.PinName);
		if (IOExchange->GetPins().Num() == 0)
		{
			IOExchange->GetIOBus()->UnregisterExchange(IOExchange->GetExchangeName());
		}
	}
	IOExchange = nullptr;
}

//----------------------------------------------------------------------------
UIOBusComponent::UIOBusComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	GUI.Category = TEXT("IO");
	GUI.IcanName = TEXT("SodaIcons.Net");
	GUI.ComponentNameOverride = TEXT("IO Bus");
	GUI.bIsPresentInAddMenu = true;

	Common.bIsTopologyComponent = true;
	Common.Activation = EVehicleComponentActivation::OnStartScenario;
}

void UIOBusComponent::OnPreActivateVehicleComponent()
{
	Super::OnPreActivateVehicleComponent();

}

bool UIOBusComponent::OnActivateVehicleComponent()
{
	bool bRet = Super::OnActivateVehicleComponent();

	return bRet;
}

void UIOBusComponent::OnDeactivateVehicleComponent()
{
	Super::OnDeactivateVehicleComponent();

	//UnregistrerAllPins();
}

UIOExchange* UIOBusComponent::RegistrerWeakExchange(FName ExchangeName)
{
	if (ExchangeName.IsNone())
	{
		return nullptr;
	}
	if (auto Found = Exchanges.Find(ExchangeName))
	{
		return *Found;
	}

	UIOExchange * Exchange = NewObject<UIOExchange>(this);
	check(Exchange);
	Exchange->RigisterExchange(this, ExchangeName);
	Exchanges.Add(ExchangeName, Exchange);
	OnRegistrerWeakExchange(Exchange);
	return Exchange;
}

bool UIOBusComponent::UnregisterExchange(FName ExchangeName)
{
	UIOExchange* Exchange = nullptr;
	if (Exchanges.RemoveAndCopyValue(ExchangeName, Exchange))
	{
		Exchange->UnregisterExchange();
		OnUnregisterExchange(Exchange);
	}
	return Exchanges.Remove(ExchangeName) > 0;
}

void UIOBusComponent::UnregistrerAllExchanges()
{
	for (auto& It : Exchanges)
	{
		It.Value->UnregisterExchange();
	}
	Exchanges.Empty();
}

void UIOBusComponent::OnSetExchangeValue(const UIOExchange* Exchange, const FIOExchangeValue& Value)
{
	for (auto & It: RegistredIODevs)
	{
		It->OnSetExchangeValue(Exchange, Value);
	}
}

void UIOBusComponent::OnRegistrerWeakExchange(UIOExchange* Exchange)
{
	for (auto& It : RegistredIODevs)
	{
		It->OnRegistrerWeakExchange(Exchange);
	}
}

void UIOBusComponent::OnUnregisterExchange(UIOExchange* Exchange)
{
	for (auto& It : RegistredIODevs)
	{
		It->OnUnregisterExchange(Exchange);
	}
}

void UIOBusComponent::OnAddPin(UIOExchange* Exchange, const FIOPinDescription& PinDesc)
{
	for (auto& It : RegistredIODevs)
	{
		It->OnAddPin(Exchange, PinDesc);
	}
}

void UIOBusComponent::OnRemovePin(UIOExchange* Exchange, const FIOPinDescription& PinDesc)
{
	for (auto& It : RegistredIODevs)
	{
		It->OnRemovePin(Exchange, PinDesc);
	}
}

void UIOBusComponent::RegisterIODev(UIODevComponent* IODev)
{
	if (IsValid(IODev))
	{
		RegistredIODevs.Add(IODev);
	}
}

bool UIOBusComponent::UnregisterIODev(UIODevComponent* IODev)
{
	if (IsValid(IODev))
	{
		return RegistredIODevs.Remove(IODev) > 0;
	}
	return false;
}

void UIOBusComponent::DrawDebug(UCanvas* Canvas, float& YL, float& YPos)
{
	Super::DrawDebug(Canvas, YL, YPos);

	static const wchar_t * IOExchangeModeToStr[4] =
	{
		TEXT("Undefined"),
		TEXT("Fixed"),
		TEXT("Analogue"),
		TEXT("PWM")
	};

	if (Common.bDrawDebugCanvas)
	{
		UFont* RenderFont = GEngine->GetSmallFont();
		Canvas->SetDrawColor(FColor::White);

		for (const auto& Exchange : Exchanges)
		{
			YPos += Canvas->DrawText(RenderFont, FString::Printf(TEXT("Exchange: %s, mode: %s"), 
				*Exchange.Value->GetExchangeName().ToString(),
				IOExchangeModeToStr[int(Exchange.Value->GetValue().Mode)]),
				16, YPos);
			YPos += Canvas->DrawText(RenderFont, FString::Printf(TEXT("   Value: [%i] [%.1fV] [%.1fHz %f.1f]"),
				Exchange.Value->GetValue().LogicalVal,
				Exchange.Value->GetValue().Voltage,
				Exchange.Value->GetValue().Frequency,
				Exchange.Value->GetValue().Duty),
				16, YPos);
			YPos += Canvas->DrawText(RenderFont, FString::Printf(TEXT("   Measured: %.1fA[%i] %.1V[%i]"),
				Exchange.Value->GetMeasuredCurrent(),
				Exchange.Value->IsMeasuredCurrentValid(),
				Exchange.Value->GetMeasuredVoltage(),
				Exchange.Value->IsMeasuredVoltageValid()),
				16, YPos);
		}

		//YPos += Canvas->DrawText(RenderFont, FString::Printf(TEXT("Registred send msgs: %d"), RecvMessages.size()), 16, YPos);
		//YPos += Canvas->DrawText(RenderFont, FString::Printf(TEXT("Registred recv msgs: %d"), SendMessages.size()), 16, YPos);
		//YPos += Canvas->DrawText(RenderFont, FString::Printf(TEXT("PkgSent: %d"), PkgSent), 16, YPos);
		//YPos += Canvas->DrawText(RenderFont, FString::Printf(TEXT("PkgReceived: %d"), PkgReceived), 16, YPos);
		//YPos += Canvas->DrawText(RenderFont, FString::Printf(TEXT("PkgSentErr: %d"), PkgSentErr), 16, YPos);
		//YPos += Canvas->DrawText(RenderFont, FString::Printf(TEXT("PkgDecoded: %d"), PkgDecoded), 16, YPos);
	}
}

//----------------------------------------------------------------------------
UIOBusFunctionLibrary::UIOBusFunctionLibrary(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}