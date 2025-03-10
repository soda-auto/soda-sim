// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.
#include "Soda/VehicleComponents/IOBus.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"

UIOPin::UIOPin(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{}

UIOBusComponent::UIOBusComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	GUI.Category = TEXT("IO");
	GUI.IcanName = TEXT("SodaIcons.Net");
	//GUI.ComponentNameOverride = TEXT("IO Dev");
	//GUI.bIsPresentInAddMenu = true;

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

}

void UIOBusComponent::DrawDebug(UCanvas* Canvas, float& YL, float& YPos)
{
	Super::DrawDebug(Canvas, YL, YPos);

	/*
	
	static const FString::ElementType* IOExchangeModeToStr[4] =
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
	*/
}

