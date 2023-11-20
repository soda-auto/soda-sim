// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "GameFramework/Actor.h"
#include "Components/BillboardComponent.h"
#include "Components/SceneComponent.h"
#include "CoreMinimal.h"
#include "ProceduralMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Soda/Actors/NavigationRoute.h"
#include "Soda/IToolActor.h"
#include "Soda/SodaTypes.h"
#include "OpenDriveTool.generated.h"

namespace opendrive
{
	struct OpenDriveData;
};

USTRUCT(BlueprintType, Blueprintable)
struct FOpenDriveRoadMarkProfile
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, Category = RoadMarkProfile, meta = (EditInRuntime))
	FString MarkType;

	UPROPERTY(EditAnywhere, Category = RoadMarkProfile, meta = (EditInRuntime))
	ESegmObjectLabel Label;

	UPROPERTY(EditAnywhere, Category = RoadMarkProfile, meta = (EditInRuntime))
	FColor Color;

	UPROPERTY(EditAnywhere, Category = RoadMarkProfile, meta = (EditInRuntime))
	float Width;

	UPROPERTY(BlueprintReadOnly, Category = Sensor)
	UMaterialInstanceDynamic* Material = nullptr;

	FOpenDriveRoadMarkProfile();
	FOpenDriveRoadMarkProfile(const FString & MarakType, ESegmObjectLabel Label, const FColor & Color, float Width);
};

/**
 * AOpenDriveTool
 * Used for procedural generation of elements at a level based on the OpenDrive file format.
 * Allows to generate road markings and ANavigationRoute(s).
 * Can be useful for generating road markings for semantic segmentation for cameras.
 * TODO: Generate NavigationRoute with branches
 * TODO: Generate NavigationRoute with directions
 */
UCLASS()
class UNREALSODA_API AOpenDriveTool : 
	public AActor,
	public IToolActor
{
GENERATED_BODY()

protected:
	UBillboardComponent *SpriteComponent;
	UTexture2D *SpriteTexture;

public:
	UPROPERTY(Category = "GenerateMarks", EditAnywhere)
	bool bAllowGenerateMarksInEditor = false;

	/** [cm] */
	UPROPERTY(Category = "GenerateMarks", BlueprintReadWrite, EditAnywhere, SaveGame, meta = (EditInRuntime))
	float MarkAccuracy = 100;

	/** [cm] */
	UPROPERTY(Category = "GenerateMarks", BlueprintReadWrite, EditAnywhere, SaveGame, meta = (EditInRuntime))
	float MarkHeight = 3;

	UPROPERTY(Category = "GenerateMarks", BlueprintReadWrite, EditAnywhere, SaveGame, meta = (EditInRuntime))
	bool bMarkOptimizeMesh = true;

	UPROPERTY(Category = "GenerateMarks", BlueprintReadWrite, EditAnywhere)
	bool bMarkDrawDebug = false;

	UPROPERTY(Category = "GenerateMarks", EditAnywhere, SaveGame, meta = (EditInRuntime))
	bool bMarkGenerateOnBeginPlay = false;

	UPROPERTY(Category = "GenerateMarks", EditAnywhere, SaveGame, meta = (EditInRuntime))
	TArray<FOpenDriveRoadMarkProfile> MarkProfile;

	UPROPERTY(Category = "GenerateMarks", EditAnywhere)
	UMaterialInterface* MarkMaterial;

	UPROPERTY(Category = "GenerateMarks", BlueprintReadOnly, Transient)
	TArray<UProceduralMeshComponent*> MarkMeshes;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Dataset, SaveGame, meta = (EditInRuntime))
	bool bRecordDataset = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Dataset, SaveGame, meta = (EditInRuntime))
	bool bStoreLanesMarks = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Dataset, SaveGame, meta = (EditInRuntime))
	bool bStoreXDOR = false;

public:
	/** [cm] */
	UPROPERTY(Category = "GenerateRoutes", BlueprintReadWrite, EditAnywhere)
	float RouteAccuracy = 300;

	/** [cm] */
	UPROPERTY(Category = "GenerateRoutes", BlueprintReadWrite, EditAnywhere)
	float RoutesHeight = 100;

	UPROPERTY(Category = "GenerateRoutes", BlueprintReadOnly, Transient)
	TArray<ANavigationRoute*> Routes;

public:
	UFUNCTION(Category = "GenerateMarks", CallInEditor, meta = (CallInRuntime, DisplayName = "Build Road Marks"))
	void BuildRoadMarks_Editor() { BuildRoadMarks(); }

	UFUNCTION(Category = "GenerateMarks", BlueprintCallable, CallInEditor)
	void DrawDebugMarks() const;

	UFUNCTION(Category = "GenerateRoutes", CallInEditor, meta = (DisplayName = "Build Routes"))
	void BuildRoutes_Editor() { BuildRoutes(); }

	UFUNCTION(Category = "GenerateMarks", BlueprintCallable)
	bool BuildRoadMarks();

	UFUNCTION(Category = "GenerateRoutes", BlueprintCallable)
	bool BuildRoutes();

	UFUNCTION(Category = "GenerateMarks", BlueprintCallable, CallInEditor, meta = (CallInRuntime))
	void ClearMarkings();

	UFUNCTION(Category = "GenerateRoutes", BlueprintCallable, CallInEditor)
	void ClearRoutes();

public:
	AOpenDriveTool(const FObjectInitializer &ObjectInitializer);
	bool FindAndLoadXDOR(bool bForceReload);

public:
	/* Override from ISodaActor */
	//virtual void OnSelect_Implementation() override;
	//virtual void OnUnselect_Implementation() override;
	virtual const FSodaActorDescriptor * GenerateActorDescriptor() const override;
	//virtual bool OnSetPinnedActor(bool bIsPinnedActor) override;
	//virtual bool IsPinnedActor() const override;
	virtual void ScenarioBegin() override;
	virtual void ScenarioEnd() override;

public:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickActor(float DeltaTime, enum ELevelTick TickType, FActorTickFunction& ThisTickFunction) override;
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void Serialize(FArchive& Ar) override { Super::Serialize(Ar); ToolActorSerialize(Ar); }

#if WITH_EDITOR
	void PostEditChangeProperty(struct FPropertyChangedEvent &PropertyChangedEvent);

#endif // WITH_EDITOR

protected:
	FOpenDriveRoadMarkProfile* FindMarkProfile(const FString& MarakType);
	FString FindXDORFile();

protected:
	TArray<TArray<FVector>> DebugMarkLines;
	TSharedPtr<opendrive::OpenDriveData> OpenDriveData;
};
