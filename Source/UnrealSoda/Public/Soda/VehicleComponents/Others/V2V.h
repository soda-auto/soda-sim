// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "Soda/VehicleComponents/VehicleBaseComponent.h"
#include "Soda/Transport/GenericV2XPublisher.h"
#include "V2V.generated.h"

class FSocket;
class FInternetAddr;

UCLASS(ClassGroup = Soda, BlueprintType, meta = (BlueprintSpawnableComponent))
class UNREALSODA_API UV2VTransmitterComponent : public UVehicleBaseComponent
{
	GENERATED_UCLASS_BODY()

public:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	virtual bool OnActivateVehicleComponent() override;
	virtual void OnDeactivateVehicleComponent() override;
	virtual void GetRemark(FString& Info) const override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = V2V, SaveGame, meta = (EditInRuntime))
	FVector BoundingBox0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = V2V, SaveGame, meta = (EditInRuntime))
	FVector BoundingBox1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = V2V, SaveGame, meta = (EditInRuntime))
	FVector BoundingBox2;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = V2V, SaveGame, meta = (EditInRuntime))
	FVector BoundingBox3;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = V2V, SaveGame, meta = (EditInRuntime))
	FVector BoundingBox4;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = V2V, SaveGame, meta = (EditInRuntime))
	FVector BoundingBox5;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = V2V, SaveGame, meta = (EditInRuntime))
	FVector BoundingBox6;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = V2V, SaveGame, meta = (EditInRuntime))
	FVector BoundingBox7;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = V2V, SaveGame, meta = (EditInRuntime))
	int ID = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = V2V, SaveGame, meta = (EditInRuntime))
	bool bAssignUniqueIdAtStartup = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = V2V, SaveGame, meta = (EditInRuntime))
	bool bAutoCalculateBoundingBox = true;

public:
	UFUNCTION(BlueprintCallable, Category = V2V)
	void AssignUniqueId() { ID = ++IDsCounter; }

	bool FillV2VMsg(soda::V2VSimulatedObject& msg, bool RecalculateBoundingBox, bool DrawDebug = false);
	bool FillV2VMsg(soda::V2XRenderObject& msg, bool RecalculateBoundingBox, bool DrawDebug = false);

	void CalculateBoundingBox();

protected:
	static int IDsCounter;
};

UCLASS(ClassGroup = Soda, BlueprintType, meta = (BlueprintSpawnableComponent))
class UNREALSODA_API UV2VReceiverComponent : public UVehicleBaseComponent
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = V2V, SaveGame, meta = (EditInRuntime, ReactivateComponent))
	FGenericV2XPublisher Publisher;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = V2V, SaveGame, meta = (EditInRuntime))
	float Radius = 100; // [m]

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = V2V, SaveGame, meta = (EditInRuntime))
	bool bRecalculateBoundingBoxEveryTick = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = V2V, SaveGame, meta = (EditInRuntime))
	bool bLogMessages = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = V2V, SaveGame, meta = (EditInRuntime, ReactivateComponent))
	bool bUseNewProtocol = false;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	virtual bool OnActivateVehicleComponent() override;
	virtual void OnDeactivateVehicleComponent() override;
	virtual void GetRemark(FString & Info) const override;
};
