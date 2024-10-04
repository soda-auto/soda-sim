// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/VehicleComponents/Others/ThrottalPedal.h"
#include "Soda/Vehicles/SodaWheeledVehicle.h"
#include "Soda/VehicleComponents/VehicleInputComponent.h"

UThrottalPedalComponent::UThrottalPedalComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PostPhysics;

	GUI.Category = TEXT("IO");
	GUI.IcanName = TEXT("SodaIcons.Soda");
	GUI.ComponentNameOverride = TEXT("Throttal Pedal");
	GUI.bIsPresentInAddMenu = true;

	IOBusNodeName = "AccPedalNode";
	Pins.SetNum(2);
	Pins[0].MinVoltage = 0.9;
	Pins[0].MaxVoltage = 3.99;
	Pins[0].ExchangeName = TEXT("AccPed_ch1");
	Pins[0].PinName = TEXT("x1.1");
	Pins[0].PinFunction = TEXT("");
	Pins[0].PinDir = EIOPinDir::Output;
	Pins[1].MinVoltage = 0.5;
	Pins[1].MaxVoltage = 1.97;
	Pins[1].ExchangeName = TEXT("AccPed_ch2");
	Pins[1].PinName = TEXT("x1.1");
	Pins[1].PinFunction = TEXT("");
	Pins[1].PinDir = EIOPinDir::Output;
}

void UThrottalPedalComponent::OnPreActivateVehicleComponent()
{
	Super::OnPreActivateVehicleComponent();

	IOBus = LinkToIOBus.GetObject<UIOBusComponent>(GetOwner());
	if (IOBus)
	{
		Node = IOBus->RegisterNode(IOBusNodeName);
		check(Node);
		for (auto& Pin : Pins)
		{
			Node->RegisterPin(Pin);
		}
	}
}

bool UThrottalPedalComponent::OnActivateVehicleComponent()
{
	if (!Super::OnActivateVehicleComponent())
	{
		return false;
	}

	if (!IOBus || !Node)
	{
		SetHealth(EVehicleComponentHealth::Error, TEXT("Can't link IOBus"));
		return false;
	}

	return true;
}

void UThrottalPedalComponent::OnDeactivateVehicleComponent()
{
	Super::OnDeactivateVehicleComponent();

	if (IsValid(IOBus))
	{
		IOBus->UnregisterNode(Node);
	}
	IOBus = nullptr;
	Node = nullptr;
}

void UThrottalPedalComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!HealthIsWorkable())
	{
		return;
	}

	if (Node)
	{
		if (UVehicleInputComponent* VehicleInput = GetWheeledVehicle()->GetActiveVehicleInput())
		{
			const float Throttle = VehicleInput->GetInputState().Throttle;

			for (auto & Pin: Node->GetPins())
			{
				FIOExchangeSourceValue SourceValue;
				SourceValue.Mode = EIOExchangeMode::Analogue;
				SourceValue.LogicalVal = false;
				SourceValue.Voltage = FMath::Lerp(Pin.Setup.MinVoltage, Pin.Setup.MaxVoltage, Throttle);
				SourceValue.Frequency = 0;
				SourceValue.Duty = 0;
				Pin.Exchange->SetSourceValue(SourceValue);
			}
		}
	}
}
