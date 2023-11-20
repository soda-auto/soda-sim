// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "Soda/VehicleComponents/VehicleSensorComponent.h"
#include "Soda/SodaTypes.h"
#include "RadarBaseSensor.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(SodaRadar, Log, All);

class URadarBaseSensorComponent;

/**
 * ERadarMode
 */
UENUM(BlueprintType)
enum class ERadarMode : uint8
{
	ClusterMode,
	ObjectMode
};

/**
 * FRadarParams
 */
USTRUCT(BlueprintType)
struct UNREALSODA_API FRadarParams
{
	GENERATED_BODY()

	/** Just tag */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = RadarParams, SaveGame, meta = (EditInRuntime))
	FString Tag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = RadarParams, SaveGame, meta = (EditInRuntime, UpdateFOVRendering))
	bool bEnabled = true;

	/** Full horizontal FOV (deg) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = RadarParams, SaveGame, meta = (EditInRuntime, ReactivateComponent, UpdateFOVRendering))
	float FOV_HorizontMax = 60;

	/** Horizontal FOV of maximum distance */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = RadarParams, SaveGame, meta = (EditInRuntime, ReactivateComponent, UpdateFOVRendering))
	float FOV_HorizontFullDist = 45;

	/** Vertical FOV */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = RadarParams, SaveGame, meta = (EditInRuntime, ReactivateComponent, UpdateFOVRendering))
	float FOV_Vertical = 20;

	/** Minimum distance, m */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = RadarParams, SaveGame, meta = (EditInRuntime, ReactivateComponent, UpdateFOVRendering))
	float DistanseMin = 0.2f;

	/** Maximum distance, m */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = RadarParams, SaveGame, meta = (EditInRuntime, ReactivateComponent, UpdateFOVRendering))
	float DistanseMax = 70;

	/** Maximum distance on full horizontal FOV, m */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = RadarParams, SaveGame, meta = (EditInRuntime, ReactivateComponent, UpdateFOVRendering))
	float DistanseOnMaxAngle = 40;

	/** Best horizontal resolution, deg */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = RadarParams, SaveGame, meta = (EditInRuntime, ReactivateComponent))
	float HorizontalBestResolution = 1.0f;

	/** Horizontal resolution on full distance FOV angle, deg */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = RadarParams, SaveGame, meta = (EditInRuntime, ReactivateComponent))
	float HorizontalResolutionFullDistAngle = 4.5f;

	/** Horizontal resolution on maximum angle FOV, deg */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = RadarParams, SaveGame, meta = (EditInRuntime, ReactivateComponent))
	float HorizontalResolutionMaxAngle = 12.3f;

	/** Minimum detecting signal RCS */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = RadarParams, SaveGame, meta = (EditInRuntime, ReactivateComponent))
	float MinSignalCoef = 0.002f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Debug, SaveGame, meta = (EditInRuntime))
	bool bShowDebugCapsules = false;

	/** [0..1] 0 - near, 1 - far */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Debug, SaveGame, meta = (EditInRuntime))
	float DebugCapsulesRang = 0.0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = FOVRenderer, SaveGame, meta = (EditInRuntime, UpdateFOVRendering))
	FSensorFOVRenderer FOVSetup { EFOVRenderingStrategy::OnSelect, FLinearColor(1.0, 0.8, 0.0, 1), 700 };

	int GetBeamsNum() const { return 2 * FOV_HorizontMax / HorizontalBestResolution; }
	float GetBeamWidth() const;
	float GetBeamHeight() const;
	float GetResolutionForAngle(float Angle) const;
	float GetRayLength(float HorizontAng) const;
};

/**
 * FRadarObjectCatagory
 */
USTRUCT(BlueprintType)
struct UNREALSODA_API FRadarObjectCatagory
{
	GENERATED_BODY()

	FRadarObjectCatagory() {}

	FRadarObjectCatagory(float RCS_, bool Blocking_) : RCS(RCS_), Blocking(Blocking_) {}

	/** Radar cross-section */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sensor, SaveGame, meta = (EditInRuntime))
	float RCS = 1.0f;

	/** Is object blocks radio wave */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sensor, SaveGame, meta = (EditInRuntime))
	bool Blocking = false;
};

/**
 * FRadarCluster
 */
struct UNREALSODA_API FRadarCluster
{
	/** Cluster center azimuth */
	float Azimuth;

	/** Cluster main distance */
	float Distance;

	/** Cluster total RCS */
	float RCS;

	/** Cluster Lateral velocity */
	float Lat;

	/** Cluster Longitudal velocity */
	float Lon;

	/** Cluster position in UE worldspace */
	FVector HitPosition;

	/** Cluster hits array */
	TArray<const FHitResult*> Hits;
};

/**
 * FRadarHit
 */
struct UNREALSODA_API FRadarHit
{
	const FHitResult* Hit;
	float Azimuth;
	float RCS;
	FVector HitPosition; // [cm]
	FVector LocalHitPosition; // [cm]
	FVector VelRelToRadar; // [m/s]
	float Resolution;
	ESegmObjectLabel ObjectCategory;
	float Distance;
	FTransform SensorView;
};

/**
 * FRadarClusters
 */
struct UNREALSODA_API FRadarClusters
{
	float MaxDistDiff = 100.0f; //Cluster distance deep
	TArray<FRadarCluster> Clusters; //Clusters array

	void Clear() { Clusters.Empty(); }
	void AddHit(const FRadarHit & Hit, const URadarBaseSensorComponent& Radar);
};

/**
 * FTrackingObjectData
 */
struct UNREALSODA_API FTrackingObjectData
{
	uint8 RadarObjectId = 0; // Radar tracking object id
	FVector2D PrevVelocity = FVector2D(0.f); //Previous relative Velocity
	FVector2D Acceleration = FVector2D(0.f); //Object relative Acceleration
	TTimestamp PrevUpdatedTimestamp{};
	int ProbabilityCounter = 0;
	int64 LifeCyclesCounter = 0;

	static const int kProbabilityCounterMax = 10; // TODO: Use Deltatime insted ProbabilityCounter/kProbabilityCounterMax
};

/**
 * FRadarObject
 */
struct UNREALSODA_API FRadarObject
{
public:
	float RCS = 0;
	float Lat = 0; //Lateral velocity [m/s]
	float Lon = 0; //Longitudal velocity [m/s]
	ESegmObjectLabel ObjectCategory;
	uint32 ObjectActorId = 0;
	AActor* ObjectActor = 0;
	FBox LocalBounds;
	uint8 RadarObjectId;
	TArray<const FHitResult*> Hits;
	FTrackingObjectData TrackingData;
	FVector BoundCenterActorLocal;
	bool bUpdated = false;

	float CachedDistance = 0;
	float CachedAzimuth = 0;
	FVector CachedObjectPoint;

	float GetDistance() const { return CachedDistance; }
	float GetAzimuth() const { return CachedAzimuth; }
	FVector GetObjectPoint() const { return CachedObjectPoint; }
};

/**
 * FRadarObjects
 */
struct UNREALSODA_API FRadarObjects
{
	/** Maximun objects number */
	int ObjectsMaxNum = 50;

	/** Object Ids map */
	TMap<uint32, FRadarObject> Objects;
	
	void ResetScan();
	void FinishScan(const URadarBaseSensorComponent& Radar);
	void AddHit(const FRadarHit& Hit, const URadarBaseSensorComponent & Radar);
	uint8 GetFreeRadarObjectId();
	uint8 GetRadarObjectIdForActorId(uint32 ActorId);
	void SortObjectsByDistance(TArray<FRadarObject* > & SortedObjects);
};

/**
 * URadarBaseSensorComponent
 * TODO: Separate the ClusterMode and the ObjectMode to differnts objects
 * TDDO: Pull out the ObjectCategories and all ***RCS properties to some global object
 * TODO: New FOVRendarare based on the real FRadarParams
 * TODO: Insted of using coliders it is may be batter to use the overlaping geometry (radar FOV mesh and another mesheas)
 */
UCLASS(Abstract, ClassGroup = Soda, BlueprintType, meta = (BlueprintSpawnableComponent))
class UNREALSODA_API URadarBaseSensorComponent : public USensorComponent
{
	GENERATED_UCLASS_BODY()

public:
	/** Data send period*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = RadarCommon, SaveGame, meta = (EditInRuntime))
	float Period = 0.07f;

	/** Objects categories */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = RadarCommon)
	TMap<ESegmObjectLabel, FRadarObjectCatagory> ObjectCategories;

	/** RCS of Buildings object category */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ObjectRCS, SaveGame, meta = (EditInRuntime, ReactivateComponent))
	float BuildingsRCS = 0.005;

	/** RCS of Fences object category */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ObjectRCS, SaveGame, meta = (EditInRuntime, ReactivateComponent))
	float FencesRCS = 0.2;

	/** RCS of Pedestrians object category */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ObjectRCS, SaveGame, meta = (EditInRuntime, ReactivateComponent))
	float PedestriansRCS = 1.0;

	/** RCS of Poles object category */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ObjectRCS, SaveGame, meta = (EditInRuntime, ReactivateComponent))
	float PolesRCS = 0.4;

	/** RCS of TrafficSigns object category */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ObjectRCS, SaveGame, meta = (EditInRuntime, ReactivateComponent))
	float TrafficSignsRCS = 0.5;

	/** RCS of Vegetation object category */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ObjectRCS, SaveGame, meta = (EditInRuntime, ReactivateComponent))
	float VegetationRCS = 0.1;

	/** RCS of Vehicles object category */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ObjectRCS, SaveGame, meta = (EditInRuntime, ReactivateComponent))
	float VehiclesRCS = 3.0;

	/** RCS of  Walls object category */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ObjectRCS, SaveGame, meta = (EditInRuntime, ReactivateComponent))
	float WallsRCS = 0.05;

	/** Show debug points */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Debug, SaveGame, meta = (EditInRuntime))
	bool bDrawDebugPoints = false;

	/** Show debug clusters RCS, hits num and distance */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Debug, SaveGame, meta = (EditInRuntime))
	bool bDrawDebugLabels = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Debug, SaveGame, meta = (EditInRuntime))
	bool bDebugLog = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Debug, SaveGame, meta = (EditInRuntime))
	float RCSObjectModeMultiplier = 6;

public:
	UFUNCTION(BlueprintCallable, Category = RadarCommon)
	virtual ERadarMode GetRadarMode() const { return ERadarMode::ObjectMode; }

public:
	URadarBaseSensorComponent();
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) override;
	virtual FBoxSphereBounds CalcBounds(const FTransform& LocalToWorld) const override;

protected:
	virtual bool OnActivateVehicleComponent() override;
	virtual void OnDeactivateVehicleComponent() override;
	virtual void GetRemark(FString& Info) const override {};
	virtual bool GenerateFOVMesh(TArray<FSensorFOVMesh>& Meshes) override;
	virtual bool NeedRenderSensorFOV() const;

protected:
	virtual bool AddFOVMesh(const FRadarParams & Params, TArray<FSensorFOVMesh>& OutMeshes);
	virtual void ProcessRadarBeams(const FRadarParams & Params);
	virtual const TArray<FRadarParams>& GetRadarParams() const { static TArray<FRadarParams> Params; return Params; }
	virtual void ProcessHit(const FHitResult* Hit, const FRadarParams * Params);
	virtual void PublishResults() {};
	virtual void ShowDebudPoints();

protected:
	uint16 MeasurementCounter = 0;
	FRadarClusters Clusters;
	FRadarObjects Objects;

	TTimestamp PrevTickTime;
	TArray<FVector> BatchStart;
	TArray<FVector> BatchEnd;
};
