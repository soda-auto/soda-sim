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
   PrimaryComponentTick.bCanEverTick = true;
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
      FString JsonString =
         TEXT("{")
         TEXT("\"peripherals\": [")
         TEXT("{")
         TEXT("\"id\": \"78444252-fe3b-4d9c-826c-d84bddb1a8dc\",")
         TEXT("\"kind\": 1,")
         TEXT("\"title\": \"Drive Unit\",")
         TEXT("\"coordinates\": [2.9, 0.13, 0.14],")
         TEXT("\"labelText\": \"\"")
         TEXT("},")
         TEXT("{")
         TEXT("\"id\": \"a726d50f-8dda-4d0a-aa0a-000f156b32ec\",")
         TEXT("\"kind\": 2,")
         TEXT("\"title\": \"Front Left Light\",")
         TEXT("\"coordinates\": [3.13, -0.7, 0.5],")
         TEXT("\"labelText\": \"\"")
         TEXT("},")
         TEXT("{")
         TEXT("\"id\": \"bf9842cc-4f36-44c0-9d35-f02f8cc65061\",")
         TEXT("\"kind\": 3,")
         TEXT("\"title\": \"Front Right Light\",")
         TEXT("\"coordinates\": [3.13, 0.7, 0.5],")
         TEXT("\"labelText\": \"\"")
         TEXT("},")
         TEXT("{")
         TEXT("\"id\": \"431db986-946e-48a6-8fd7-99f3129313f1\",")
         TEXT("\"kind\": 4,")
         TEXT("\"title\": \"Rear Left Position Light\",")
         TEXT("\"coordinates\": [-0.44, -0.66, 0.97],")
         TEXT("\"labelText\": \"\"")
         TEXT("},")
         TEXT("{")
         TEXT("\"id\": \"426e91e7-c5aa-4de7-9641-8312336c30a0\",")
         TEXT("\"kind\": 5,")
         TEXT("\"title\": \"Rear Left Reverse Light\",")
         TEXT("\"coordinates\": [-0.4, -0.69, 0.97],")
         TEXT("\"labelText\": \"\"")
         TEXT("},")
         TEXT("{")
         TEXT("\"id\": \"c029a8b7-865e-4337-8901-364f345c1ca0\",")
         TEXT("\"kind\": 6,")
         TEXT("\"title\": \"Rear Left Turn Indicator\",")
         TEXT("\"coordinates\": [-0.46, -0.67, 1.0],") 
         TEXT("\"labelText\": \"\"")
         TEXT("},")
         TEXT("{")
         TEXT("\"id\": \"5f5595b8-38c5-4e71-9791-36ea67c5268e\",")
         TEXT("\"kind\": 7,")
         TEXT("\"title\": \"Rear Right Position Light\",") 
         TEXT("\"coordinates\": [-0.44, 0.66, 0.97],")
         TEXT("\"labelText\": \"\"")
         TEXT("},")
         TEXT("{")
         TEXT("\"id\": \"1d913ac6-6ff5-4f91-8c8a-c141759681ff\",")
         TEXT("\"kind\": 8,")
         TEXT("\"title\": \"Rear Right Reverse Light\",") 
         TEXT("\"coordinates\": [-0.41, 0.695, 0.97],")
         TEXT("\"labelText\": \"\"")
         TEXT("},")
         TEXT("{")
         TEXT("\"id\": \"954682ea-403f-452c-b9cb-06c71e52031b\",")
         TEXT("\"kind\": 9,")
         TEXT("\"title\": \"Rear Right Turn Indicator\",")
         TEXT("\"coordinates\": [-0.46, 0.67, 1.0],")
         TEXT("\"labelText\": \"\"")
         TEXT("},")
         TEXT("{")
         TEXT("\"id\": \"dd9fb6cb-38bb-456a-8c8d-982619560190\",")
         TEXT("\"kind\": 10,")
         TEXT("\"title\": \"Roof Light\",")
         TEXT("\"coordinates\": [0.93, 0.0, 1.39],")
         TEXT("\"labelText\": \"\"")
         TEXT("}")
         TEXT("],")
         TEXT("\"ioModules\": [")
         TEXT("{")
         TEXT("\"id\": \"ae3b7d03-986c-4d09-8b06-632caf0abe00\",")
         TEXT("\"kind\": 0,")
         TEXT("\"title\": \"FL ECU\",")
         TEXT("\"coordinates\": [2.7, -0.47, 0.14],")
         TEXT("\"labelText\": \"ECU2.Pin8 <-> L DRL\\nECU2.Pin9 <-> DU.Pin4\"")
         TEXT("},")
         TEXT("{")
         TEXT("\"id\": \"f70de327-0caa-4d25-994e-d68dfe26807e\",")
         TEXT("\"kind\": 0,")
         TEXT("\"title\": \"FR ECU\",")
         TEXT("\"coordinates\": [2.7, 0.47, 0.14],")
         TEXT("\"labelText\": \"ECU1.Pin4 <-> R Low Beam\\nECU1.Pin5 <-> R Turn Ind\"")
         TEXT("},")
         TEXT("{")
         TEXT("\"id\": \"0f108a3d-b831-4841-9834-c136001fe014\",")
         TEXT("\"kind\": 0,")
         TEXT("\"title\": \"Rear ECU\",")
         TEXT("\"coordinates\": [-0.26, 0.0, 0.14],")
         TEXT("\"labelText\": \"\"")
         TEXT("}")
         TEXT("],")
         TEXT("\"connections\": [")
         TEXT("{")
         TEXT("\"sourceId\": \"78444252-fe3b-4d9c-826c-d84bddb1a8dc\",")
         TEXT("\"sourcePin\": \"X1.1\",")
         TEXT("\"destinationId\": \"f70de327-0caa-4d25-994e-d68dfe26807e\",")
         TEXT("\"destinationPin\": \"X1.1\",")
         TEXT("\"kind\": 0")
         TEXT("},")
         TEXT("{")
         TEXT("\"sourceId\": \"78444252-fe3b-4d9c-826c-d84bddb1a8dc\",")
         TEXT("\"sourcePin\": \"X1.2\",")
         TEXT("\"destinationId\": \"f70de327-0caa-4d25-994e-d68dfe26807e\",")
         TEXT("\"destinationPin\": \"X1.2\",")
         TEXT("\"kind\": 0")
         TEXT("},")
         TEXT("{")
         TEXT("\"sourceId\": \"78444252-fe3b-4d9c-826c-d84bddb1a8dc\",")
         TEXT("\"sourcePin\": \"X1.3\",")
         TEXT("\"destinationId\": \"f70de327-0caa-4d25-994e-d68dfe26807e\",")
         TEXT("\"destinationPin\": \"X1.3\",")
         TEXT("\"kind\": 0")
         TEXT("},")
         TEXT("{")
         TEXT("\"sourceId\": \"a726d50f-8dda-4d0a-aa0a-000f156b32ec\",")
         TEXT("\"sourcePin\": \"X1.1\",")
         TEXT("\"destinationId\": \"ae3b7d03-986c-4d09-8b06-632caf0abe00\",")
         TEXT("\"destinationPin\": \"X1.1\",")
         TEXT("\"kind\": 0")
         TEXT("},")
         TEXT("{")
         TEXT("\"sourceId\": \"a726d50f-8dda-4d0a-aa0a-000f156b32ec\",")
         TEXT("\"sourcePin\": \"X1.2\",")
         TEXT("\"destinationId\": \"ae3b7d03-986c-4d09-8b06-632caf0abe00\",")
         TEXT("\"destinationPin\": \"X1.2\",")
         TEXT("\"kind\": 0")
         TEXT("},")
         TEXT("{")
         TEXT("\"sourceId\": \"bf9842cc-4f36-44c0-9d35-f02f8cc65061\",")
         TEXT("\"sourcePin\": \"X1.1\",")
         TEXT("\"destinationId\": \"f70de327-0caa-4d25-994e-d68dfe26807e\",")
         TEXT("\"destinationPin\": \"X1.4\",")
         TEXT("\"kind\": 0")
         TEXT("},")
         TEXT("{")
         TEXT("\"sourceId\": \"bf9842cc-4f36-44c0-9d35-f02f8cc65061\",")
         TEXT("\"sourcePin\": \"X1.2\",")
         TEXT("\"destinationId\": \"f70de327-0caa-4d25-994e-d68dfe26807e\",")
         TEXT("\"destinationPin\": \"X1.5\",")
         TEXT("\"kind\": 0")
         TEXT("},")
         TEXT("{")
         TEXT("\"sourceId\": \"431db986-946e-48a6-8fd7-99f3129313f1\",")
         TEXT("\"sourcePin\": \"X1.1\",")
         TEXT("\"destinationId\": \"0f108a3d-b831-4841-9834-c136001fe014\",")
         TEXT("\"destinationPin\": \"X1.1\",")
         TEXT("\"kind\": 0")
         TEXT("},")
         TEXT("{")
         TEXT("\"sourceId\": \"431db986-946e-48a6-8fd7-99f3129313f1\",")
         TEXT("\"sourcePin\": \"X1.2\",")
         TEXT("\"destinationId\": \"0f108a3d-b831-4841-9834-c136001fe014\",")
         TEXT("\"destinationPin\": \"X1.2\",")
         TEXT("\"kind\": 0")
         TEXT("},")
         TEXT("{")
         TEXT("\"sourceId\": \"426e91e7-c5aa-4de7-9641-8312336c30a0\",")
         TEXT("\"sourcePin\": \"X1.1\",")
         TEXT("\"destinationId\": \"0f108a3d-b831-4841-9834-c136001fe014\",")
         TEXT("\"destinationPin\": \"X1.3\",")
         TEXT("\"kind\": 0")
         TEXT("},")
         TEXT("{")
         TEXT("\"sourceId\": \"426e91e7-c5aa-4de7-9641-8312336c30a0\",")
         TEXT("\"sourcePin\": \"X1.2\",")
         TEXT("\"destinationId\": \"0f108a3d-b831-4841-9834-c136001fe014\",")
         TEXT("\"destinationPin\": \"X1.4\",")
         TEXT("\"kind\": 0")
         TEXT("},")
         TEXT("{")
         TEXT("\"sourceId\": \"c029a8b7-865e-4337-8901-364f345c1ca0\",")
         TEXT("\"sourcePin\": \"X1.1\",")
         TEXT("\"destinationId\": \"0f108a3d-b831-4841-9834-c136001fe014\",")
         TEXT("\"destinationPin\": \"X1.5\",")
         TEXT("\"kind\": 0")
         TEXT("},")
         TEXT("{")
         TEXT("\"sourceId\": \"c029a8b7-865e-4337-8901-364f345c1ca0\",")
         TEXT("\"sourcePin\": \"X1.2\",")
         TEXT("\"destinationId\": \"0f108a3d-b831-4841-9834-c136001fe014\",")
         TEXT("\"destinationPin\": \"X1.6\",")
         TEXT("\"kind\": 0")
         TEXT("},")
         TEXT("{")
         TEXT("\"sourceId\": \"5f5595b8-38c5-4e71-9791-36ea67c5268e\",")
         TEXT("\"sourcePin\": \"X1.1\",")
         TEXT("\"destinationId\": \"0f108a3d-b831-4841-9834-c136001fe014\",")
         TEXT("\"destinationPin\": \"X1.7\",")
         TEXT("\"kind\": 0")
         TEXT("},")
         TEXT("{")
         TEXT("\"sourceId\": \"5f5595b8-38c5-4e71-9791-36ea67c5268e\",")
         TEXT("\"sourcePin\": \"X1.2\",")
         TEXT("\"destinationId\": \"0f108a3d-b831-4841-9834-c136001fe014\",")
         TEXT("\"destinationPin\": \"X1.8\",")
         TEXT("\"kind\": 0")
         TEXT("},")
         TEXT("{")
         TEXT("\"sourceId\": \"1d913ac6-6ff5-4f91-8c8a-c141759681ff\",")
         TEXT("\"sourcePin\": \"X1.1\",")
         TEXT("\"destinationId\": \"0f108a3d-b831-4841-9834-c136001fe014\",")
         TEXT("\"destinationPin\": \"X1.9\",")
         TEXT("\"kind\": 0")
         TEXT("},")
         TEXT("{")
         TEXT("\"sourceId\": \"1d913ac6-6ff5-4f91-8c8a-c141759681ff\",")
         TEXT("\"sourcePin\": \"X1.2\",")
         TEXT("\"destinationId\": \"0f108a3d-b831-4841-9834-c136001fe014\",")
         TEXT("\"destinationPin\": \"X1.10\",")
         TEXT("\"kind\": 0")
         TEXT("},")
         TEXT("{")
         TEXT("\"sourceId\": \"954682ea-403f-452c-b9cb-06c71e52031b\",")
         TEXT("\"sourcePin\": \"X1.1\",")
         TEXT("\"destinationId\": \"0f108a3d-b831-4841-9834-c136001fe014\",")
         TEXT("\"destinationPin\": \"X1.11\",")
         TEXT("\"kind\": 0")
         TEXT("},")
         TEXT("{")
         TEXT("\"sourceId\": \"954682ea-403f-452c-b9cb-06c71e52031b\",")
         TEXT("\"sourcePin\": \"X1.2\",")
         TEXT("\"destinationId\": \"0f108a3d-b831-4841-9834-c136001fe014\",")
         TEXT("\"destinationPin\": \"X1.12\",")
         TEXT("\"kind\": 0")
         TEXT("},")
         TEXT("{")
         TEXT("\"sourceId\": \"dd9fb6cb-38bb-456a-8c8d-982619560190\",")
         TEXT("\"sourcePin\": \"X1.1\",")
         TEXT("\"destinationId\": \"0f108a3d-b831-4841-9834-c136001fe014\",")
         TEXT("\"destinationPin\": \"X1.13\",")
         TEXT("\"kind\": 0")
         TEXT("},")
         TEXT("{")
         TEXT("\"sourceId\": \"dd9fb6cb-38bb-456a-8c8d-982619560190\",")
         TEXT("\"sourcePin\": \"X1.2\",")
         TEXT("\"destinationId\": \"0f108a3d-b831-4841-9834-c136001fe014\",")
         TEXT("\"destinationPin\": \"X1.14\",")
         TEXT("\"kind\": 0")
         TEXT("},")
         TEXT("{")
         TEXT("\"sourceId\": \"ae3b7d03-986c-4d09-8b06-632caf0abe00\",")
         TEXT("\"sourcePin\": \"X2.1\",")
         TEXT("\"destinationId\": \"f70de327-0caa-4d25-994e-d68dfe26807e\",")
         TEXT("\"destinationPin\": \"X2.1\",")
         TEXT("\"kind\": 1")
         TEXT("},")
         TEXT("{")
         TEXT("\"sourceId\": \"ae3b7d03-986c-4d09-8b06-632caf0abe00\",")
         TEXT("\"sourcePin\": \"X2.2\",")
         TEXT("\"destinationId\": \"f70de327-0caa-4d25-994e-d68dfe26807e\",")
         TEXT("\"destinationPin\": \"X2.2\",")
         TEXT("\"kind\": 1")
         TEXT("},")
         TEXT("{")
         TEXT("\"sourceId\": \"0f108a3d-b831-4841-9834-c136001fe014\",")
         TEXT("\"sourcePin\": \"X2.1\",")
         TEXT("\"destinationId\": \"f70de327-0caa-4d25-994e-d68dfe26807e\",")
         TEXT("\"destinationPin\": \"X2.3\",")
         TEXT("\"kind\": 1")
         TEXT("},")
         TEXT("{")
         TEXT("\"sourceId\": \"0f108a3d-b831-4841-9834-c136001fe014\",")
         TEXT("\"sourcePin\": \"X2.2\",")
         TEXT("\"destinationId\": \"f70de327-0caa-4d25-994e-d68dfe26807e\",")
         TEXT("\"destinationPin\": \"X2.4\",")
         TEXT("\"kind\": 1")
         TEXT("}")
         TEXT("]")
         TEXT("}");

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

            FString LabelText = ComponentObject->GetStringField(TEXT("labelText"));

            UE_LOG(LogTemp, Display, TEXT("Processing Component with ID: %s, Kind: %d, NewLocation: %s, LabelText: %s"), *Id, Kind, *NewLocation.ToString(), *LabelText);

            bool bComponentUpdated = false;

            for (UDummyComponent* DummyComponent : DummyComponents)
            {
               if (DummyComponent && DummyComponent->DummyType == static_cast<EDummyType>(Kind) && DummyComponent->UUID.IsEmpty())
               {
                  DummyComponent->UUID = Id;
                  DummyComponent->UpdateDummyLocation(NewLocation);
                  DummyComponent->SetLabelText(LabelText);
                  bComponentUpdated = true;
                  UE_LOG(LogTemp, Display, TEXT("Assigned UUID: %s to DummyComponent"), *Id);
                  UE_LOG(LogTemp, Display, TEXT("Updated DummyComponent with ID: %s, Kind: %d to new location: %s"), *Id, Kind, *NewLocation.ToString());
                  break;
               }
            }

            if (!bComponentUpdated)
            {
               UE_LOG(LogTemp, Warning, TEXT("No available DummyComponent found for ID: %s, Kind: %d"), *Id, Kind);
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
   ConnectionIDs.Empty();

   const TArray<TSharedPtr<FJsonValue>>* ConnectionsArray;
   if (JsonObject->TryGetArrayField(TEXT("connections"), ConnectionsArray))
   {
      for (const TSharedPtr<FJsonValue>& Value : *ConnectionsArray)
      {
         TSharedPtr<FJsonObject> ConnectionObject = Value->AsObject();
         FString SourceId = ConnectionObject->GetStringField(TEXT("sourceId"));
         FString DestinationId = ConnectionObject->GetStringField(TEXT("destinationId"));

         ConnectionIDs.Add(TPair<FString, FString>(SourceId, DestinationId));
         UE_LOG(LogTemp, Display, TEXT("Connection added: %s -> %s"), *SourceId, *DestinationId);
      }
   }

   AActor* OwnerActor = GetOwner();
   TArray<UDummyComponent*> DummyComponents;
   OwnerActor->GetComponents(DummyComponents);

   for (UDummyComponent* DummyComponent : DummyComponents)
   {
      if (DummyComponent && DummyComponent->DummyType == EDummyType::RoofLight)
      {
         UDummyComponent* NearestECU = nullptr;
         float MinDistance = FLT_MAX;
         FVector RoofLightLocation = DummyComponent->GetComponentLocation();

         for (UDummyComponent* PotentialECU : DummyComponents)
         {
            if (PotentialECU && PotentialECU->DummyType == EDummyType::ECU)
            {
               float Distance = FVector::Dist(RoofLightLocation, PotentialECU->GetComponentLocation());
               if (Distance < MinDistance)
               {
                  MinDistance = Distance;
                  NearestECU = PotentialECU;
               }
            }
         }

         if (NearestECU)
         {
            ConnectionIDs.Add(TPair<FString, FString>(DummyComponent->UUID, NearestECU->UUID));
            UE_LOG(LogTemp, Display, TEXT("Connection added for RoofLight: %s -> %s"), *DummyComponent->UUID, *NearestECU->UUID);
         }
         else
         {
            UE_LOG(LogTemp, Warning, TEXT("No ECU found for RoofLight with UUID: %s"), *DummyComponent->UUID);
         }
      }
   }
}

void UAutoWiringComponent::UpdateRoofLightConnection()
{
   AActor* OwnerActor = GetOwner();
   if (!OwnerActor) return;

   TArray<UDummyComponent*> DummyComponents;
   OwnerActor->GetComponents(DummyComponents);

   UDummyComponent* RoofLightComponent = nullptr;
   UDummyComponent* NearestECU = nullptr;
   float MinDistance = FLT_MAX;

   for (UDummyComponent* DummyComponent : DummyComponents)
   {
      if (DummyComponent && DummyComponent->DummyType == EDummyType::RoofLight)
      {
         RoofLightComponent = DummyComponent;
         FVector RoofLightLocation = DummyComponent->GetComponentLocation();

         for (UDummyComponent* PotentialECU : DummyComponents)
         {
            if (PotentialECU && PotentialECU->DummyType == EDummyType::ECU)
            {
               float Distance = FVector::Dist(RoofLightLocation, PotentialECU->GetComponentLocation());
               if (Distance < MinDistance)
               {
                  MinDistance = Distance;
                  NearestECU = PotentialECU;
               }
            }
         }

         break; // Предполагаем, что только один RoofLightComponent
      }
   }

   if (RoofLightComponent && NearestECU)
   {
      TPair<FString, FString> NewConnection(RoofLightComponent->UUID, NearestECU->UUID);

      if (LastRoofLightConnection != NewConnection)
      {
         // Удаление предыдущего текста только для Roof Light
         for (UDummyComponent* DummyComponent : DummyComponents)
         {
            if (DummyComponent && DummyComponent->UUID == LastRoofLightConnection.Value)
            {
               FString CurrentLabelText = DummyComponent->GetLabelText();
               FString TextToRemove;
               if (LastRoofLightConnection.Value == "ae3b7d03-986c-4d09-8b06-632caf0abe00") // FL ECU
               {
                  TextToRemove = TEXT("ECU2.Pin8 <-> Roof Light.Pin1\nECU2.Pin9 <-> Roof Light.Pin2");
               }
               else if (LastRoofLightConnection.Value == "f70de327-0caa-4d25-994e-d68dfe26807e") // FR ECU
               {
                  TextToRemove = TEXT("ECU1.Pin8 <-> Roof Light.Pin1\nECU1.Pin9 <-> Roof Light.Pin2");
               }
               else if (LastRoofLightConnection.Value == "0f108a3d-b831-4841-9834-c136001fe014") // Rear ECU
               {
                  TextToRemove = TEXT("ECU3.Pin8 <-> Roof Light.Pin1\nECU3.Pin9 <-> Roof Light.Pin2");
               }

               CurrentLabelText = CurrentLabelText.Replace(*TextToRemove, TEXT(""));
               DummyComponent->SetLabelText(CurrentLabelText);
               break;
            }
         }

         LastRoofLightConnection = NewConnection;

         // Обновление текста для ближайшего ECU
         FString CurrentLabelText = NearestECU->GetLabelText();
         FString NewText;
         if (NearestECU->UUID == "ae3b7d03-986c-4d09-8b06-632caf0abe00") // FL ECU
         {
            NewText = TEXT("ECU2.Pin8 <-> Roof Light.Pin1\nECU2.Pin9 <-> Roof Light.Pin2");
         }
         else if (NearestECU->UUID == "f70de327-0caa-4d25-994e-d68dfe26807e") // FR ECU
         {
            NewText = TEXT("ECU1.Pin8 <-> Roof Light.Pin1\nECU1.Pin9 <-> Roof Light.Pin2");
         }
         else if (NearestECU->UUID == "0f108a3d-b831-4841-9834-c136001fe014") // Rear ECU
         {
            NewText = TEXT("ECU3.Pin8 <-> Roof Light.Pin1\nECU3.Pin9 <-> Roof Light.Pin2");
         }

         if (!CurrentLabelText.Contains(NewText))
         {
            CurrentLabelText.Append(NewText);
            NearestECU->SetLabelText(CurrentLabelText);
         }

         // Обновление соединений
         ConnectionIDs.Empty(); // Удаляем все существующие соединения
         ConnectionIDs.Add(NewConnection);
      }
   }
   else
   {
      UE_LOG(LogTemp, Warning, TEXT("No ECU found for RoofLight with UUID: %s"), RoofLightComponent ? *RoofLightComponent->UUID : TEXT("Unknown"));
   }
}

void UAutoWiringComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
   Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

   UpdateRoofLightConnection();

   AActor* OwnerActor = GetOwner();
   TArray<UDummyComponent*> DummyComponents;
   OwnerActor->GetComponents(DummyComponents);

   for (const auto& ConnectionID : ConnectionIDs)
   {
      FVector SourceLocation, DestinationLocation;
      bool bSourceFound = false, bDestinationFound = false;

      for (UDummyComponent* DummyComponent : DummyComponents)
      {
         if (DummyComponent->UUID == ConnectionID.Key)
         {
            SourceLocation = DummyComponent->GetComponentLocation();
            bSourceFound = true;
            UE_LOG(LogTemp, Display, TEXT("Found source DummyComponent: %s"), *ConnectionID.Key);
         }
         if (DummyComponent->UUID == ConnectionID.Value)
         {
            DestinationLocation = DummyComponent->GetComponentLocation();
            bDestinationFound = true;
            UE_LOG(LogTemp, Display, TEXT("Found destination DummyComponent: %s"), *ConnectionID.Value);
         }

         if (bSourceFound && bDestinationFound)
         {
            UE_LOG(LogTemp, Display, TEXT("Drawing line from %s to %s"), *ConnectionID.Key, *ConnectionID.Value);
            DrawDebugLine(
               GetWorld(),
               SourceLocation,
               DestinationLocation,
               FColor(171, 196, 197), //171, 196, 197
               false, -1.0f, 0,
               0.5f
            );
            break;
         }
      }

      if (!bSourceFound)
      {
         UE_LOG(LogTemp, Warning, TEXT("Source DummyComponent with UUID: %s not found"), *ConnectionID.Key);
      }
      if (!bDestinationFound)
      {
         UE_LOG(LogTemp, Warning, TEXT("Destination DummyComponent with UUID: %s not found"), *ConnectionID.Value);
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
   ConnectionIDs.Empty();
}
