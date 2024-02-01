// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "GameFramework/Actor.h"
#include "ProceduralMeshComponent.h"
#include "Soda/Actors/NavigationRoute.h"
#include "Soda/IToolActor.h"
#include "TrackBuilder.generated.h"

class FJsonObject;

/**
 * ATrackBuilder (Experimental)
 * Procedural generation of racing tracks based on a JSON custom format.
 * Dev notes: If the tool turns out to be useful, need to think about a way to create JSON tracks from Google maps.
 * TODO: Need to check the functionality after switching to a new triangulation algorithm.
 */
UCLASS(ClassGroup = Soda, meta = (BlueprintSpawnableComponent))
class UNREALSODA_API ATrackBuilder : 
	public AActor,
	public IToolActor
{
	GENERATED_BODY()

public:
	ATrackBuilder();

	/** JSON file path to open in editor */
	UPROPERTY(EditAnywhere, Category = LoadJsonTrack)
	FString FileName;

	/** Whether to import altitude from JSON file */
	UPROPERTY(EditAnywhere, Category = LoadJsonTrack, SaveGame, meta = (EditInRuntime))
	bool bImportAltitude = false;

	UPROPERTY(EditAnywhere, Category = LoadJsonTrack, SaveGame, meta = (EditInRuntime))
	bool bRelax = false;

	UPROPERTY(EditAnywhere, Category = LoadJsonTrack, SaveGame, meta = (EditInRuntime))
	float RelaxSpeed = 0.5;

	UPROPERTY(EditAnywhere, Category = LoadJsonTrack, SaveGame, meta = (EditInRuntime))
	float RelaxIterations = 10;

	UPROPERTY(EditAnywhere, Category = LoadJsonTrack, SaveGame, meta = (EditInRuntime))
	float RelaxTargetAngel = 100;

	UPROPERTY(EditAnywhere, Category = LoadJsonTrack, SaveGame, meta = (EditInRuntime))
	bool bIsClosedTrack = true;

	/** The minimum distance between any two vertices of generated track[cm] or -1 if not used*/
	UPROPERTY(EditAnywhere, Category = TrackBuilder, SaveGame, meta = (EditInRuntime))
	float TrackMinSegmentLength = 100;

	/** Whether to generate borders (right and left indent  of the track) */
	UPROPERTY(EditAnywhere, Category = TrackBuilder, SaveGame, meta = (EditInRuntime))
	bool bWithBorder = true;

	UPROPERTY(EditAnywhere, Category = TrackBuilder, SaveGame, meta = (EditInRuntime))
	float BorderWidth = 150.0;

	UPROPERTY(EditAnywhere, Category = TrackBuilder, SaveGame, meta = (EditInRuntime))
	float TrackElevation = 10;

	/** The track is divided into segments. TrackSegmentStep - how many vertices of the center line should be included in one segment. */
	UPROPERTY(EditAnywhere, Category = TrackBuilder, SaveGame, meta = (EditInRuntime))
	int TrackSegmentStep  = 50;

	/** How many segments to build. If -1 then build all segments. */
	UPROPERTY(EditAnywhere, Category = TrackBuilder, SaveGame, meta = (EditInRuntime))
	int TrackBuildSegmentCount  = -1;

	/** Multiplier of V texture coordinates. */
	UPROPERTY(EditAnywhere, Category = TrackBuilder, SaveGame, meta = (EditInRuntime))
	float TrackTexScaleV = 0.0008;

	/** Part of U texture (0...1) that will be allocated to the border.  */
	UPROPERTY(EditAnywhere, Category = TrackBuilder, SaveGame, meta = (EditInRuntime))
	float TrackTexBorderPartU = 0.1;

	/** Generate track with center line. */
	UPROPERTY(EditAnywhere, Category = TrackBuilder, SaveGame, meta = (EditInRuntime))
	bool bTrackWithCenterLine = true;

	UPROPERTY(EditAnywhere, Category = TrackBuilder)
	class UMaterialInterface* TrackMaterial = nullptr;

	/** Generate track mesh during OnBegin. */
	UPROPERTY(EditAnywhere, Category = TrackBuilder, SaveGame, meta = (EditInRuntime))
	bool bGenerateTrackOnBeginPlay = true;

	/** The minimum distance between any two vertices of generated route[cm] or -1 if not used*/
	UPROPERTY(EditAnywhere, Category = RoutesBuilder, SaveGame, meta = (EditInRuntime))
	float RouteMinSegmentLength = 500;

	/** Generate routes  during OnBegin. */
	UPROPERTY(EditAnywhere, Category = RoutesBuilder, SaveGame, meta = (EditInRuntime))
	bool bGenerateLineRoutesOnBeginPlay = true;

	UPROPERTY(EditAnywhere, Category = RoutesBuilder, SaveGame, meta = (EditInRuntime))
	bool bGenerateRaceRouteOnBeginPlay = false;

	UPROPERTY(EditAnywhere, Category = RoutesBuilder, SaveGame, meta = (EditInRuntime))
	int NumRouteLanes = 1;

	UPROPERTY(EditAnywhere, Category = RoutesBuilder, SaveGame, meta = (EditInRuntime))
	FVector RouteOffset;

	UPROPERTY(EditAnywhere, Category = MarkingsBuilder, SaveGame, meta = (EditInRuntime))
	float MarkingsMinSegmentLength = 30;

	UPROPERTY(EditAnywhere, Category = MarkingsBuilder, SaveGame, meta = (EditInRuntime))
	float MarkingsWidth = 20;
	
	UPROPERTY(EditAnywhere, Category = MarkingsBuilder, SaveGame, meta = (EditInRuntime))
	float MarkingsTexScaleV = 0.004;
		
	UPROPERTY(EditAnywhere, Category = MarkingsBuilder, SaveGame, meta = (EditInRuntime))
	int MarkingsStep = 200;

	/** Generate markings  during OnBegin. */
	UPROPERTY(EditAnywhere, Category = MarkingsBuilder, SaveGame, meta = (EditInRuntime))
	bool bGeneratMarkingsOnBeginPlay = true;

	UPROPERTY(EditAnywhere, Category = MarkingsBuilder)
	class UMaterialInterface* MarkingsMaterial = nullptr;

	UPROPERTY(EditAnywhere, Category = StartPoint)
	class UMaterialInterface* DecalMaterial = nullptr;

	UPROPERTY(EditAnywhere, Category = StartPoint)
	float StartLineOffset = 300;

	UPROPERTY(EditAnywhere, Category = StartPoint)
	float StartLineWidth = 50;

	/** Generate start line during OnBegin. */
	UPROPERTY(EditAnywhere, Category = StartPoint)
	bool bGeneratStartLineOnBeginPlay = true;

	UPROPERTY(EditAnywhere, Category = Debug, SaveGame, meta = (EditInRuntime))
	bool bDrawDebug = false;

public:
	UPROPERTY(BlueprintReadOnly, Category = TrackBuilder, SaveGame)
	TArray<FVector> OutsidePoints;

	UPROPERTY(BlueprintReadOnly, Category = TrackBuilder, SaveGame)
	TArray<FVector> InsidePoints;

	UPROPERTY(BlueprintReadOnly, Category = TrackBuilder, SaveGame)
	TArray<FVector> CentrePoints;

	UPROPERTY(BlueprintReadOnly, Category = TrackBuilder, SaveGame)
	TArray<FVector> RacingPoints;

	UPROPERTY(BlueprintReadOnly, Category = TrackBuilder, SaveGame)
	bool bIsClockwise2D = false;

	UPROPERTY(BlueprintReadOnly, Category = RefPoint, SaveGame)
	bool bRefPointIsOk = false;

	UPROPERTY(BlueprintReadOnly, Category = LoadJsonTrack, SaveGame)
	bool bJSONLoaded = false;

	UPROPERTY(BlueprintReadOnly, Category = LoadJsonTrack, SaveGame)
	FString LoadedFileName;

	UPROPERTY(BlueprintReadOnly, Category = TrackBuilder, Transient)
	TArray<UProceduralMeshComponent*> TrackMeshes;

	UPROPERTY(BlueprintReadOnly, Category = MarkingsBuilder, Transient)
	TArray<UProceduralMeshComponent*> MarkingMeshes;

	UPROPERTY(Category = RoutesBuilder, BlueprintReadOnly, Transient)
	TArray<ANavigationRoute*> Routes;

	UPROPERTY(Category = StartPint, BlueprintReadOnly, Transient)
	UDecalComponent * StartLineDecal = nullptr;

	UPROPERTY(SaveGame)
	double RefPointLon = 0;

	UPROPERTY(SaveGame)
	double RefPointLat = 0;

	UPROPERTY(SaveGame)
	double RefPointAlt = 0;

public:
	UFUNCTION(BlueprintCallable, Category = LoadJsonTrack)
	bool LoadJsonFromFile(const FString& InFileName);

	UFUNCTION(CallInEditor, Category = LoadJsonTrack, meta = (DisplayName="Load JSON", CallInRuntime))
	void LoadJson();

	UFUNCTION(CallInEditor, BlueprintCallable, Category = LoadJsonTrack, meta = (CallInRuntime))
	void UnloadJson();

	UFUNCTION(CallInEditor, BlueprintCallable, Category = LoadJsonTrack, meta = (CallInRuntime))
	void GenerateAll();

	UFUNCTION(CallInEditor, BlueprintCallable, Category = TrackBuilder, meta = (CallInRuntime))
	void GenerateTrack();

	UFUNCTION(CallInEditor, BlueprintCallable, Category = TrackBuilder, meta = (CallInRuntime))
	void ClearTrack();

	UFUNCTION(CallInEditor, BlueprintCallable, Category = RoutesBuilder, meta = (CallInRuntime))
	void GenerateLineRoutes();

	UFUNCTION(CallInEditor, BlueprintCallable, Category = RoutesBuilder, meta = (CallInRuntime))
	void GenerateRaceRoute();

	UFUNCTION(CallInEditor, BlueprintCallable, Category = RoutesBuilder, meta = (CallInRuntime))
	void ClearRoutes();

	UFUNCTION(CallInEditor, BlueprintCallable, Category = MarkingsBuilder, meta = (CallInRuntime))
	void GenerateMarkings();

	UFUNCTION(CallInEditor, BlueprintCallable, Category = MarkingsBuilder, meta = (CallInRuntime))
	void ClearMarkings();

	UFUNCTION(CallInEditor, BlueprintCallable, Category = RefPoint, meta = (CallInRuntime))
	void UpdateGlobalRefpoint();

	UFUNCTION(CallInEditor, BlueprintCallable, Category = StartPoint, meta = (CallInRuntime))
	void UpdateActorsFactory();

	UFUNCTION(CallInEditor, BlueprintCallable, Category = StartPoint, meta = (CallInRuntime))
	void GenerateStartLine();

	UFUNCTION(CallInEditor, BlueprintCallable, Category = StartPoint, meta = (CallInRuntime))
	void ClearStartLine();

public:
	/* Override from ISodaActor */
	//virtual void OnSelect_Implementation() override;
	//virtual void OnUnselect_Implementation() override;
	virtual const FSodaActorDescriptor* GenerateActorDescriptor() const override;
	//virtual bool OnSetPinnedActor(bool bIsPinnedActor) override;
	//virtual bool IsPinnedActor() const override;
	virtual TSharedPtr<SWidget> GenerateToolBar();

public:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickActor(float DeltaTime, enum ELevelTick TickType, FActorTickFunction& ThisTickFunction) override;
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void Serialize(FArchive& Ar) override { Super::Serialize(Ar); ToolActorSerialize(Ar); }

protected:
	static bool JSONReadLine(const TSharedPtr<FJsonObject>& JSON, const FString& FieldName, TArray<FVector>& OutPoints, bool ImportAltitude);
	bool GenerateMarkingsInner(const TArray<FVector>& Points);
};
