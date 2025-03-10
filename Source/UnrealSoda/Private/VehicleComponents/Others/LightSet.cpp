// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.


#include "Soda/VehicleComponents/Others/LightSet.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Components/LightComponent.h"

using namespace std::chrono_literals;

UVehicleLightItem::UVehicleLightItem(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}
void UVehicleLightItem::Setup(const FVehicleLightItemSetup& InItemSetup)
{
	ItemSetup = InItemSetup;
}

bool UVehicleLightItem::SetupIO(UIOBusComponent* IOBus)
{
	if (ItemSetup.bUseIO)
	{
		PinInterface = IOBus->CreatePin(ItemSetup.IOPinSetup);
	}
	return !!PinInterface;
}

void UVehicleLightItem::SetEnabled( bool bNewEnabled, float Intensity)
{
	bEnabled = bNewEnabled;
	for (auto& It : ItemSetup.Materials)
	{
		if(IsValid(It)) It->SetScalarParameterValue(GetSetup().MaterialPropertyName, bEnabled ? Intensity * ItemSetup.IntensityMultiplayer : 0.0);
	}

	for (auto& It : ItemSetup.Lights)
	{
		if (IsValid(It)) It->SetVisibility(bEnabled);
	}

	if (IsValid(PinInterface) && ItemSetup.bSendIOFeedback)
	{
		float Voltage = PinInterface->GetInputSourceValue().Voltage;
		FIOPinFeedbackValue FeedbackValue;
		FeedbackValue.MeasuredCurrent = Voltage * FMath::Clamp(Intensity, 0.f, 1.f) * ItemSetup.Resistanse;
		FeedbackValue.MeasuredVoltage = Voltage;
		FeedbackValue.bIsMeasuredCurrentValid = true;
		FeedbackValue.bIsMeasuredVoltageValid = true;
		PinInterface->PublishFeedback(FeedbackValue);
	}
}

//---------------------------------------------------------------------------------------
ULightSetComponent::ULightSetComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PostPhysics;
}

void ULightSetComponent::SetEnabledAllLights(bool bEnabled)
{
	for (auto& It : LightItems)
	{
		It.Value->SetEnabled(bEnabled, bEnabled ? 1.0: 0.0);
	}
}

void ULightSetComponent::OnPreActivateVehicleComponent()
{
	Super::OnPreActivateVehicleComponent();

	AddLightItems(DefaultLightSetups);

	IOBus = LinkToIOBus.GetObject<UIOBusComponent>(GetOwner());
	if (IOBus)
	{
		for (auto& Itme : LightItems)
		{
			Itme.Value->SetupIO(IOBus);
		}
	}
}

bool ULightSetComponent::OnActivateVehicleComponent()
{
	if (!Super::OnActivateVehicleComponent())
	{
		return false;
	}

	return true;
}

void ULightSetComponent::OnDeactivateVehicleComponent()
{
	Super::OnDeactivateVehicleComponent();

	IOBus = nullptr;
	LightItems.Reset();
}

UVehicleLightItem* ULightSetComponent::AddLightItem(const FVehicleLightItemSetup& InSetup)
{
	FVehicleLightItemSetup Setup = InSetup;

	if (FVehicleLightItemSetupHelper* Found = LightItemSetupHelper.Find(Setup.LightName))
	{
		Setup.Materials = Found->Materials;
		Setup.Lights = Found->Lights;
	}

	if (UVehicleLightItem** Found = LightItems.Find(Setup.LightName))
	{
		(*Found)->Setup(Setup);
		return *Found;
	}
	else
	{
		UVehicleLightItem* LightItem = NewObject<UVehicleLightItem>(this);
		LightItem->Setup(Setup);
		LightItems.Add(Setup.LightName, LightItem);
		return LightItem;
	}
}

void ULightSetComponent::AddLightItems(const TArray<FVehicleLightItemSetup>& Setups)
{
	for (auto& Setup : Setups)
	{
		AddLightItem(Setup);
	}
}

bool ULightSetComponent::RemoveLightItemByName(const FName& LightName)
{
	return LightItems.Remove(LightName) > 0;
}

bool ULightSetComponent::RemoveLightItem(const UVehicleLightItem* LightItem)
{
	bool bRet = false;
	for (auto It = LightItems.CreateIterator(); It; ++It)
	{
		if (It.Value() == LightItem)
		{
			It.RemoveCurrent();
			bRet = true;
		}
	}
	return bRet;
}

void ULightSetComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bUseIOs)
	{
		auto Now = soda::Now();
		for (auto& LightItem : LightItems)
		{
			UIOPin* PinInterface = LightItem.Value->GetPinInterface();
			const FVehicleLightItemSetup& LightSetup = LightItem.Value->GetSetup();

			if (PinInterface && LightSetup.bUseIO)
			{
				FTimestamp ValueTS;
				auto Value = PinInterface->GetInputSourceValue(ValueTS);
				float Coef = 0;
				float Voltage = 0;
				if (Now - *ValueTS < 500ms)
				{
					switch (Value.Mode)
					{
					case EIOPinMode::Fixed:
					case EIOPinMode::Analogue:
						Coef = (Value.Voltage - LightSetup.IOPinSetup.MinVoltage) / (LightSetup.IOPinSetup.MaxVoltage - LightSetup.IOPinSetup.MinVoltage);
						break;
					case EIOPinMode::PWM:
						Coef = Value.Duty;
						break;
					}
					Voltage = Value.Voltage;
				}

				LightItem.Value->SetEnabled(Voltage > 0.5, Coef);

				FIOPinSourceValue PubSourceValue{};
				PubSourceValue.Mode = EIOPinMode::Input;
				PubSourceValue.Voltage = Voltage;
				PinInterface->PublishSource(PubSourceValue);

				FIOPinFeedbackValue PubFeedbackValue{};
				PubFeedbackValue.bIsMeasuredVoltageValid = true;
				PubFeedbackValue.MeasuredVoltage = Voltage;
				PubFeedbackValue.bIsMeasuredCurrentValid = true;
				PubFeedbackValue.MeasuredCurrent = FMath::Clamp(Voltage / LightSetup.Resistanse, 0.0f, LightSetup.IOPinSetup.MaxCurrent);
				PinInterface->PublishFeedback(PubFeedbackValue);
			}
		}
	}
}