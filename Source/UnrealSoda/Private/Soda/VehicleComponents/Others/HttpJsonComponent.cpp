// Fill out your copyright notice in the Description page of Project Settings.


#include "Soda/VehicleComponents/Others/HttpJsonComponent.h"

bool UHttpJsonComponent::OnActivateVehicleComponent()
{
	if (!Super::OnActivateVehicleComponent())
	{
		return false;
	}

	FHttpRequestPtr Request = FHttpModule::Get().CreateRequest();
	Request->OnProcessRequestComplete().BindUObject(this, &UHttpJsonComponent::OnResponceRecieved);
	Request->SetURL("");

	return true;
}

void UHttpJsonComponent::OnDeactivateVehicleComponent()
{
}

void UHttpJsonComponent::OnResponceRecieved(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully)
{

}
