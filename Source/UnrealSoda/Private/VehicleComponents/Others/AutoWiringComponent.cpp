#include "Soda/VehicleComponents/Others/AutoWiringComponent.h"
#include "Json.h"
#include "Soda/VehicleComponents/Others/DummyComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "DrawDebugHelpers.h"

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
                    "kind": 0,
                    "coordinates": [1.1, 2.2, 3.3]
                }
            ],
            "ioModules": [
                {
                    "id": "ae3b7d03-986c-4d09-8b06-632caf0abe00",
                    "kind": 1,
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

      // Deserialize
      if (FJsonSerializer::Deserialize(Reader, ResponseObj) && ResponseObj.IsValid())
      {
         UpdateDummies(ResponseObj);
         DrawConnections(ResponseObj);
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

void UAutoWiringComponent::UpdateDummies(const TSharedPtr<FJsonObject>& JsonObject)
{
   AActor* OwnerActor = GetOwner();
   if (!OwnerActor)
   {
      UE_LOG(LogTemp, Error, TEXT("Owner actor is not valid"));
      return;
   }

   TArray<UDummyComponent*> DummyComponents;
   OwnerActor->GetComponents(DummyComponents);

   if (DummyComponents.Num() == 0)
   {
      UE_LOG(LogTemp, Warning, TEXT("No DummyComponents found on owner actor."));
      return;
   }

   UE_LOG(LogTemp, Display, TEXT("Found %d DummyComponents."), DummyComponents.Num());

   const TArray<TSharedPtr<FJsonValue>>* ComponentsArray;
   auto ProcessComponents = [&](const TArray<TSharedPtr<FJsonValue>>* ComponentsArray) {
      for (const TSharedPtr<FJsonValue>& Value : *ComponentsArray)
      {
         TSharedPtr<FJsonObject> ComponentObject = Value->AsObject();
         FString Id = ComponentObject->GetStringField(TEXT("id"));
         int32 Kind = ComponentObject->GetIntegerField(TEXT("kind"));
         const TArray<TSharedPtr<FJsonValue>>* CoordinatesArray;
         if (ComponentObject->TryGetArrayField(TEXT("coordinates"), CoordinatesArray))
         {
            FVector NewLocation(
               (*CoordinatesArray)[0]->AsNumber() * 100.0f,
               (*CoordinatesArray)[1]->AsNumber() * 100.0f,
               (*CoordinatesArray)[2]->AsNumber() * 100.0f
            );

            UE_LOG(LogTemp, Display, TEXT("Processing Component with ID: %s, Kind: %d, NewLocation: %s"), *Id, Kind, *NewLocation.ToString());

            for (UDummyComponent* DummyComponent : DummyComponents)
            {
               if (DummyComponent)
               {
                  UE_LOG(LogTemp, Display, TEXT("Checking DummyComponent with Kind: %d against JSON kind: %d"), static_cast<int32>(DummyComponent->DummyType), Kind);
                  if (DummyComponent->DummyType == static_cast<EDummyType>(Kind))
                  {
                     DummyComponent->UUID = Id; 
                     DummyComponent->UpdateDummyLocation(NewLocation);
                     UE_LOG(LogTemp, Display, TEXT("Assigned UUID: %s to DummyComponent"), *Id);
                     UE_LOG(LogTemp, Display, TEXT("Updated DummyComponent with ID: %s, Kind: %d to new location: %s"), *Id, Kind, *NewLocation.ToString());
                     break; 
                  }
                  else
                  {
                     UE_LOG(LogTemp, Display, TEXT("DummyComponent Kind: %d does not match JSON kind: %d"), static_cast<int32>(DummyComponent->DummyType), Kind);
                  }
               }
            }
         }
      }
      };

   if (JsonObject->TryGetArrayField(TEXT("peripherals"), ComponentsArray))
   {
      ProcessComponents(ComponentsArray);
   }

   if (JsonObject->TryGetArrayField(TEXT("ioModules"), ComponentsArray))
   {
      ProcessComponents(ComponentsArray);
   }
}

void UAutoWiringComponent::DrawConnections(const TSharedPtr<FJsonObject>& JsonObject)
{
   const TArray<TSharedPtr<FJsonValue>>* ConnectionsArray;
   if (JsonObject->TryGetArrayField(TEXT("connections"), ConnectionsArray))
   {
      for (const TSharedPtr<FJsonValue>& Value : *ConnectionsArray)
      {
         TSharedPtr<FJsonObject> ConnectionObject = Value->AsObject();
         FString SourceId = ConnectionObject->GetStringField(TEXT("sourceId"));
         FString DestinationId = ConnectionObject->GetStringField(TEXT("destinationId"));

         AActor* OwnerActor = GetOwner();
         TArray<UDummyComponent*> DummyComponents;
         OwnerActor->GetComponents(DummyComponents);

         FVector SourceLocation, DestinationLocation;
         bool bSourceFound = false, bDestinationFound = false;

         for (UDummyComponent* DummyComponent : DummyComponents)
         {
            UE_LOG(LogTemp, Display, TEXT("Checking DummyComponent UUID: %s against SourceId: %s and DestinationId: %s"), *DummyComponent->UUID, *SourceId, *DestinationId);
            if (DummyComponent->UUID == SourceId)
            {
               SourceLocation = DummyComponent->GetComponentLocation();
               bSourceFound = true;
               UE_LOG(LogTemp, Display, TEXT("Found Source: %s at Location: %s"), *SourceId, *SourceLocation.ToString());
            }
            else if (DummyComponent->UUID == DestinationId)
            {
               DestinationLocation = DummyComponent->GetComponentLocation();
               bDestinationFound = true;
               UE_LOG(LogTemp, Display, TEXT("Found Destination: %s at Location: %s"), *DestinationId, *DestinationLocation.ToString());
            }

            if (bSourceFound && bDestinationFound)
            {
               DrawDebugLine(
                  GetWorld(),
                  SourceLocation,
                  DestinationLocation,
                  FColor::Green,
                  false, 100000.0f, 0.0,
                  5
               );
               UE_LOG(LogTemp, Display, TEXT("Drawing debug line from %s to %s"), *SourceId, *DestinationId);
               break;
            }
         }

         if (!bSourceFound || !bDestinationFound)
         {
            if (!bSourceFound)
            {
               UE_LOG(LogTemp, Warning, TEXT("Source ID: %s not found"), *SourceId);
            }
            if (!bDestinationFound)
            {
               UE_LOG(LogTemp, Warning, TEXT("Destination ID: %s not found"), *DestinationId);
            }
         }
      }
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
