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
	float Intensitie{}; // TODO
	ELidarPointStatus Status = ELidarPointStatus::Invalid;
};

struct FLidarSensorData
{

	/** [deg] */
	float HorizontalAngleMin{};

	/** [deg] */
	float HorizontalAngleMax{};

	/** [deg] */
	float VerticalAngleMin{};

	/** [deg] */
	float VerticalAngleMax{};

	/** minimum range value [cm] */
	float RangeMin{};

	/** maximum range value [cm] */
	float RangeMax{};

	/** 2D structure of the point cloud */
	TOptional<FUintVector2> Size {};

	bool bIntensitieIsValid = false;

	TArray<FLidarScanPoint> Points{};
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
	virtual float GetFOVHorizontMax() const { return 0; } // [deg]
	virtual float GetFOVHorizontMin() const { return 0; } // [deg]
	virtual float GetFOVVerticalMax() const { return 0; } // [deg]
	virtual float GetFOVVerticalMin() const { return 0; } // [deg]
	virtual float GetLidarMinDistance() const { return 0; } // [cm]
	virtual float GetLidarMaxDistance() const { return 0; } // [cm]
	virtual TOptional<FUintVector2> GetLidarSize() const { return TOptional<FUintVector2>{}; }
	virtual const TArray<FVector>& GetLidarRays() const { static TArray<FVector> Rays; return Rays; }
	virtual bool PublishSensorData(float DeltaTime, const FSensorDataHeader& Header, const soda::FLidarSensorData& Scan) { SyncDataset(); return false; }

public:
	virtual void DrawLidarPoints(const soda::FLidarSensorData& Scan, bool bDrawInGameThread);

protected:
	virtual bool GenerateFOVMesh(TArray<FSensorFOVMesh>& Meshes) override;
	virtual bool NeedRenderSensorFOV() const;
	virtual FBoxSphereBounds CalcBounds(const FTransform& LocalToWorld) const override;

public:
	//ULidarRayTraceSensorComponent();
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
};
