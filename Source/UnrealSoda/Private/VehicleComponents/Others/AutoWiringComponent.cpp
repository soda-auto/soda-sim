// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/VehicleComponents/Others/AutoWiringComponent.h"
#include "Json.h"
#include "Soda/VehicleComponents/Others/DummyComponent.h"


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
      // JSON string
      FString JsonString = TEXT(R"(
        {
            "peripherals": [
                {
                    "id": "78444252-fe3b-4d9c-826c-d84bddb1a8dc",
                    "kind": 1,
                    "coordinates": [1.1, 2.2, 3.3]
                }
            ],
            "ioModules": [
                {
                    "id": "ae3b7d03-986c-4d09-8b06-632caf0abe00",
                    "kind": 2,
                    "coordinates": [4.4, 5.5, 6.6]
                }
            ],
            "connections": [
                {
                    "sourceId": "78444252-fe3b-4d9c-826c-d84bddb1a8dc",
                    "sourcePin": "X1.2",
                    "destinationId": "ae3b7d03-986c-4d09-8b06-632caf0abe00",
                    "destinationPin": "X2.3"
                }
            ]
        })");

      // Json object create
      TSharedPtr<FJsonObject> ResponseObj;
      TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);

      // Deserialise
      if (FJsonSerializer::Deserialize(Reader, ResponseObj) && ResponseObj.IsValid())
      {

         //UE_LOG(LogTemp, Display, TEXT("Response: %s"), *JsonString);

         // Temp example
         const TArray<TSharedPtr<FJsonValue>>* PeripheralsArray;
         if (ResponseObj->TryGetArrayField(TEXT("peripherals"), PeripheralsArray))
         {
            for (const TSharedPtr<FJsonValue>& Value : *PeripheralsArray)
            {
               TSharedPtr<FJsonObject> PeripheralObject = Value->AsObject();
               FString Id = PeripheralObject->GetStringField(TEXT("id"));
               int32 Kind = PeripheralObject->GetIntegerField(TEXT("kind"));
               const TArray<TSharedPtr<FJsonValue>>* CoordinatesArray;
               if (PeripheralObject->TryGetArrayField(TEXT("coordinates"), CoordinatesArray))
               {
                  for (const TSharedPtr<FJsonValue>& Coord : *CoordinatesArray)
                  {
                     float Coordinate = Coord->AsNumber();
                     UE_LOG(LogTemp, Display, TEXT("Peripheral ID: %s, Kind: %d, Coordinate: %f"), *Id, Kind, Coordinate);
                  }
               }
            }
         }
      }
      else
      {
         UE_LOG(LogTemp, Error, TEXT("Failed to deserialize JSON string"));
      }
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

