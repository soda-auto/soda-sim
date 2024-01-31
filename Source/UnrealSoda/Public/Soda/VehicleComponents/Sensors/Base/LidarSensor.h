// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "Soda/VehicleComponents/VehicleSensorComponent.h"
#include "LidarSensor.generated.h"

namespace soda
{

enum class ELidarPointStatus
{
	Invalid,
	Valid,
	Filtered,
};

struct FLidarScanPoint
{
	FVector Location {}; // [cm]
	float Depth{}; // [cm]
	int Layer = -1;
	ELidarPointStatus Status = ELidarPointStatus::Invalid;
};

struct FLidarScan
{
	//TTimestamp Timestamp;
	//int64 Index;
	TArray<FLidarScanPoint> Points;
};

} // namespace soda

UENUM(BlueprintType)
enum class ELidarInterpolation : uint8
{
	Bilinear,
	Min,
	Nearest
};


UCLASS(abstract, ClassGroup = Soda, BlueprintType, meta = (BlueprintSpawnableComponent))
class UNREALSODA_API ULidarSensor : public USensorComponent
{
	GENERATED_UCLASS_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Debug, SaveGame, meta = (EditInRuntime))
	bool bDrawLidarPoints = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = FOVRenderer, SaveGame, meta = (EditInRuntime, UpdateFOVRendering))
	FSensorFOVRenderer FOVSetup;

public:
	virtual float GetFOVHorizontMax() const { return 0; }
	virtual float GetFOVHorizontMin() const { return 0; }
	virtual float GetFOVVerticalMax() const { return 0; }
	virtual float GetFOVVerticalMin() const { return 0; }
	virtual float GetLidarMinDistance() const { return 0; }
	virtual float GetLidarMaxDistance() const { return 0; }
	virtual const TArray<FVector>& GetLidarRays() const { static TArray<FVector> Rays; return Rays; }
	virtual void PostProcessSensorData(soda::FLidarScan& Scan) {}
	virtual bool PublishSensorData(float DeltaTime, const FSensorDataHeader& Header, const soda::FLidarScan& Scan) { return false; }

public:
	virtual void DrawLidarPoints(const soda::FLidarScan& Scan, bool bDrawInGameThread);

protected:
	virtual bool GenerateFOVMesh(TArray<FSensorFOVMesh>& Meshes) override;
	virtual bool NeedRenderSensorFOV() const;
	virtual FBoxSphereBounds CalcBounds(const FTransform& LocalToWorld) const override;

public:
	//ULidarRayTraceSensorComponent();
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
};
