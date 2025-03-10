// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/VehicleComponents/Others/OutputButtons.h"
#include "Soda/Vehicles/SodaWheeledVehicle.h"
#include "Soda/VehicleComponents/VehicleInputComponent.h"

UOutputButtonsComponent::UOutputButtonsComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PostPhysics;

	GUI.Category = TEXT("IO");
	GUI.IcanName = TEXT("SodaIcons.Soda");
	GUI.ComponentNameOverride = TEXT("IO Button");
	GUI.bIsPresentInAddMenu = true;
}

void UOutputButtonsComponent::OnPreActivateVehicleComponent()
{
	Super::OnPreActivateVehicleComponent();

	IOBus = LinkToIOBus.GetObject<UIOBusComponent>(GetOwner());
	if (IOBus)
	{
		for (auto& Setup : ButtonSetups)
		{
			auto & Button = Buttons.Add_GetRef({});
			Button.Setup = Setup;
			Button.PinInterface = TStrongObjectPtr<UIOPin>(IOBus->CreatePin(Button.Setup.Pin));
			
		}
	}
}

bool UOutputButtonsComponent::OnActivateVehicleComponent()
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

	return true;
}

void UOutputButtonsComponent::OnDeactivateVehicleComponent()
{
	Super::OnDeactivateVehicleComponent();

	Buttons.Reset();
	IOBus = nullptr;
}

void UOutputButtonsComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!HealthIsWorkable())
	{
		return;
	}

	if (APlayerController* PlayerController = GetWorld()->GetFirstPlayerController())
	{
		for (auto& Button : Buttons)
		{
			if (Button.PinInterface)
			{
				Button.Voltage = Button.Setup.ReleasedVoltage;
				for (auto& Item : Button.Setup.ButtonItmes)
				{
					if (Item.SwitchMode == EIOButtonSwitchMode::Momentary)
					{
						if (PlayerController->IsInputKeyDown(Item.Key))
						{
							Button.bIsPressed = true;
							Button.Voltage = Item.Voltage;
							break;
						}
						else
						{
							Button.bIsPressed = false;
						}
					}
					else if (Item.SwitchMode == EIOButtonSwitchMode::Position)
					{
						if (PlayerController->WasInputKeyJustPressed(Item.Key))
						{
							Button.bIsPressed = !Button.bIsPressed;
							if (Button.bIsPressed)
							{
								Button.Voltage = Item.Voltage;
							}
							break;
						}
					}
					else // Item.SwitchMode == EIOButtonSwitchMode::Analogue
					{

					}
				}
				FIOPinSourceValue SourceValue;
				SourceValue.Mode = EIOPinMode::Analogue;
				SourceValue.LogicalVal = Button.bIsPressed;
				SourceValue.Voltage = Button.Voltage;
				SourceValue.Frequency = 0;
				SourceValue.Duty = 0;
				Button.PinInterface->PublishSource(SourceValue);
			}
		}
	}
}
