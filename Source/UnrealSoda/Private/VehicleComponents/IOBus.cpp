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
}

void UIOExchange::UnregisterExchange()
{
	if (IOBus)
	{
		IOBus = nullptr;
		ExchangeName = NAME_None;
	}
}

void UIOExchange::SetSourceValue(const FIOExchangeSourceValue& Value)
{
	SourceValue = Value;
	//if (IOBus) IOBus->OnSetExchangeValue(this, Value);
	//SetSourceValue_Ts = 
}

void UIOExchange::SetFeedbackValue(const FIOExchangeFeedbackValue& Value)
{
	FeedbackValue = Value;
	//SetFeedbackValue_Ts = 
}

void UIOExchange::BeginDestroy()
{
	Super::BeginDestroy();
	UnregisterExchange();
}

//----------------------------------------------------------------------------
UIOBusNode::UIOBusNode(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UIOBusNode::Setup(UIOBusComponent* InBus, FName InNodeName)
{
	check(IsValid(InBus));
	check(!InNodeName.IsNone());

	BusComponent = InBus;
	NodeName = InNodeName;
}

UIOExchange* UIOBusNode::RegisterPin(const FIOPinSetup& PinSetup)
{
	if (BusComponent)
	{
		UIOExchange* Exchange = BusComponent->FindExchange(PinSetup.ExchangeName);
		if (!Exchange)
		{
			Exchange = NewObject<UIOExchange>(BusComponent);
			check(Exchange);
			Exchange->RigisterExchange(BusComponent, PinSetup.ExchangeName);
		}
		Pins.Add(FIOPin{ PinSetup , Exchange });
		return Exchange;
	}
	else
	{
		return nullptr;
	}
}

bool UIOBusNode::UnregisterPinByExchange(UIOExchange* Exchange)
{
	return Pins.RemoveAllSwap([&Exchange](auto& It) { return IsValid(It.Exchange) && It.Exchange == Exchange; }) > 0;
}

bool UIOBusNode::UnregisterPinByExchangeName(FName ExchangeName)
{
	return Pins.RemoveAllSwap([&ExchangeName](auto& It) { return IsValid(It.Exchange) && It.Exchange->GetExchangeName() == ExchangeName; }) > 0;
}

bool UIOBusNode::UnregisterPinByPinName(FName PinName)
{
	return Pins.RemoveAllSwap([&PinName](auto& It) { return It.Setup.PinName == PinName; }) > 0;
}

bool UIOBusNode::UnregisterPinByPinFunction(FName PinFunction)
{
	return Pins.RemoveAllSwap([&PinFunction](auto& It) { return It.Setup.PinFunction == PinFunction; }) > 0;
}

UIOExchange* UIOBusNode::FindExchange(FName ExchangeName) const
{
	if (auto Exchange = Pins.FindByPredicate([&ExchangeName](auto& It) { return IsValid(It.Exchange) && It.Exchange->GetExchangeName() == ExchangeName; }))
	{
		return Exchange->Exchange;
	}
	else
	{
		return nullptr;
	}
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

UIOBusNode* UIOBusComponent::RegisterNode(FName NodeName)
{

	if (UIOBusNode** FoundNode = Nodes.Find(NodeName))
	{
		return *FoundNode;
	}
	else
	{
		UIOBusNode* Node = NewObject<UIOBusNode>(this);
		Node->Setup(this, NodeName);
		Nodes.Add(NodeName, Node);
		return Node;
	}

}

bool UIOBusComponent::UnregisterNodeByName(FName NodeName)
{
	return Nodes.Remove(NodeName) > 0;
}

bool UIOBusComponent::UnregisterNode(const UIOBusNode* Node)
{
	bool bRet = false;
	for (auto It = Nodes.CreateIterator(); It; ++It)
	{
		if (It.Value() == Node)
		{
			It.RemoveCurrent();
			bRet = true;
		}
	}
	return bRet;
}

UIOExchange* UIOBusComponent::FindExchange(FName ExchaneName)
{
	for (auto & Node : Nodes)
	{
		if (UIOExchange* Found = Node.Value->FindExchange(ExchaneName))
		{
			return Found;
		}
	}
	return nullptr;
}

/*
void UIOBusComponent::OnSetExchangeValue(const UIOExchange* Exchange, const FIOExchangeSourceValue& Value)
{
	for (auto & It: RegistredIODevs)
	{
		It->OnSetExchangeValue(Exchange, Value);
	}
}
*/

void UIOBusComponent::OnChangePins()
{
	for (auto& It : RegistredIODevs)
	{
		It->OnChangePins();
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

		for (const auto& Node: Nodes)
		{
			for (const auto& Pin : Node.Value->GetPins())
			{
				auto Exchange = Pin.Exchange;
				YPos += Canvas->DrawText(RenderFont, FString::Printf(TEXT("Exchange: %s, mode: %s"),
					*Exchange->GetExchangeName().ToString(),
					IOExchangeModeToStr[int(Exchange->GetSourceValue().Mode)]),
					16, YPos);
				YPos += Canvas->DrawText(RenderFont, FString::Printf(TEXT("   Value: [%i] [%.1fV] [%.1fHz %f.1f]"),
					Exchange->GetSourceValue().LogicalVal,
					Exchange->GetSourceValue().Voltage,
					Exchange->GetSourceValue().Frequency,
					Exchange->GetSourceValue().Duty),
					16, YPos);
				YPos += Canvas->DrawText(RenderFont, FString::Printf(TEXT("   Measured: %.1fA[%i] %.1V[%i]"),
					Exchange->GetFeedbackValue().MeasuredCurrent,
					Exchange->GetFeedbackValue().bIsMeasuredCurrentValid,
					Exchange->GetFeedbackValue().MeasuredVoltage,
					Exchange->GetFeedbackValue().bIsMeasuredVoltageValid),
					16, YPos);
			}
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
/*
UIOBusFunctionLibrary::UIOBusFunctionLibrary(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}
*/