// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "Soda/SodaTypes.h"
#include "Soda/Misc/Extent.h"
#include "Engine/EngineTypes.h"
#include "SodaStatics.generated.h"

class UProceduralMeshComponent;
class USodaGameModeComponent;

UENUM(BlueprintType)
enum class ETriangulationPlane : uint8
{
	XY,
	XZ,
	YZ
};

UENUM(BlueprintType)
enum class EActorTagMethod : uint8
{
	Auto,
	ByPrimitivePath,
	ForceCurrentStencilValue,
	ForceCustomLabel
};

UCLASS(Category=Soda)
class UNREALSODA_API USodaStatics : public UBlueprintFunctionLibrary
{
	GENERATED_UCLASS_BODY()

public:

	/** Try to find the Game World */
	static UWorld * GetGameWorld(const UObject* WorldContextObject=nullptr);

	UFUNCTION(BlueprintPure, Category = "Soda")
	static USodaGameModeComponent* GetSodaGameMode();

	/**Get current level name */
	UFUNCTION(BlueprintCallable, Category = "Soda")
	static FName GetLevelName(const UObject* WorldContextObject);

	/** Get all maps for project */
	UFUNCTION(BlueprintCallable, Category=Soda)
	static TArray<FString> GetAllMapPaths();

	/**
	 * Travel to another level
	 *
	 * @param	LevelName			the level to open
	 * @param	Options				a string of options to use for the travel URL
	 */
	UFUNCTION(BlueprintCallable, Category=Soda)
	static bool OpenLevel(const UObject* WorldContextObject, FName LevelPath, FString Options = FString(TEXT("")));

	/** Set the tag of an actor.
	If bTagForSemanticSegmentation true, activate the custom depth pass. This
	pass is necessary for rendering the semantic segmentation. However, it may
	add a performance penalty since occlusion doesn't seem to be applied to
	objects having this value active. */
	UFUNCTION(BlueprintCallable, Category=Soda)
	static void TagActor(AActor *Actor, bool bTagForSemanticSegmentation, EActorTagMethod TagMethod = EActorTagMethod::Auto, ESegmObjectLabel CustomLabel = ESegmObjectLabel::None);

	/** Set the tag of every actor in level.
	If bTagForSemanticSegmentation true, activate the custom depth pass. This
	pass is necessary for rendering the semantic segmentation. However, it may
	add a performance penalty since occlusion doesn't seem to be applied to
	objects having this value active. */
	UFUNCTION(BlueprintCallable, Category=Soda)
	static void TagActorsInLevel(UObject * WorldContextObject, bool bTagForSemanticSegmentation);


	/** Retrieve the tag of an already tagged component. */
	UFUNCTION(BlueprintCallable, Category=Soda)
	static ESegmObjectLabel GetTagOfTaggedComponent(UPrimitiveComponent *Component);

	/** Retrieve the tags of an already tagged actor. ESegmObjectLabel::None is
	not added to the array. */
	UFUNCTION(BlueprintCallable, Category=Soda)
	static void GetTagsOfTaggedActor(AActor *Actor, TSet<ESegmObjectLabel> &Tags);

	/** Return true if @a Component has been tagged with the given @a Tag. */
	UFUNCTION(BlueprintCallable, Category=Soda)
	static bool MatchComponent(UPrimitiveComponent *Component, ESegmObjectLabel Tag)
	{
		if(!Component) return false;
		return (Tag == GetTagOfTaggedComponent(Component));
	}
	UFUNCTION(BlueprintCallable, Category=Soda)
  	static FString GetTagAsString(ESegmObjectLabel Tag);

	/** Add V2V component to all vehicles in level.
	*/
	UFUNCTION(BlueprintCallable, Category = "Soda")
	static void AddV2XMarkerToAllVehiclesInLevel(UObject* WorldContextObject, TSubclassOf<AActor> VehicleClass);

	UFUNCTION(BlueprintCallable, Category=Soda)
	static bool WriteStringToFile(const FString & FileName, const FString & StringToWrite, bool AllowOwerWriting = true);

	UFUNCTION(BlueprintCallable, Category=Soda)
	static bool ReadStringFromFile(FString& Result, const FString & FileName);

	UFUNCTION(BlueprintCallable, Category=Soda)
	static bool SavePathToFile(USplineComponent * SplineComponent, float Distance, const FString & FileName, bool bIsLatLong);

	UFUNCTION(BlueprintCallable, Category=Soda)
  	static FString GetUnrealSodaVersion();

	UFUNCTION(BlueprintCallable, Category=Soda)
	static FString Int64ToString(int64 Value);

	UFUNCTION(BlueprintCallable, Category = "Soda")
	static void GetAllSubclassOfClass(const UClass * Class, TArray<UClass*> & SubClasses);

	UFUNCTION(BlueprintCallable, Category = "Soda")
	static UObject * GetDefaultObject(const UClass * Class);

	UFUNCTION(BlueprintCallable, Category = "Soda")
	static UActorComponent* FindDefaultComponentByClass(
		const TSubclassOf<AActor> InActorClass,
		const TSubclassOf<UActorComponent> InComponentClass);

	UFUNCTION(BlueprintCallable, Category = "Soda")
	static bool DelaunayTriangulation(
		const TArray<FVector>& OutsidePolyline,
		const TArray<FVector>& HolePolyline,
		const TArray<FVector>& AdditionalPoints,
		TArray<int32>& OutTriangles,
		TArray<FVector>& OutVertices,
		ETriangulationPlane Plane = ETriangulationPlane::XY,
		bool bFlip = false);

	UFUNCTION(BlueprintCallable, Category = "Soda")
	static FName GetSensorShortName(FName FullName);

	UFUNCTION(BlueprintCallable, Category = "Soda")
	static float FindSplineInputKeyClosestToWorldLocationFast(const FVector& WorldLocation, int32& StartSegment, USplineComponent* Spline);

	UFUNCTION(BlueprintCallable, Category = "Soda")
	static bool IsBlueprintClass(const UClass* Class) { return Class && Class->HasAnyClassFlags(EClassFlags::CLASS_CompiledFromBlueprint); }

	UFUNCTION(BlueprintCallable, Category = "Soda")
	static float DrawProgress(
		UCanvas* Canvas,
		float XPos,
		float YPos,
		float Width,
		float Height,
		float Value,
		float Min,
		float Max,
		bool bAlingCenter = false,
		const FString& TextValue = ""
	);

	UFUNCTION(BlueprintCallable, Category=Soda)
	static FBox ExtentToBox(const FExtent& Extent) { return Extent.ToBox(); }

	UFUNCTION(BlueprintCallable, Category=Soda)
	static FBoxSphereBounds ExtentToBoxSphereBounds(const FExtent& Extent) { return Extent.ToBoxSphereBounds(); }

	UFUNCTION(BlueprintCallable, Category=Soda)
	static void SetFKeyByName(FString& Name, FKey& Key);

	UFUNCTION(BlueprintCallable, Category=Soda)
	static float GetDistanceAlongSpline(float InputKey, USplineComponent* InSplineComponent);

	UFUNCTION(BlueprintCallable, Category=Soda)
	static bool GetShowFPS(UObject* WorldContextObject);

	UFUNCTION(BlueprintCallable, Category=Soda)
	static bool SetShowFPS(UObject* WorldContextObject, bool ShowFps);

	UFUNCTION(BlueprintCallable, Category=Soda)
	static bool DeleteFile(const FString& FileName);

	UFUNCTION(BlueprintCallable, Category=Soda)
	static UObject* GetSubobjectByName(const UObject* Parent, FName ObjectName);

	UFUNCTION(BlueprintCallable, Category=Soda)
	static TArray<UObject*> FindAllObjectsByClass(UObject* WorldContextObject, const UClass* Class);

	UFUNCTION(BlueprintCallable, Category=Soda)
	static AActor * FindActoByName(UObject* WorldContextObject, FName Name);

	UFUNCTION(BlueprintCallable, Category=Soda)
	static void SetSynchronousMode(bool bEnable, float DeltaSeconds);

	UFUNCTION(BlueprintCallable, Category=Soda)
	static bool IsSynchronousMode();

	UFUNCTION(BlueprintCallable, Category=Soda)
	static int GetFrameIndex();

	UFUNCTION(BlueprintCallable, Category=Soda)
	static bool SynchTick(int64& Timestamp, int& FramIndex);

	UFUNCTION(BlueprintCallable, Category=Soda)
	static bool UploadVehicleJSON(const FString & JSONStr, const FString & VehicleName);

	UFUNCTION(BlueprintCallable, Category=Soda)
	static UProceduralMeshComponent* GenerateRoadLine(
		UProceduralMeshComponent* LaneMesh,
		int SectionID,
		const USplineComponent* SplineComponent,
		//float Offset,
		float Width,
		float LineLen,
		float LineGap
		//float Start,
		//float End
	);

	UFUNCTION(BlueprintCallable, Category=Soda)
	static FBox CalculateActorExtent(const AActor* Actor);
};
