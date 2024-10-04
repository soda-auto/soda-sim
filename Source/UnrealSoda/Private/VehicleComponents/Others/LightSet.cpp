// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.


#include "Soda/VehicleComponents/Others/LightSet.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Components/LightComponent.h"

UVehicleLightItem::UVehicleLightItem(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}
void UVehicleLightItem::Setup(const FVehicleLightItemSetup& InItemSetup)
{
	ItemSetup = InItemSetup;
}

bool UVehicleLightItem::SetupIO(UIOBusNode* Node)
{
	if (IsValid(Node) && ItemSetup.bUseIO)
	{
		Exchange = Node->RegisterPin(ItemSetup.IOPinSetup);
	}
	return !!Exchange;
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

	if (IsValid(Exchange) && ItemSetup.bSendIOFeedback)
	{
		float Voltage = Exchange->GetSourceValue().Voltage;
		FIOExchangeFeedbackValue FeedbackValue;
		FeedbackValue.MeasuredCurrent = Voltage * FMath::Clamp(Intensity, 0.f, 1.f) * ItemSetup.Resistanse;
		FeedbackValue.MeasuredVoltage = Voltage;
		FeedbackValue.bIsMeasuredCurrentValid = true;
		FeedbackValue.bIsMeasuredVoltageValid = true;
		Exchange->SetFeedbackValue(FeedbackValue);
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

	IOBus = LinkToIOBus.GetObject<UIOBusComponent>(GetOwner());

	if (IOBus)
	{
		Node = IOBus->RegisterNode(IOBusNodeName);
		check(Node);

		AddLightItems(DefaultLightSetups);
		
		for (auto& Itme : LightItems)
		{
			Itme.Value->SetupIO(Node);
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

	if (IsValid(IOBus))
	{
		IOBus->UnregisterNode(Node);
	}
	IOBus = nullptr;
	Node = nullptr;

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
		for (auto& LightItem : LightItems)
		{
			UIOExchange* Exchange = LightItem.Value->GetExchange();
			const FVehicleLightItemSetup& LightSetup = LightItem.Value->GetSetup();

			if (Exchange)
			{
				if (LightSetup.bUseIO)
				{
					float Coef = 0;
					auto& Value = Exchange->GetSourceValue();
					switch (EIOExchangeMode::Analogue) //(Value.Mode)
					{
					case EIOExchangeMode::Fixed:
					case EIOExchangeMode::Analogue:
						Coef = (Value.Voltage - LightSetup.IOPinSetup.MinVoltage) / (LightSetup.IOPinSetup.MaxVoltage - LightSetup.IOPinSetup.MinVoltage);
						break;
					case EIOExchangeMode::PWM:
						Coef = Value.Duty;
						break;
					}
					//LightItem.Value->SetEnabled(Value.LogicalVal, Coef);
					LightItem.Value->SetEnabled(Value.Voltage > 0.5, Coef);
				}
			}
		}
	}
}