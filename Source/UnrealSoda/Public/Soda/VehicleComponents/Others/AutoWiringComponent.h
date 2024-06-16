#pragma once

#include "CoreMinimal.h"
#include "Soda/VehicleComponents/WheeledVehicleComponent.h"
#include "Http.h"
#include "AutoWiringComponent.generated.h"

UCLASS(ClassGroup = Soda, BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class UNREALSODA_API UAutoWiringComponent : public UVehicleBaseComponent
{
   GENERATED_UCLASS_BODY()

public:
   UFUNCTION(BlueprintCallable, Category = Debug, meta = (CallInRuntime))
   virtual void SendRequest();

   virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
   virtual bool OnActivateVehicleComponent() override;
   virtual void OnDeactivateVehicleComponent() override;

private:
   void OnResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully);
   void UpdateDummies(const TSharedPtr<FJsonObject>& JsonObject);
   void DrawConnections(const TSharedPtr<FJsonObject>& JsonObject);

   TArray<TPair<FString, FString>> ConnectionIDs;
   TOptional<TPair<FString, FString>> LastRoofLightConnection;
   void UpdateRoofLightConnection();
   bool AreComponentsInPlace() const;
};
