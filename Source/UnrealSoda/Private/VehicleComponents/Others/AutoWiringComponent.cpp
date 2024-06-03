// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/VehicleComponents/Others/AutoWiringComponent.h"
#include "Json.h"
#include "Soda/Vehicles/IWheeledVehicleMovementInterface.h"


UAutoWiringComponent::UAutoWiringComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	GUI.Category = TEXT("Other");
	GUI.ComponentNameOverride = TEXT("Auto wiring");
	GUI.bIsPresentInAddMenu = true;

}

void UAutoWiringComponent::SendRequest()
{
	FHttpRequestPtr Request = FHttpModule::Get().CreateRequest();
	Request->OnProcessRequestComplete().BindUObject(this, &UAutoWiringComponent::OnResponseReceived);
	Request->SetURL("https://jsonplaceholder.typicode.com/posts/1");
	Request->SetVerb("GET");
	Request->ProcessRequest();
}

void UAutoWiringComponent::OnResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully)
{
	if (bConnectedSuccessfully && Response.IsValid())
	{
		TSharedPtr<FJsonObject> ResponseObj;
		TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());
		FJsonSerializer::Deserialize(Reader, ResponseObj);
		
		UE_LOG(LogTemp, Display, TEXT("Response: %s"), *Response->GetContentAsString());
		UE_LOG(LogTemp, Display, TEXT("Title: %s"), *ResponseObj->GetStringField("title"));

		//ResponseObj->
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to receive response"));
	}
}

bool UAutoWiringComponent::OnActivateVehicleComponent()
{
	if (!Super::OnActivateVehicleComponent())
	{
		return false;
	}

	return true;
}

void UAutoWiringComponent::OnDeactivateVehicleComponent()
{
	Super::OnDeactivateVehicleComponent();
}

