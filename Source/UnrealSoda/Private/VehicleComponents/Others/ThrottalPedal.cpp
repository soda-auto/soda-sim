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

	Channel1.MinVoltage = 0.9;
	Channel1.MaxVoltage = 3.99;
	Channel1.ExchangeName = TEXT("AccPed_ch1");
	Channel1.PinDescription.PinName = TEXT("AccPed_1");
	Channel1.PinDescription.PinFunction = TEXT("AccPed_ch1");
	Channel1.PinDescription.PinDir = EIOPinDir::Output;

	Channel2.MinVoltage = 0.5;
	Channel2.MaxVoltage = 1.97;
	Channel2.ExchangeName = TEXT("AccPed_ch2");
	Channel2.PinDescription.PinName = TEXT("AccPed_2");
	Channel2.PinDescription.PinFunction = TEXT("AccPed_ch2");
	Channel2.PinDescription.PinDir = EIOPinDir::Output;
}

void UThrottalPedalComponent::OnPreActivateVehicleComponent()
{
	Super::OnPreActivateVehicleComponent();

	IOBus = LinkToIOBus.GetObject<UIOBusComponent>(GetOwner());
	if (IOBus)
	{
		Channel1.Bind(GetOwner(), IOBus);
		Channel2.Bind(GetOwner(), IOBus);
	}
}

bool UThrottalPedalComponent::OnActivateVehicleComponent()
{
	if (!Super::OnActivateVehicleComponent())
	{
		return false;
	}

	if (!IOBus)
	{
		SetHealth(EVehicleComponentHealth::Error, TEXT("Can't link IOBus"));
		return false;
	}

	if (!Channel1.IsBinded())
	{
		SetHealth(EVehicleComponentHealth::Error, TEXT("Can't bind Channel1"));
		return false;
	}

	if (!Channel2.IsBinded())
	{
		SetHealth(EVehicleComponentHealth::Error, TEXT("Can't bind Channel2"));
		return false;
	}

	return true;
}

void UThrottalPedalComponent::OnDeactivateVehicleComponent()
{
	Super::OnDeactivateVehicleComponent();

	Channel1.Unbind();
	Channel2.Unbind();

	IOBus = nullptr;
}

void UThrottalPedalComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!HealthIsWorkable())
	{
		return;
	}

	if (UVehicleInputComponent* VehicleInput = GetWheeledVehicle()->GetActiveVehicleInput())
	{
		const float Throttle = VehicleInput->GetInputState().Throttle;

		if (UIOExchange* Exchange = Channel1.GetExchange())
		{
			FIOExchangeValue Value;
			Value.Mode = EIOExchangeMode::Analogue;
			Value.LogicalVal = false;
			Value.Voltage = FMath::Lerp(Channel1.MinVoltage, Channel1.MaxVoltage, Throttle);
			Value.Frequency = 0;
			Value.Duty = 0;
			Exchange->SetValue(Value);
		}

		if (UIOExchange* Exchange = Channel2.GetExchange())
		{
			FIOExchangeValue Value;
			Value.Mode = EIOExchangeMode::Analogue;
			Value.LogicalVal = false;
			Value.Voltage = FMath::Lerp(Channel2.MinVoltage, Channel2.MaxVoltage, Throttle);
			Value.Frequency = 0;
			Value.Duty = 0;
			Exchange->SetValue(Value);
		}
	}
}
