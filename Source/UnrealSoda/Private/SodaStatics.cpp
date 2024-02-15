// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/SodaStatics.h"
#include "Soda/UnrealSoda.h"
#include "Soda/SodaApp.h"
#include "Soda/VehicleComponents/VehicleSensorComponent.h"
#include "Soda/VehicleComponents/Sensors/Base/V2XSensor.h"
#include "Soda/SodaGameMode.h"
#include "Soda/LevelState.h"
#include "Soda/UnrealSodaVersion.h"
#include "Soda/SodaUserSettings.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SplineComponent.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"
#include "ProceduralMeshComponent.h"
#include "Engine/Engine.h"
#include "CanvasItem.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Interfaces/IPluginManager.h"
#include "Engine/SCS_Node.h"
#include "Engine/StaticMesh.h"
#include "Engine/ObjectLibrary.h"
#include "HAL/PlatformFileManager.h"
#include "PhysicsEngine/PhysicsAsset.h"
#include "DynamicMeshBuilder.h"
#include "UObject/UObjectIterator.h"
#include "Engine/GameViewportClient.h"

#include <iostream>
#include <fstream>
#include <map>

template < typename T >
static auto CastEnum(T label)
{
	return static_cast<typename std::underlying_type< T >::type>(label);
}

static ESegmObjectLabel GetLabelByFolderName(const FString& String)
{
	if (String == "Buildings")
		return ESegmObjectLabel::Buildings;
	else if (String == "Fences")
		return ESegmObjectLabel::Fences;
	else if (String == "Pedestrians")
		return ESegmObjectLabel::Pedestrians;
	else if (String == "Pole")
		return ESegmObjectLabel::Poles;
	else if (String == "Props")
		return ESegmObjectLabel::Other;
	else if (String == "Road")
		return ESegmObjectLabel::Roads;
	else if (String == "RoadLines")
		return ESegmObjectLabel::RoadLines;
	else if (String == "SideWalk")
		return ESegmObjectLabel::Sidewalks;
	else if (String == "TrafficSigns")
		return ESegmObjectLabel::TrafficSigns;
	else if (String == "Vegetation")
		return ESegmObjectLabel::Vegetation;
	else if (String == "Vehicles")
		return ESegmObjectLabel::Vehicles;
	else if (String == "Walls")
		return ESegmObjectLabel::Walls;

	/*Soda Types*/
	else if (String == "RoadLines_Solid")
		return ESegmObjectLabel::RoadLines_Solid;
	else if (String == "RoadLines_Dashed")
		return ESegmObjectLabel::RoadLines_Dashed;
	else if (String == "RoadLines_Crosswalk")
		return ESegmObjectLabel::RoadLines_Crosswalk;
	else if (String == "RoadLines_ZigZag")
		return ESegmObjectLabel::RoadLines_ZigZag;
	else if (String == "RoadLines_DoubleSide")
		return ESegmObjectLabel::RoadLines_DoubleSide;
	else if (String == "RoadLines_Stop")
		return ESegmObjectLabel::RoadLines_Stop;
	else if (String == "RoadLines_DoubleDashed")
		return ESegmObjectLabel::RoadLines_DoubleDashed;
	else if (String == "RoadLines_Nondrivable")
		return ESegmObjectLabel::RoadLines_Nondrivable;
	else
		return ESegmObjectLabel::None;
}

template < typename T >
static ESegmObjectLabel GetLabelByPath(const T* Object)
{
	if (Object)
	{
		const FString Path = Object->GetPathName();
		TArray< FString > StringArray;
		Path.ParseIntoArray(StringArray, TEXT("/"), false);
		for (int i = 1; i < StringArray.Num() - 1; ++i)
		{
			ESegmObjectLabel Label = GetLabelByFolderName(StringArray[i]);
			if (Label != ESegmObjectLabel::None)
				return Label;
		}
	}
	return ESegmObjectLabel::None;
}

static void SetStencilValue(UPrimitiveComponent& Component, const ESegmObjectLabel& Label, const bool bSetRenderCustomDepth)
{
	Component.SetCustomDepthStencilValue(CastEnum(Label));
	Component.SetRenderCustomDepth(bSetRenderCustomDepth && (Label != ESegmObjectLabel::None));
}

USodaStatics::USodaStatics(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

FString USodaStatics::GetUnrealSodaVersion() 
{
	return UNREALSODA_VERSION_STRING; 
}

UWorld* USodaStatics::GetGameWorld(const UObject* WorldContextObject)
{
	if (WorldContextObject)
	{
		UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull);
		if (World) return World;
	}

	const TIndirectArray<FWorldContext>& WorldContexts = GEngine->GetWorldContexts();
	for (auto& WorldContext : WorldContexts)
	{
		switch (WorldContext.WorldType)
		{
		case EWorldType::PIE:
		case EWorldType::Game:
			UWorld* World = WorldContext.World();
			if (World && World->bBegunPlay)
			{
				return World;
			}
			break;
		}
	}
	return nullptr;
}

USodaGameModeComponent* USodaStatics::GetSodaGameMode()
{
	return USodaGameModeComponent::Get();
}

USodaUserSettings* USodaStatics::GetSodaUserSettings()
{
	return SodaApp.GetSodaUserSettings();
}

FName USodaStatics::GetLevelName(const UObject* WorldContextObject)
{
	if (UWorld* World = GetGameWorld(WorldContextObject))
	{
		return FName(*World->GetName());
	}
	return NAME_None;
}

TArray<FString> USodaStatics::GetAllMapPaths()
{
	TArray<FString> Ret;

	auto ObjectLibrary = UObjectLibrary::CreateLibrary(UWorld::StaticClass(), false, true);
	ObjectLibrary->LoadAssetDataFromPath(TEXT("/Game"));
	TArray<FAssetData> AssetDatas;
	ObjectLibrary->GetAssetDataList(AssetDatas);
	
	for (int32 i = 0; i < AssetDatas.Num(); ++i)
	{
		FAssetData& AssetData = AssetDatas[i];

		FString LevelName = AssetData.AssetName.ToString();

		if (LevelName.StartsWith(TEXT("__"), ESearchCase::CaseSensitive))
		{
			continue;
		}

		Ret.Add(AssetData.PackagePath.ToString() / LevelName);

	}
	return Ret;
}

bool USodaStatics::OpenLevel(const UObject* WorldContextObject, FName LevelPath, FString Options)
{
	UWorld* World = GetGameWorld(WorldContextObject);
	if (!World)
	{
		return false;
	}

	TArray<FString> LevelsPath = USodaStatics::GetAllMapPaths();
	for (auto& l : LevelsPath)
	{
		if (l.Contains(LevelPath.ToString()))
		{
			UGameplayStatics::OpenLevel(World, LevelPath, true, Options);
			return true;
		}
	}

	UE_LOG(LogSoda, Error, TEXT("USodaStatics::OpenLevel(%s), The level isn't found"), *LevelPath.ToString());
	return false;
}


void USodaStatics::TagActor(AActor* Actor, bool bTagForSemanticSegmentation, EActorTagMethod TagMethod, ESegmObjectLabel CustomLabel)
{
	if (!Actor)
		return;

	TArray< UPrimitiveComponent* > PrimitiveComponents;
	Actor->GetComponents< UPrimitiveComponent >(PrimitiveComponents);
	for (UPrimitiveComponent* Component : PrimitiveComponents)
	{
		ESegmObjectLabel Label = ESegmObjectLabel::None;
		switch (TagMethod)
		{
		    case EActorTagMethod::Auto:
				if (Component->ComponentTags.Find(TEXT("CusomSegmTag")) != INDEX_NONE)
				{
					Label = static_cast<ESegmObjectLabel> (Component->CustomDepthStencilValue);
					if (Label != ESegmObjectLabel::None) break;
				}
			case EActorTagMethod::ByPrimitivePath: 
				if (UStaticMeshComponent* MeshComponent = Cast<UStaticMeshComponent>(Component))
				{
					Label = GetLabelByPath(MeshComponent->GetStaticMesh().Get());
				}
				else if (USkeletalMeshComponent* SkeletalMeshComponent = Cast<USkeletalMeshComponent>(Component))
				{
					Label = GetLabelByPath(SkeletalMeshComponent->GetPhysicsAsset());
				}
				break;
			case EActorTagMethod::ForceCurrentStencilValue: 
				Label = static_cast<ESegmObjectLabel> (Component->CustomDepthStencilValue); 
				break;
			case EActorTagMethod::ForceCustomLabel: 
				Label = CustomLabel; 
				break;
		}
		if (TagMethod == EActorTagMethod::ForceCurrentStencilValue || TagMethod == EActorTagMethod::ForceCustomLabel)
		{
			Component->ComponentTags.AddUnique(TEXT("CusomSegmTag"));
		}
		SetStencilValue(*Component, Label, bTagForSemanticSegmentation);
		Component->SetCollisionResponseToChannel(SodaApp.GetSodaUserSettings()->RadarCollisionChannel, ECollisionResponse::ECR_Overlap);
	}
}

void USodaStatics::TagActorsInLevel(UObject* WorldContextObject, bool bTagForSemanticSegmentation)
{
	if (UWorld* World = GetGameWorld(WorldContextObject))
	{
		for (TActorIterator< AActor > it(World); it; ++it)
		{
			TagActor(*it, bTagForSemanticSegmentation);
		}
	}
}

void USodaStatics::GetTagsOfTaggedActor(AActor* Actor, TSet< ESegmObjectLabel >& Tags)
{
	if (!Actor)
		return;
	TArray< UPrimitiveComponent* > Components;
	Actor->GetComponents< UPrimitiveComponent >(Components);
	for (auto* Component : Components)
	{
		if (Component != nullptr)
		{
			const auto Tag = GetTagOfTaggedComponent(Component);
			if (Tag != ESegmObjectLabel::None)
			{
				Tags.Add(Tag);
			}
		}
	}
}

FString USodaStatics::GetTagAsString(const ESegmObjectLabel Label)
{
#define GET_LABEL_STR(lbl)      \
	case ESegmObjectLabel::lbl: \
		return TEXT(#lbl);
	switch (Label)
	{
		GET_LABEL_STR(None)
			GET_LABEL_STR(Buildings)
			GET_LABEL_STR(Fences)
			GET_LABEL_STR(Other)
			GET_LABEL_STR(Pedestrians)
			GET_LABEL_STR(Poles)
			GET_LABEL_STR(RoadLines)
			GET_LABEL_STR(Roads)
			GET_LABEL_STR(Sidewalks)
			GET_LABEL_STR(TrafficSigns)
			GET_LABEL_STR(Vegetation)
			GET_LABEL_STR(Vehicles)
			GET_LABEL_STR(Walls)
	default:
		return "None";
	}
#undef GET_LABEL_STR
}

void USodaStatics::AddV2XMarkerToAllVehiclesInLevel(UObject* WorldContextObject, TSubclassOf<AActor> VehicleClass)
{
	UWorld* World = GetGameWorld(WorldContextObject);
	if (!World) return;

	TArray<AActor*> FoundVehicles;
	UGameplayStatics::GetAllActorsOfClass(World, VehicleClass, FoundVehicles);
	if (FoundVehicles.Num() == 0)
	{
		UE_LOG(LogSoda, Log, TEXT("USodaStatics::AddV2XMarkerToAllVehiclesInLevel - no vehicles found"));
	}

	for (auto& Vehicle : FoundVehicles)
	{
		if (Vehicle->GetComponentByClass(UV2XMarkerSensor::StaticClass()) == 0)
		{
			FName V2XObjectName("V2X");

			UV2XMarkerSensor* NewV2XComp = NewObject<UV2XMarkerSensor>(Vehicle, V2XObjectName);
			check(NewV2XComp);

			if (Vehicle->GetComponentByClass(UV2XReceiverSensor::StaticClass()) != 0)
			{
				NewV2XComp->DeactivateVehicleComponent();
			}

			NewV2XComp->RegisterComponent();       
			NewV2XComp->AttachToComponent(Vehicle->GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
			NewV2XComp->CalculateBoundingBox();
			if (!NewV2XComp->bAssignUniqueIdAtStartup)
			{
				NewV2XComp->AssignUniqueId();
			}
			UE_LOG(LogSoda, Log, TEXT("AddV2XMarkerToAllVehiclesInLevel(), V2X component added. Id = %d"), NewV2XComp->ID);
		}
	}
}

bool USodaStatics::WriteStringToFile(const FString& FileName, const FString& StringToWrite, bool AllowOwerWriting)
{
	if (!AllowOwerWriting)
	{
		if (IFileManager::Get().FileExists(*FileName))
		{
			return false;
		}
	}

	return FFileHelper::SaveStringToFile(StringToWrite, *FileName);
}

bool USodaStatics::ReadStringFromFile(FString& Result, const FString& FileName)
{
	if (!IFileManager::Get().FileExists(*FileName))
	{
		return false;
	}

	return FFileHelper::LoadFileToString(Result, *FileName);
}

bool USodaStatics::SavePathToFile(USplineComponent* SplineComponent, float Distance, const FString& FileName, bool bIsLatLong)
{
	ALevelState* LevelState = ALevelState::Get();

	if (!SplineComponent || !LevelState) return false;

	std::ofstream OutFile;

	try
	{
		OutFile.open(TCHAR_TO_UTF8(*FileName));
	}
	catch (...)
	{
		UE_LOG(LogSoda, Error, TEXT("Can't open \"%s\" "), *FileName);
		return false;
	}

	float SplineLength = SplineComponent->GetSplineLength();

	try
	{
		if (!bIsLatLong)
		{
			OutFile << "X,Y,Z\n";
			for (int i = 0; i < SplineLength / Distance; ++i)
			{
				FVector Pt = SplineComponent->GetLocationAtDistanceAlongSpline(i * Distance, ESplineCoordinateSpace::World);
				OutFile << Pt.X << "," << Pt.Y << "," << Pt.Z << "\n";
			}
		}
		else
		{
			OutFile << "Lon,Lat,Alt\n";
			for (int i = 0; i < SplineLength / Distance; ++i)
			{
				FVector Pt = SplineComponent->GetLocationAtDistanceAlongSpline(i * Distance, ESplineCoordinateSpace::World);
				double Lon, Lat, Alt;
				LevelState->GetLLConverter().UE2LLA(Pt, Lon, Lat, Alt);
				OutFile << Lon << "," << Lat << "," << Alt << "\n";
			}
		}
	}
	catch (...)
	{
		UE_LOG(LogSoda, Error, TEXT("Can't write to \"%s\" "), *FileName);
		return false;
	}
	OutFile.close();

	UE_LOG(LogSoda, Warning, TEXT("Write spline to \"%s\" "), *FileName);

	return true;
}

void USodaStatics::GetAllSubclassOfClass(const UClass* Class, TArray<UClass*>& SubClasses)
{
	if (!Class) return;

	SubClasses.Empty();
	for (TObjectIterator<UClass> It; It; ++It)
	{
		if (It->IsChildOf(Class) && !It->HasAnyClassFlags(CLASS_Abstract))
			SubClasses.Add(*It);
	}
}

UObject* USodaStatics::GetDefaultObject(const UClass* Class)
{
	if(Class) return Class->GetDefaultObject();
	return nullptr;
}

bool USodaStatics::DelaunayTriangulation(
	const TArray<FVector>& OutsidePolyline3D,
	const TArray<FVector>& HolePolyline3D,
	const TArray<FVector>& AdditionalPoints3D,
	TArray<int32> & OutTriangles,
	TArray<FVector> & OutVertices,
	ETriangulationPlane Plane,
	bool bFlip)
{
	return false;
	// TODO: rewrite this algorithm to UE::Geometry::FConstrainedDelaunay2d
	/*
	OutTriangles.Empty();
	OutVertices.Empty();

	std::vector<p2t::Point> OutsidePolyline2D;
	std::vector<p2t::Point> HolePolyline2D;
	std::vector<p2t::Point> AdditionalPoints2D; 

	switch (Plane)
	{
		case ETriangulationPlane::XY:
			for (auto& Pt : OutsidePolyline3D) OutsidePolyline2D.push_back(p2t::Point(Pt.X, Pt.Y));
			for (auto& Pt : HolePolyline3D) HolePolyline2D.push_back(p2t::Point(Pt.X, Pt.Y));
			for (auto& Pt : AdditionalPoints3D) AdditionalPoints2D.push_back(p2t::Point(Pt.X, Pt.Y));
			break;

		case ETriangulationPlane::XZ:
			for (auto& Pt : OutsidePolyline3D) OutsidePolyline2D.push_back(p2t::Point(Pt.X, Pt.Z));
			for (auto& Pt : HolePolyline3D) HolePolyline2D.push_back(p2t::Point(Pt.X, Pt.Z));
			for (auto& Pt : AdditionalPoints3D) AdditionalPoints2D.push_back(p2t::Point(Pt.X, Pt.Z));
			break;

		case ETriangulationPlane::YZ:
			for (auto& Pt : OutsidePolyline3D) OutsidePolyline2D.push_back(p2t::Point(Pt.Y, Pt.Z));
			for (auto& Pt : HolePolyline3D) HolePolyline2D.push_back(p2t::Point(Pt.Y, Pt.Z));
			for (auto& Pt : AdditionalPoints3D) AdditionalPoints2D.push_back(p2t::Point(Pt.Y, Pt.Z));
			break;
	}

	std::vector<p2t::Point*> OutsidePolyline2DPtr;
	std::vector<p2t::Point*> HolePolyline2DPtr;

	std::map<p2t::Point*, const FVector*> Pt2DtoPt3D;

	for (int i =0; i< OutsidePolyline3D.Num(); ++i)
	{
		OutsidePolyline2DPtr.push_back(&OutsidePolyline2D[i]);
		Pt2DtoPt3D[&OutsidePolyline2D[i]] = &OutsidePolyline3D[i];
	}
	for (int i = 0; i < HolePolyline3D.Num(); ++i)
	{
		HolePolyline2DPtr.push_back(&HolePolyline2D[i]);
		Pt2DtoPt3D[&HolePolyline2D[i]] = &HolePolyline3D[i];
	}

	TSharedPtr<p2t::CDT> CDT;

	try
	{
		CDT = MakeShared<p2t::CDT>(OutsidePolyline2DPtr);
		if (HolePolyline3D.Num())CDT->AddHole(HolePolyline2DPtr);
		for (int i = 0; i < AdditionalPoints2D.size(); ++i)
		{

			CDT->AddPoint(&AdditionalPoints2D[i]);
			Pt2DtoPt3D[&AdditionalPoints2D[i]] = &AdditionalPoints3D[i];
		}
		CDT->Triangulate();
	}
	catch (...)
	{
		UE_LOG(LogSoda, Error, TEXT("Can't triangulate mesh "));
		return false;
	}

	std::vector<p2t::Triangle*> CDT_Triangulates = CDT->GetTriangles();
	std::map<p2t::Point*, int> PointMap;
	for (int i = 0; i < CDT->sweep_context_->point_count(); ++i)
	{
		PointMap[CDT->sweep_context_->GetPoint(i)] = i;
	}

	for (int i = 0; i < CDT->sweep_context_->point_count(); ++i)
	{
		OutVertices.Add(*Pt2DtoPt3D[CDT->sweep_context_->GetPoint(i)]);
	}

	if (bFlip)
	{
		for (auto& Triangle : CDT_Triangulates)
		{
			OutTriangles.Add(PointMap[Triangle->GetPoint(0)]);
			OutTriangles.Add(PointMap[Triangle->GetPoint(1)]);
			OutTriangles.Add(PointMap[Triangle->GetPoint(2)]);
		}
	}
	else
	{
		for (auto& Triangle : CDT_Triangulates)
		{
			OutTriangles.Add(PointMap[Triangle->GetPoint(2)]);
			OutTriangles.Add(PointMap[Triangle->GetPoint(1)]);
			OutTriangles.Add(PointMap[Triangle->GetPoint(0)]);
		}
	}

	return true;
	*/
}

FName USodaStatics::GetSensorShortName(FName FullName)
{
	FString Name = FullName.ToString();
	int32 Ind = Name.Find(TEXT("Component"), ESearchCase::CaseSensitive);
	if (Ind != INDEX_NONE)
	{
		Name.RemoveAt(Ind, 9);
	}

	Ind = Name.Find(TEXT("Sensor"), ESearchCase::CaseSensitive);
	if (Ind != INDEX_NONE)
	{
		Name.RemoveAt(Ind, 6);
	}

	
	if (Name.EndsWith(TEXT("_C"), ESearchCase::CaseSensitive))
	{
		Name.RemoveAt(Name.Len() - 2, 2);
	}

	return FName(*Name);
}

float USodaStatics::DrawProgress(
	UCanvas* Canvas,
	float XPos,
	float YPos,
	float Width,
	float Height,
	float Value,
	float Min,
	float Max,
	bool bAlingCenter,
	const FString& TextValue)
{

	FCanvasTileItem TileItem1(FVector2D(XPos, YPos), GWhiteTexture, FVector2D(Width, Height), FLinearColor(0.0f, 0.125f, 0.0f, 0.25f));
	TileItem1.BlendMode = SE_BLEND_Translucent;
	Canvas->DrawItem(TileItem1);

	if (bAlingCenter)
	{
		float NormValue = (FMath::Clamp(Value, Min, Max) - Min) / (Max - Min) * 2.0 - 1.0;
		FCanvasTileItem TileItem2(FVector2D(XPos + Width / 2, YPos), GWhiteTexture, FVector2D(Width / 2 * NormValue, Height), FLinearColor(0.8f, 0.8f, 0.0f, 0.5f));
		TileItem2.BlendMode = SE_BLEND_Translucent;
		Canvas->DrawItem(TileItem2);
	}
	else
	{
		float NormValue = (FMath::Clamp(Value, Min, Max) - Min) / (Max - Min);
		FCanvasTileItem TileItem2(FVector2D(XPos, YPos), GWhiteTexture, FVector2D(Width * NormValue, Height), FLinearColor(0.8f, 0.8f, 0.0f, 0.5f));
		TileItem2.BlendMode = SE_BLEND_Translucent;
		Canvas->DrawItem(TileItem2);
	}

	if (TextValue.Len())
	{
		UFont* RenderFont = GEngine->GetSmallFont();
		Canvas->SetDrawColor(FColor::White);
		float  TextWidth;
		float  TextHeight;
		Canvas->TextSize(RenderFont, TextValue, TextWidth, TextHeight, 1.0, 1.0);
		Canvas->DrawText(RenderFont, TextValue, XPos + (Width - TextWidth) / 2.0, YPos + (Height - TextHeight) / 2.0);
	}

	return Height;
}

void USodaStatics::SetFKeyByName(FString& Name, FKey& Key)
{
	TArray<FKey> AllKeys;
	EKeys::GetAllKeys(AllKeys);
	for (FKey& It : AllKeys)
	{
		if (It.GetFName().ToString() == Name)
		{
			Key = It;
			return;
		}
	}
}

float USodaStatics::FindSplineInputKeyClosestToWorldLocationFast(const FVector& WorldLocation, int32& StartSegment, USplineComponent* Spline)
{
	const FVector LocalLocation = Spline->GetComponentTransform().InverseTransformPosition(WorldLocation);
	const int32 NumPoints = Spline->SplineCurves.Position.Points.Num();
	const int32 NumSegments = Spline->SplineCurves.Position.bIsLooped ? NumPoints : NumPoints - 1;
	bool PerformFullSearch = false;
	if (StartSegment == -1 || StartSegment >= NumSegments - 1)
	{
		PerformFullSearch = true;
		StartSegment = 0;
	}

	if (NumPoints > 1)
	{
		float BestDistanceSq;
		float BestResult = Spline->SplineCurves.Position.InaccurateFindNearestOnSegment(LocalLocation, StartSegment, BestDistanceSq);
		int32 NewStartSegment = StartSegment;
		for (int32 Segment = StartSegment + 1; Segment < NumSegments; ++Segment)
		{
			float LocalDistanceSq;
			float LocalResult = Spline->SplineCurves.Position.InaccurateFindNearestOnSegment(LocalLocation, Segment, LocalDistanceSq);
			if (LocalDistanceSq < BestDistanceSq)
			{
				BestDistanceSq = LocalDistanceSq;
				BestResult = LocalResult;
				NewStartSegment = Segment;
			}
			else if (!PerformFullSearch)
			{
				break;
			}
		}
		for (int32 Segment = StartSegment - 1; Segment >= 0; --Segment)
		{
			float LocalDistanceSq;
			float LocalResult = Spline->SplineCurves.Position.InaccurateFindNearestOnSegment(LocalLocation, Segment, LocalDistanceSq);
			if (LocalDistanceSq < BestDistanceSq)
			{
				BestDistanceSq = LocalDistanceSq;
				BestResult = LocalResult;
				NewStartSegment = Segment;
			}
			else
			{
				break;
			}
		}
		StartSegment = NewStartSegment;
		return BestResult;
	}

	if (NumPoints == 1)
	{
		return Spline->SplineCurves.Position.Points[0].InVal;
	}

	return 0.0f;

}


float USodaStatics::GetDistanceAlongSpline(float InputKey, USplineComponent* InSplineComponent)
{
	float Frac, Int;
	Frac = FMath::Modf(InputKey, &Int);
	int Index = (int32)Int;

	int MaxExclusiveInputKey = (float)(InSplineComponent->IsClosedLoop() ? InSplineComponent->GetNumberOfSplinePoints() : InSplineComponent->GetNumberOfSplinePoints() - 1);
	check(Index <= MaxExclusiveInputKey);
	if (Index == MaxExclusiveInputKey)
	{
		check(Frac < 0.0001f);
		Index = MaxExclusiveInputKey - 1;
		Frac = 1.0f;
	}

	return InSplineComponent->GetDistanceAlongSplineAtSplinePoint(Index)
		+ InSplineComponent->SplineCurves.GetSegmentLength(Index, Frac, InSplineComponent->IsClosedLoop(), InSplineComponent->GetComponentTransform().GetScale3D());
}

// FPS
bool USodaStatics::GetShowFPS(UObject* /*WorldContextObject*/)
{
	if (!GEngine->GameViewport)
		return false;

	return GEngine->GameViewport->IsStatEnabled(TEXT("FPS"));
}

bool USodaStatics::SetShowFPS(UObject* WorldContextObject, bool ShowFps)
{
	if (UWorld* World = GetGameWorld(WorldContextObject))
	{
		if (!GEngine->GameViewport)
			return false;
		GEngine->SetEngineStat(World, GEngine->GameViewport, TEXT("FPS"), ShowFps);
		return true;
	}
	return false;
}

bool USodaStatics::DeleteFile(const FString& FileName)
{
	IPlatformFile& FileManager = FPlatformFileManager::Get().GetPlatformFile();
	return FileManager.DeleteFile(*FileName);
}


UObject * USodaStatics::GetSubobjectByName(const UObject * Parent, FName ObjectName)
{
	UObject* Object = nullptr;
	TArray<UObject*> SubObjects;

	ForEachObjectWithOuter(Parent, [&SubObjects](UObject* Object)
	{
		SubObjects.Add(Object);
	}, true);

	for (UObject* SubObject : SubObjects)
	{
		if (SubObject->GetFName() == ObjectName)
		{
			Object = SubObject;
			break;
		}
	}
	return Object;
}

TArray<UObject*> USodaStatics::FindAllObjectsByClass(UObject* WorldContextObject, const UClass* Class)
{
	TArray<UObject*> SubObjects;
	if (UWorld* World = GetGameWorld(WorldContextObject))
	{
		ForEachObjectWithOuter(World, [&SubObjects, Class](UObject* Object)
			{
				if (Object->GetClass()->IsChildOf(Class))
				{
					SubObjects.Add(Object);
				}
			}, true);
	}
	return SubObjects;
}

UActorComponent* USodaStatics::FindDefaultComponentByClass(
	const TSubclassOf<AActor> InActorClass,
	const TSubclassOf<UActorComponent> InComponentClass)
{
	if (!IsValid(InActorClass))
	{
		return nullptr;
	}

	// Check CDO.
	AActor* ActorCDO = InActorClass->GetDefaultObject<AActor>();
	UActorComponent* FoundComponent = ActorCDO->FindComponentByClass(InComponentClass);

	if (FoundComponent != nullptr)
	{
		return FoundComponent;
	}

	// Check blueprint nodes. Components added in blueprint editor only (and not in code) are not available from
	// CDO.
	UBlueprintGeneratedClass* RootBlueprintGeneratedClass = Cast<UBlueprintGeneratedClass>(InActorClass);
	UClass* ActorClass = InActorClass;

	// Go down the inheritance tree to find nodes that were added to parent blueprints of our blueprint graph.
	do
	{
		UBlueprintGeneratedClass* ActorBlueprintGeneratedClass = Cast<UBlueprintGeneratedClass>(ActorClass);
		if (!ActorBlueprintGeneratedClass)
		{
			return nullptr;
		}

		if (!ActorBlueprintGeneratedClass->SimpleConstructionScript)
		{
			return nullptr;
		}

		const TArray<USCS_Node*>& ActorBlueprintNodes =
			ActorBlueprintGeneratedClass->SimpleConstructionScript->GetAllNodes();

		for (USCS_Node* Node : ActorBlueprintNodes)
		{
			if (Node->ComponentClass->IsChildOf(InComponentClass))
			{
				return Node->GetActualComponentTemplate(RootBlueprintGeneratedClass);
			}
		}

		ActorClass = Cast<UClass>(ActorClass->GetSuperStruct());

	} while (ActorClass != AActor::StaticClass());

	return nullptr;
}

/*
UObject* USodaStatics::FindObjectsByName(FName Name)
{
	const TIndirectArray<FWorldContext>& WorldContexts = GEngine->GetWorldContexts();
	for (auto& WorldContext : WorldContexts)
	{
		switch (WorldContext.WorldType)
		{
		case EWorldType::PIE:
		case EWorldType::Game:
			UWorld* World = WorldContext.World();
			if (World && World->bBegunPlay)
			{
				return StaticFindObject(UObject::StaticClass(), World, *Name.ToString(), false);
			}
			break;
		}
	}
	return nullptr;
}
*/

AActor* USodaStatics::FindActoByName(UObject* WorldContextObject, FName Name)
{
	if (UWorld* World = GetGameWorld(WorldContextObject))
	{
		for (TActorIterator< AActor > it(World); it; ++it)
		{
			if ((*it)->GetFName() == Name)
			{
				return *it;
			}
		}
	}
	return nullptr;
}


bool USodaStatics::UploadVehicleJSON(const FString& JSONStr, const FString& VehicleName)
{

	const FString VehiclesDir = FPaths::ProjectSavedDir() / TEXT("SodaVehicles");
	const FString FileName = VehiclesDir / VehicleName + TEXT(".json");

	if (!FFileHelper::SaveStringToFile(JSONStr, *FileName))
	{
		UE_LOG(LogSoda, Error, TEXT("USodaStatics::UploadVehicleJSON(); Can't write to '%s' file"), *FileName);
		return false;
	}
	return true;
}

UProceduralMeshComponent * USodaStatics::GenerateRoadLine(
	UProceduralMeshComponent* LaneMesh,
	int SectionID,
	const USplineComponent* SplineComponent, 
	//float Offset,
	float Width,
	float LineLen,
	float LineGap
	//float Start,
	//float End
	)
{
	
	TArray < FVector > Vertices;
	TArray < int32 >Triangles;
	TArray<FVector2D> UVs;
	TArray<FVector> Normals;
	TArray<FColor> Colors;
	TArray<FProcMeshTangent> Tangents;

	const float MeshStep = 100;

	const float FullLength = SplineComponent->GetSplineLength();
	const int SegmCount = FullLength / (LineLen + LineGap) + 1.f;

	for (int s = 0; s < SegmCount; ++s)
	{
		const float CurrLen = s * (LineLen + LineGap);
		const float SegmLen = std::min(SplineComponent->GetSplineLength() - CurrLen, LineLen);
		const int SegmNum = SegmLen / MeshStep + 0.5;
		const float MeshStep2 = SegmLen / SegmNum;

		const int FirstVertices = Vertices.Num();

		for (int i = 0; i < SegmNum + 1; ++i)
		{
			FVector CenterLine = SplineComponent->GetLocationAtDistanceAlongSpline(CurrLen + i * MeshStep2, ESplineCoordinateSpace::Local);
			FVector RightVectors = SplineComponent->GetRightVectorAtDistanceAlongSpline(CurrLen + i * MeshStep2, ESplineCoordinateSpace::Local);
			FVector RightVectors2D = RightVectors.GetSafeNormal2D();

			Vertices.Add(CenterLine - RightVectors2D * Width / 2);
			Vertices.Add(CenterLine + RightVectors2D * Width / 2);
		}
		for (int i = 0; i < SegmNum; ++i)
		{
			Triangles.Add(FirstVertices + i * 2 + 2);
			Triangles.Add(FirstVertices + i * 2 + 1);
			Triangles.Add(FirstVertices + i * 2 + 0);
			Triangles.Add(FirstVertices + i * 2 + 2);
			Triangles.Add(FirstVertices + i * 2 + 3);
			Triangles.Add(FirstVertices + i * 2 + 1);
		}
	}

	//LaneMesh->RegisterComponent();
	LaneMesh->CreateMeshSection(SectionID, Vertices, Triangles, Normals, UVs, Colors, Tangents, false);
	//LaneMesh->SetMaterial(road_id, Profile->Material);
	LaneMesh->bCastCinematicShadow = false;
	LaneMesh->bCastDynamicShadow = false;
	LaneMesh->bCastStaticShadow = false;
	LaneMesh->bCastFarShadow = false;
	LaneMesh->SetCastShadow(false);
	LaneMesh->SetCastInsetShadow(false);
	//LaneMesh->SetCustomDepthStencilValue((uint8)Profile->Label);
	//LaneMesh->SetRenderCustomDepth(true);
	return LaneMesh;
	
}

void USodaStatics::SetSynchronousMode(bool bEnable, float DeltaSeconds)
{
	SodaApp.SetSynchronousMode(bEnable, DeltaSeconds); 
}

bool USodaStatics::IsSynchronousMode()
{ 
	return SodaApp.IsSynchronousMode();
}

int USodaStatics::GetFrameIndex()
{ 
	return SodaApp.GetFrameIndex();
}

bool USodaStatics::SynchTick(int64& Timestamp, int& FramIndex)
{ 
	return SodaApp.SynchTick(Timestamp, FramIndex);
}

FString USodaStatics::Int64ToString(int64 Value) 
{ 
	return std::to_string(Value).c_str(); 
}

FBox USodaStatics::CalculateActorExtent(const AActor * Actor)
{
	FBox Box(ForceInit);

	const FTransform& ActorToWorld = Actor->GetTransform();
	const FTransform WorldToActor = ActorToWorld.Inverse();

	for (UActorComponent* ActorComponent : Actor->GetComponents())
	{
		UPrimitiveComponent* PrimComp = Cast<UPrimitiveComponent>(ActorComponent);
		if (PrimComp)
		{
			// Only use collidable components to find collision bounding box.
			if (PrimComp->IsRegistered() && (/*bNonColliding ||*/ PrimComp->IsCollisionEnabled()))
			{
				if (USkeletalMeshComponent* SkeletalMeshComp = Cast<USkeletalMeshComponent>(PrimComp))
				{
					SkeletalMeshComp->InvalidateCachedBounds();
				}
				const FTransform ComponentToActor = PrimComp->GetComponentTransform() * WorldToActor;
				FBoxSphereBounds ActorSpaceComponentBounds = PrimComp->CalcBounds(ComponentToActor);

				Box += ActorSpaceComponentBounds.GetBox();
			}
		}
	}

	return Box;
}

/*
void USodaStatics::GetActorLocalBounds(AActor* Actor, FBox& LocalBounds)
{
	if (USkeletalMeshComponent* SkeletalMeshComp = Actor->FindComponentByClass<USkeletalMeshComponent>())
	{
		FTransform UnrotatedTransform = FTransform(Actor->GetTransform().GetRotation().Inverse(), FVector::ZeroVector, FVector::OneVector);
		FRotator SkeletalMeshRotator = SkeletalMeshComp->GetComponentRotation();
		SkeletalMeshComp->SetAllPhysicsRotation(FRotator());
		SkeletalMeshComp->InvalidateCachedBounds();
		LocalBounds = Actor->CalculateComponentsBoundingBoxInLocalSpace(false);
		SkeletalMeshComp->SetAllPhysicsRotation(SkeletalMeshRotator);
		SkeletalMeshComp->InvalidateCachedBounds();
	}
	else if (UStaticMeshComponent* StaticMeshComp = Actor->FindComponentByClass<UStaticMeshComponent>())
	{
		if (UStaticMesh* StaticMesh = StaticMeshComp->GetStaticMesh())
		{
			FVector LocDelta = StaticMeshComp->GetComponentLocation() - Actor->GetActorLocation();
			LocalBounds = StaticMesh->GetBoundingBox();
			LocalBounds = LocalBounds.ShiftBy(LocDelta);
		}
	}
}
*/

ESegmObjectLabel USodaStatics::GetTagOfTaggedComponent(UPrimitiveComponent* Component)
{
	if (!Component) return ESegmObjectLabel::None;
	return static_cast<ESegmObjectLabel>(Component->CustomDepthStencilValue);
}