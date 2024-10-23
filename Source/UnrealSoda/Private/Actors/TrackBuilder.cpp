// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/Actors/TrackBuilder.h"
#include "Soda/UnrealSoda.h"
#include "Soda/Misc/Utils.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "ConstrainedDelaunay2.h"
#include "KismetProceduralMeshLibrary.h"
#include "Soda/SodaStatics.h"
#include "Soda/SodaSubsystem.h"
#include "Soda/LevelState.h"
#include "Soda/SodaActorFactory.h"
#include "EngineUtils.h"
#include "Soda/Actors/SpawnPoint.h"
#include "Soda/Actors/LapCounter.h"
#include "Misc/Paths.h"
#include "Components/DecalComponent.h"
#include "Soda/LevelState.h"
#include "DesktopPlatformModule.h"
#include "Materials/MaterialInterface.h"
#include "Materials/Material.h"
#include "Styling/StyleColors.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Framework/Commands/UIAction.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/SWindow.h"
#include "SodaStyleSet.h"
#include "Widgets/Layout/SBorder.h"
#include "Soda/DBGateway.h"
#include "Soda/SodaApp.h"
#include "Soda/SodaSubsystem.h"
#include "Soda/UI/SMessageBox.h"
#include "RuntimeEditorUtils.h"
#include <map>

struct FPolyline;
struct FPolylineVertex;

struct FPolylineVertex
{
	FPolylineVertex() {}
	FPolylineVertex(const FVector& InPos) : Pos(InPos) {}
	FPolylineVertex(const FVector& InPos, FVector2D InUV) : Pos(InPos), UV(InUV) {}
	FVector Pos;
	FVector2D UV;

	FVector& operator() ()
	{
		return Pos;
	}
	FVector  operator() () const
	{
		return Pos;
	}
	FVector operator+(const FVector& Other) const
	{
		return Pos + Other;
	}
	FVector operator-(const FVector& Other) const
	{
		return Pos - Other;
	}

	FVector2D GetNormalizedUV() const
	{
		FVector2D Ret = UV;

		if (Ret.X < 0) Ret.X = Ret.X + int(Ret.X) + 1.0;
		if (Ret.Y < 0) Ret.Y = Ret.Y + int(Ret.Y) + 1.0;

		if (Ret.X > 1) Ret.X = Ret.X - int(Ret.X);
		if (Ret.Y > 1) Ret.Y = Ret.Y - int(Ret.Y);

		return Ret;
	}
};

struct FPolyline
{
	TArray<FPolylineVertex> Vertices;
	bool bClosed = true;

	FPolyline(bool bInClosed): bClosed(bInClosed) {}
	FPolyline(const TArray<FVector>& InPoints, bool bInClosed, float AdditionalElevation = 0)
	{
		Vertices.SetNumZeroed(InPoints.Num());
		for (int i = 0; i < InPoints.Num(); ++i)
		{
			Vertices[i] = InPoints[i];
			Vertices[i].Pos.Z += AdditionalElevation;
		}
		bClosed = bInClosed;
	}

	inline FPolylineVertex& operator[] (unsigned i) 
	{
		return Vertices[i];
	}

	inline int32 Num() const
	{
		return Vertices.Num();
	}

	FVector ComputeRightVectors2D(int Ind)
	{
		if (!bClosed)
		{
			if (Ind == Vertices.Num() - 1)
			{
				return ComputeNormal2D(Vertices[Ind - 1].Pos, Vertices[Ind].Pos, Vertices[Ind].Pos);
			}
			if (Ind == 0)
			{
				return ComputeNormal2D(Vertices[0].Pos, Vertices[0].Pos, Vertices[1].Pos);
			}
			return ComputeNormal2D(Vertices[Ind - 1].Pos, Vertices[Ind].Pos, Vertices[Ind + 1].Pos);
		}
		else
		{
			const int PrevInd = (Ind == 0) ? Vertices.Num() - 1 : Ind - 1;
			const int NextInd = (Ind + 1) % Vertices.Num();
			return ComputeNormal2D(Vertices[PrevInd].Pos, Vertices[Ind].Pos, Vertices[NextInd].Pos);
		}
	}

	void Expend(float Width)
	{
		for (int i = 0; i < Vertices.Num(); ++i)
		{
			const FVector& Pos = Vertices[i].Pos += ComputeRightVectors2D(i) * Width;
		}
	}

	void AddElevation(float Z)
	{
		for (auto& Pt : Vertices) Pt.Pos += FVector(0, 0, Z);
	}

	FVector Intersect(const FVector& PointA, const FVector& PointB)
	{
		const int Num = Vertices.Num() - 1 + int(bClosed);
		float NearestDist = (Vertices[0].Pos - PointA).Size();
		FVector NearestIntersection = Vertices[0].Pos;
		for (int i = 0; i < Num; ++i)
		{
			const FVector& Pt0 = Vertices[i].Pos;
			const FVector& Pt1 = Vertices[(i + 1) % Vertices.Num()].Pos;

			FVector Intersection;
			if (FMath::SegmentIntersection2D(Pt0, Pt1, PointA, PointB, Intersection))
			{
				float Dist = (PointA - Intersection).Size();
				if (Dist < NearestDist)
				{
					NearestDist = Dist;
					NearestIntersection = Intersection;
				}
			}
		}
		return NearestIntersection;
	}

	FPolyline Simplify(float MinSegmentLength) const
	{
		FPolyline Out(bClosed);
		float AccLength = TNumericLimits<float>::Max() / 2;
		FPolylineVertex PrePt = FVector::ZeroVector;
		for (auto& Pt : Vertices)
		{
			if (PrePt.Pos.IsZero()) PrePt = Pt;
			float Step = (PrePt.Pos - Pt.Pos).Size();
			AccLength += Step;
			Out.CashedLength += Step;
			if (MinSegmentLength < 0 || AccLength >= MinSegmentLength)
			{
				AccLength = 0;
				Out.Vertices.Add(Pt);
			}
			PrePt = Pt;
		}
		return Out;
	}

	float ClosestPoint(const FVector& Point, FPolylineVertex& Out) const
	{
		float FullDistance = 0;
		float ResDistance = 0;
		float NearestDist = (Vertices[0] - Point).Size();
		const int Num = Vertices.Num() - 1 + int(bClosed);
		FVector ClosestPoint = Vertices[0].Pos;
		int Min_i0 = 0;
		for (int i = 0; i < Num; ++i)
		{
			const FVector& Pt0 = Vertices[i].Pos;
			const FVector& Pt1 = Vertices[(i + 1) % Vertices.Num()].Pos;
			const FVector Pt3 = FMath::ClosestPointOnSegment(Point, Pt0, Pt1);
			float Dist = (Point - Pt3).Size();
			if (Dist < NearestDist)
			{
				ClosestPoint = Pt3;
				NearestDist = Dist;
				ResDistance = FullDistance + (Pt0 - Pt3).Size();
				Min_i0 = i;
			}
			FullDistance += (Pt0 - Pt1).Size();
		}

		float Alpha = (Vertices[Min_i0].Pos - ClosestPoint).Size() / (Vertices[Min_i0].Pos - Vertices[(Min_i0 + 1) % Vertices.Num()].Pos).Size();

		Alpha = FMath::Clamp(Alpha, 0.0f, 1.0f);

		float MaxV = Vertices.Last().UV.Y;

		FVector2D UV0 = Vertices[(Min_i0 == 0) ? Vertices.Num() - 1 : Min_i0 - 1].UV;
		FVector2D UV1 = Vertices[Min_i0].UV;
		FVector2D UV2 = Vertices[(Min_i0 + 1) % Vertices.Num()].UV;
		FVector2D UV3 = Vertices[(Min_i0 + 2) % Vertices.Num()].UV;

		if (Min_i0 == 0) UV0.Y -= MaxV;
		if ((Min_i0 + 1) >= Vertices.Num()) UV2.Y += MaxV;
		if ((Min_i0 + 2) >= Vertices.Num()) UV3.Y += MaxV;

		Out.UV = CubicInterpolate(UV0, UV1, UV2, UV3, Alpha);
		Out.Pos = ClosestPoint;
		return ResDistance;
	}

	void Relax(float Speed = 0.5, int Iterations = 10, float TargetAngel = 100)
	{
		TargetAngel = TargetAngel / 180 * M_PI;
		const int Num = bClosed ? Vertices.Num() : Vertices.Num() - 2;
		TArray<FVector> Add;
		Add.SetNumZeroed(Vertices.Num());
		for (int j = 0; j < Iterations; ++j)
		{
			for (int i = 0; i < Num; ++i)
			{
				const FVector& Pt0 = Vertices[i].Pos;
				const FVector& Pt1 = Vertices[(i + 1) % Vertices.Num()].Pos;
				const FVector& Pt2 = Vertices[(i + 2) % Vertices.Num()].Pos;
				const FVector V1 = Pt0 - Pt1;
				const FVector V2 = Pt2 - Pt1;
				const FVector V = V1 + V2;
				const FVector Norm = V.GetSafeNormal();
				const float Angle = FMath::Acos(V1.CosineAngle2D(V2));
				const float MaxDistance = V.Size() / 2.0;
				const float lk = 1.0f - std::min(Angle / TargetAngel , 1.0f);
				Add[(i + 1) % Vertices.Num()] = Norm * MaxDistance * Speed * lk;
			}
			for (int i = 0; i < Num; ++i)
			{
				int Ind = (i + 1) % Vertices.Num();
				Vertices[Ind].Pos += Add[Ind];
			}
		}
	}

	FVector GetPointByLength(float Length)
	{
		float AccLength = 0;

		for (int i = 0; i < Vertices.Num() - 1; ++i)
		{
			float DLen = (Vertices[i].Pos - Vertices[i + 1].Pos).Size();

			if ((AccLength + DLen) >= Length)
			{
				return Vertices[i].Pos + (Vertices[i + 1].Pos - Vertices[i].Pos).GetSafeNormal() * (AccLength + DLen - Length);
			}
			AccLength += DLen;
		}

		return Vertices.Last().Pos;
	}

	int FindNearestVertex(const FVector& Point)
	{
		int Ret = 0;
		float MinLength = (Vertices[0].Pos - Point).Size();
		for (int i = 0; i < Vertices.Num(); ++i)
		{
			int Ind = i % Vertices.Num();
			float Length = (Vertices[Ind].Pos - Point).Size();
			if (Length < MinLength)
			{
				MinLength = Length;
				Ret = Ind;
			}
		}
		return Ret;
	}

	void Normalize()
	{
		float Max = 0;
		int iMax = -1;
		for (int i = 0; i < Vertices.Num(); ++i)
		{
			if (Vertices[i].UV.Y > Max)
			{
				Max = Vertices[i].UV.Y;
				iMax = i;
			}
		}
		if (iMax >= 0 && iMax != Vertices.Num() - 1)
		{

			for (int i = 0; i <= iMax; ++i)
			{
				Vertices[i].UV.Y -= int(Max) + 1;
			}
		}
	}

	float GetCashedLength() const
	{
		return CashedLength;
	}

protected:
	float CashedLength = 0;
};

static float SimplifyPlolylane(const TArray<FVector>& In, float MinSegmentLength, TArray<FVector>& Out, bool bClose = false)
{
	float Length = 0;

	float AccLength = TNumericLimits<float>::Max() / 2;
	FVector PrePt = FVector::ZeroVector;

	for (auto& Pt : In)
	{
		if (PrePt.IsZero()) PrePt = Pt;
		float Step = (PrePt - Pt).Size();
		AccLength += Step;
		Length += Step;
		if (MinSegmentLength < 0 || AccLength >= MinSegmentLength)
		{
			AccLength = 0;
			Out.Add(Pt);
		}
		PrePt = Pt;
	}

	if (bClose)
	{
		if ((In[0] - Out.Last()).Size() < MinSegmentLength * 0.5)
		{
			Out.Last() = In[0];
		}
		else
		{
			Out.Add(In[0]);
		}
	}

	return Length;
}


ATrackBuilder::ATrackBuilder()
{
	RouteOffset = FVector(0, 0, 100);
	RootComponent = CreateDefaultSubobject< USceneComponent >(TEXT("Root"));

	static ConstructorHelpers::FObjectFinder< UMaterial > DefMat(TEXT("/SodaSim/Assets/SimpleMat/VertexColorMat"));
	if (DefMat.Succeeded())
	{
		TrackMaterial = DefMat.Object;
		MarkingsMaterial = DefMat.Object;
		DecalMaterial = DefMat.Object;
	}

	PrimaryActorTick.bCanEverTick = true;

}

bool ATrackBuilder::JSONReadLine(const TSharedPtr<FJsonObject>& JSON, const FString& FieldName, TArray<FVector>& OutPoints, bool ImportAltitude)
{
	const TArray< TSharedPtr< FJsonValue > >* JsonNode;

	if (!JSON->TryGetArrayField(FieldName, JsonNode))
	{
		UE_LOG(LogSoda, Warning, TEXT("JSONReadLine(%s); Can't find node in the"), *FieldName);
		return false;
	}

	float AccLength = TNumericLimits<float>::Max() / 2;
	FVector PrePt = FVector::ZeroVector;

	for (auto& JsonValue : *JsonNode)
	{
		const TArray< TSharedPtr< FJsonValue > >* Points;
		if (!JsonValue->TryGetArray(Points))
		{
			UE_LOG(LogSoda, Error, TEXT("JSONReadLine(%s); Wrong point format (the point isn't array)"), *FieldName);
			return false;
		}
		if (Points->Num() < 2)
		{
			UE_LOG(LogSoda, Error, TEXT("JSONReadLine(%s); Wrong point format (the point size < 2)"), *FieldName);
			return false;
		}

		FVector Pt = FVector::ZeroVector;

		if (!(*Points)[0]->TryGetNumber(Pt.X) || !(*Points)[1]->TryGetNumber(Pt.Y))
		{
			UE_LOG(LogSoda, Error, TEXT("JSONReadLine(%s); Wrong point format (x or y isn't number)"), *FieldName);
			return false;
		}

		if (Points->Num() >= 3 && ImportAltitude)
		{
			if (!(*Points)[2]->TryGetNumber(Pt.Z))
			{
				UE_LOG(LogSoda, Error, TEXT("JSONReadLine(%s); Wrong point format (z isn't number)"), *FieldName);
				return false;
			}
		}

		Pt *= 100;
		Pt.X = -Pt.X;

		OutPoints.Add(Pt);
	}

	return true;
}

void ATrackBuilder::BeginPlay()
{
	Super::BeginPlay();

	/*
	UVariantParam* JSONFilesParma = VariantParams->Add((FString*)(nullptr), "JSON Files", false);
	JSONFilesParma->CategoryName = "LoadJsonTrack";
	

	TArray<FString> Files;
	IFileManager::Get().FindFiles(Files, *(FPaths::ProjectDir() / "JSON_Traks"));

	for (auto& File : Files)
	{
		JSONFilesParma->MapAsString.Add(File, FPaths::ProjectDir() / "JSON_Traks" / File);
	}
	*/

	if (bJSONLoaded)
	{
		if (bGenerateTrackOnBeginPlay)
		{
			GenerateTrack();
		}

		if (bGeneratMarkingsOnBeginPlay)
		{
			GenerateMarkings();
		}

		if (bGenerateLineRoutesOnBeginPlay)
		{
			GenerateLineRoutes();
		}

		if (bGenerateRaceRouteOnBeginPlay)
		{
			GenerateRaceRoute();
		}

		if (bGeneratStartLineOnBeginPlay)
		{
			GenerateStartLine();
		}
	}
}

void ATrackBuilder::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	ClearTrack();
	ClearRoutes();
	ClearMarkings();
	ClearLapCounter();
	ClearStartLine();
}

void ATrackBuilder::TickActor(float DeltaTime, enum ELevelTick TickType, FActorTickFunction& ThisTickFunction)
{
	Super::TickActor(DeltaTime, TickType, ThisTickFunction);

	if (bDrawDebug)
	{
		for (int i = 0; i < OutsidePoints.Num() -1; ++i)
		{
			DrawDebugLine(GetWorld(), OutsidePoints[i], OutsidePoints[i + 1], FColor(0, 0, 255), false, -1,0, 2);
		}

		for (int i = 0; i < InsidePoints.Num() - 1; ++i)
		{
			DrawDebugLine(GetWorld(), InsidePoints[i], InsidePoints[i + 1], FColor(0, 0, 255), false, -1, 0, 2);
		}

		for (int i = 0; i < CentrePoints.Num() - 1; ++i)
		{
			DrawDebugLine(GetWorld(), CentrePoints[i], CentrePoints[i + 1], FColor(0, 255, 0), false, -1, 0, 2);
		}

		for (int i = 0; i < RacingPoints.Num() - 1; ++i)
		{
			DrawDebugLine(GetWorld(), RacingPoints[i], RacingPoints[i + 1], FColor(255, 0, 0), false, -1, 0, 2);
		}
	}
}

void ATrackBuilder::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	//GenerateMesh();
}

bool ATrackBuilder::LoadJsonFromFile(const FString & InFileName)
{
	UnloadJson();

	FString JsonString;
	if (!FFileHelper::LoadFileToString(JsonString, *InFileName))
	{
		UE_LOG(LogSoda, Error, TEXT("ATrackBuilder::LoadJsonFromFile(); Can't read file"));
		return false;
	}

	TSharedPtr<FJsonObject> JSON;
	TSharedRef<TJsonReader<TCHAR>> Reader = TJsonReaderFactory<TCHAR>::Create(JsonString);
	if (!FJsonSerializer::Deserialize(Reader, JSON))
	{
		UE_LOG(LogSoda, Error, TEXT("ATrackBuilder::LoadJsonFromFile(); Can't JSON deserialize"));
		return false;
	}

	if (!JSONReadLine(JSON, TEXT("Inside"), InsidePoints, bImportAltitude) ||
		!JSONReadLine(JSON, TEXT("Outside"), OutsidePoints, bImportAltitude) ||
		!JSONReadLine(JSON, TEXT("Centre"), CentrePoints, bImportAltitude))
	{
		UE_LOG(LogSoda, Error, TEXT("ATrackBuilder::LoadJsonFromFile(); Can't parse Inside or Outside or Centre node"));
		return false;
	}

	JSONReadLine(JSON, TEXT("Racing"), RacingPoints, false);

	if (CentrePoints.Num() < 3 && OutsidePoints.Num() < 3 && CentrePoints.Num() < 3)
	{
		UE_LOG(LogSoda, Error, TEXT("ATrackBuilder::LoadJsonFromFile(); Size of track line < 3"));
		return false;
	}

	//Align the lines
	/*
	int Ind;
	Ind = FindNearestPoint2D(CentrePoints[0], InsidePoints);
	InsidePoints.Append(InsidePoints.GetData(), Ind);
	InsidePoints.RemoveAt(0, Ind);
	Ind = FindNearestPoint2D(CentrePoints[0], OutsidePoints);
	OutsidePoints.Append(OutsidePoints.GetData(), Ind);
	OutsidePoints.RemoveAt(0, Ind);
	*/

	if (bIsClosedTrack)
	{
		bool InsidePointsDir = IsClockwise2D(InsidePoints);
		bool OutsidePointsDir = IsClockwise2D(OutsidePoints);
		bool CentrePointsDir = IsClockwise2D(CentrePoints);

		if (CentrePointsDir != InsidePointsDir || CentrePointsDir != OutsidePointsDir)
		{
			UE_LOG(LogSoda, Error, TEXT("ATrackBuilder::LoadJsonFromFile(); Direction of Inside, Outside and Centre line are not matched"));
			//return false;
		}

		bIsClockwise2D = CentrePointsDir;
	} 
	else
	{
		FVector ForwardVec =  ComputeNormal2D(InsidePoints[0], CentrePoints[0], OutsidePoints[0]);
		FVector RightVec = InsidePoints[0] - CentrePoints[0];
		bIsClockwise2D = (ForwardVec.X * RightVec.Y - RightVec.X * ForwardVec.Y) > 0;
	}

	const TArray< TSharedPtr< FJsonValue > >* ReferencePoint;
	if (JSON->TryGetArrayField(TEXT("ReferencePoint"), ReferencePoint) && ReferencePoint && ReferencePoint->Num() == 3 && 
		(*ReferencePoint)[0]->TryGetNumber(RefPointLon) && 
		(*ReferencePoint)[1]->TryGetNumber(RefPointLat) &&
		(*ReferencePoint)[2]->TryGetNumber(RefPointAlt))
	{
		bRefPointIsOk = true;
	}
	else
	{
		UE_LOG(LogSoda, Warning, TEXT("ATrackBuilder::LoadJsonFromFile(); Can't parse 'ReferencePoint' node"));
	}

	bJSONLoaded = true;
	LoadedFileName = InFileName;

	UE_LOG(LogSoda, Log, TEXT(" ATrackBuilder::LoadJsonFromFile(); %s sucessful loaded"), *LoadedFileName);

	return true;
}

void ATrackBuilder::UnloadJson()
{
	OutsidePoints.Empty();
	InsidePoints.Empty();
	CentrePoints.Empty();
	RacingPoints.Empty();

	bRefPointIsOk = false;
	bJSONLoaded = false;
	LoadedFileName = "";

	RefPointLon = 0;
	RefPointLat = 0;
	RefPointAlt = 0;
}

void ATrackBuilder::GenerateAll()
{
	GenerateTrack();
	GenerateMarkings();
	GenerateLineRoutes();
	GenerateStartLine();
	GenerateLapCounter();
	UpdateGlobalRefpoint();
}

void ATrackBuilder::LoadJson()
{
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	if (!DesktopPlatform)
	{
		UE_LOG(LogSoda, Error, TEXT("ATrackBuilder::LoadJson() Can't get the IDesktopPlatform ref"));
		return;
	}

	const FString FileTypes = TEXT("(*.json)|*.json");

	TArray<FString> OpenFilenames;
	int32 FilterIndex = -1;
	if (!DesktopPlatform->OpenFileDialog(nullptr, TEXT("Import track from JSON"), TEXT(""), TEXT(""), FileTypes, EFileDialogFlags::None, OpenFilenames, FilterIndex) || OpenFilenames.Num() <= 0)
	{
		UE_LOG(LogSoda, Error, TEXT("ATrackBuilder::LoadJson() Can't open file"));
		return;
	}

	LoadJsonFromFile(OpenFilenames[0]);
}


void ATrackBuilder::ClearTrack()
{
	for (auto& it : TrackMeshes)
	{
		if (IsValid(it))
		{
			it->ClearAllMeshSections();
			it->DestroyComponent();
		}
	}
	TrackMeshes.Empty();
}

void ATrackBuilder::GenerateTrack()
{
	if (!bJSONLoaded)
	{
		UE_LOG(LogSoda, Error, TEXT("ATrackBuilder::GenerateMarkingsInner(); JSON isn't loaded"));
		return;
	}

	ClearTrack();

	if (InsidePoints.Num() < 3 || OutsidePoints.Num() < 3 || CentrePoints.Num() < 3)
	{
		UE_LOG(LogSoda, Error, TEXT("ATrackBuilder::GenerateMesh(); Input track is broken"));
		return;
	}

	const float Dir = bIsClockwise2D ? -1 : 1;

	/*
	 * Initialization and simplify  of inside, otside and centre track's polylines 
	 */
	FPolyline InsidePolyline(InsidePoints, bIsClosedTrack, TrackElevation);
	FPolyline OutsidePolyline(OutsidePoints, bIsClosedTrack, TrackElevation);
	FPolyline CentrePolyline(CentrePoints, bIsClosedTrack, TrackElevation);

	if (bRelax)
	{
		InsidePolyline.Relax(RelaxSpeed, RelaxIterations, RelaxTargetAngel);
		OutsidePolyline.Relax(RelaxSpeed, RelaxIterations, RelaxTargetAngel);
		CentrePolyline.Relax(RelaxSpeed, RelaxIterations, RelaxTargetAngel);
	}

	InsidePolyline = InsidePolyline.Simplify(TrackMinSegmentLength);
	OutsidePolyline = OutsidePolyline.Simplify(TrackMinSegmentLength);
	CentrePolyline = CentrePolyline.Simplify(TrackMinSegmentLength);

	if (InsidePolyline.GetCashedLength() <= 0 || OutsidePolyline.GetCashedLength() <= 0 || InsidePolyline.Vertices.Num() < 3 || OutsidePolyline.Vertices.Num() < 0)
	{
		UE_LOG(LogSoda, Error, TEXT("ATrackBuilder::GenerateMesh(); Simplified track is broken"));
		return;
	}


	/*
	 * Comput UV for every polyline. UVs calculat relative to the center line. 
	 * InsidePolylineHelper and OutsidePolylineHelper are the center line transferred to the inside and outside polylines.
	 */
	FPolyline InsidePolylineHelper(bIsClosedTrack);
	FPolyline OutsidePolylineHelper(bIsClosedTrack);

	float VTexLength = 0;
	for (int i = 0; i < CentrePolyline.Num(); ++i)
	{
		FVector RightVectors2D = CentrePolyline.ComputeRightVectors2D(i) * Dir;
		InsidePolylineHelper.Vertices.Add(FPolylineVertex(InsidePolyline.Intersect(CentrePolyline[i].Pos, CentrePolyline[i].Pos + RightVectors2D * 10000), FVector2D(0.0, VTexLength) ));
		OutsidePolylineHelper.Vertices.Add(FPolylineVertex(OutsidePolyline.Intersect(CentrePolyline[i].Pos, CentrePolyline[i].Pos - RightVectors2D * 10000), FVector2D(1.0, VTexLength)));
		CentrePolyline[i].UV = FVector2D(0.5, VTexLength);
		VTexLength += (CentrePolyline[i].Pos - CentrePolyline[(i + 1) % CentrePolyline.Num()].Pos).Size() * TrackTexScaleV;
	}

	/*
	 * CalibCoeffTexV is scaling factor for the TexV coordinate to scale TexV to a multiple of 1.
	 */
	const float CalibCoeffTexV = float(int(VTexLength)) / VTexLength;
	for (int i = 0; i < CentrePolyline.Num(); ++i)
	{
		CentrePolyline[i].UV.Y *= CalibCoeffTexV;
		InsidePolylineHelper[i].UV.Y *= CalibCoeffTexV;
		OutsidePolylineHelper[i].UV.Y *= CalibCoeffTexV;
	}

	/*
	 * Determine the UV for inside and outside polyline how nearest destance to InsidePolylineHelper and OutsidePolylineHelper.
	 */
	for (int i = 0; i < InsidePolyline.Num(); ++i)
	{
		FPolylineVertex Pt;
		InsidePolylineHelper.ClosestPoint(InsidePolyline[i].Pos, Pt);
		InsidePolyline[i].UV =  Pt.UV;
	}
	for (int i = 0; i < OutsidePolyline.Num(); ++i)
	{
		FPolylineVertex Pt;
		OutsidePolylineHelper.ClosestPoint(OutsidePolyline[i].Pos, Pt);
		OutsidePolyline[i].UV = Pt.UV;
	}

	int CenterLineInd = 0;
	int InsideLineInd = 0;
	int OutsideLineInd = 0;
	int MeshID = 0;
	bool IsLastSegment = false;

	/*
	 * Split the track into segments, loop until we reach the last segment
	 */
	while (!IsLastSegment && !(TrackBuildSegmentCount > 0 && (TrackBuildSegmentCount - 1) < MeshID))
	{
		/*
		 * Find the segment of track
		 */
		FPolyline InsideSegment(false);
		FPolyline OutsideSegment(false);
		FPolyline CentreSegment(false);

		const int PointsLeft = CentrePolyline.Num() - CenterLineInd;
		IsLastSegment = PointsLeft < (TrackSegmentStep + TrackSegmentStep / 2);

		int CenterSegmEndInd;
		int InsideSegmEndInd;
		int OutsideSegmEndInd;

		if (IsLastSegment)
		{
			CenterSegmEndInd = CentrePolyline.Num() - 1;
			InsideSegmEndInd = InsidePolyline.Num() - 1;
			OutsideSegmEndInd = OutsidePolyline.Num() - 1;
		}
		else
		{
			CenterSegmEndInd = CenterLineInd + TrackSegmentStep - 1;
			const FVector& CenterPoint = CentrePolyline[CenterSegmEndInd].Pos;
			InsideSegmEndInd = InsidePolyline.FindNearestVertex(CenterPoint);
			OutsideSegmEndInd = OutsidePolyline.FindNearestVertex(CenterPoint);
		}

		for (int i = CenterLineInd; i <= CenterSegmEndInd; ++i) CentreSegment.Vertices.Add(CentrePolyline[i]);
		for (int i = InsideLineInd; i <= InsideSegmEndInd; ++i) InsideSegment.Vertices.Add(InsidePolyline[i]);
		for (int i = OutsideLineInd; i <= OutsideSegmEndInd; ++i) OutsideSegment.Vertices.Add(OutsidePolyline[i]);

		if (IsLastSegment && bIsClosedTrack)
		{
			if ((CentrePolyline.Vertices[0].Pos - CentrePolyline.Vertices.Last().Pos).Size() > TrackMinSegmentLength)
			{
				CentreSegment.Vertices.Add(FPolylineVertex(CentrePolyline.Vertices[0].Pos, FVector2D(0.5, VTexLength * CalibCoeffTexV)));
			}
			else
			{
				CentreSegment.Vertices.Last() = FPolylineVertex(CentrePolyline.Vertices[0].Pos, FVector2D(0.5, VTexLength * CalibCoeffTexV));
			}

			if ((InsidePolyline.Vertices[0].Pos - InsidePolyline.Vertices.Last().Pos).Size() > TrackMinSegmentLength)
			{
				InsideSegment.Vertices.Add(FPolylineVertex(InsidePolyline.Vertices[0].Pos, FVector2D(0.0, VTexLength * CalibCoeffTexV)));
			}
			else
			{
				InsideSegment.Vertices.Last() = FPolylineVertex(InsidePolyline.Vertices[0].Pos, FVector2D(0.0, VTexLength * CalibCoeffTexV));
			}

			if ((OutsidePolyline.Vertices[0].Pos - OutsidePolyline.Vertices.Last().Pos).Size() > TrackMinSegmentLength)
			{
				OutsideSegment.Vertices.Add(FPolylineVertex(OutsidePolyline.Vertices[0].Pos, FVector2D(1.0, VTexLength * CalibCoeffTexV)));
			}
			else
			{
				OutsideSegment.Vertices.Last() = FPolylineVertex(OutsidePolyline.Vertices[0].Pos, FVector2D(1.0, VTexLength * CalibCoeffTexV));
			}

		}
		
		/*
		 * Normalize segment texture coordinates
		 */
		FVector2D FirstUV = CentreSegment[0].GetNormalizedUV() - CentreSegment[0].UV;
		for (int i = 0; i < CentreSegment.Num(); ++i) CentreSegment[i].UV.Y += FirstUV.Y;
		for (int i = 0; i < InsideSegment.Num(); ++i) InsideSegment[i].UV.Y += FirstUV.Y;
		for (int i = 0; i < OutsideSegment.Num(); ++i) OutsideSegment[i].UV.Y += FirstUV.Y;

		InsideSegment.Normalize();
		OutsideSegment.Normalize();

		/*
		 * Compute border points and inner points for segment with borders and center line
		 */
		FPolyline VerticesBorder(true), VerticesInner(false);
		if (bTrackWithCenterLine) VerticesBorder.Vertices.Add(FPolylineVertex(CentreSegment[0]));
		if (bWithBorder)
		{
			for (int i = 0; i < InsideSegment.Num(); ++i)
			{
				FVector RightVectors2D = (IsLastSegment && (i == (InsideSegment.Num() - 1))) ?
					InsidePolyline.ComputeRightVectors2D(0) :
					InsidePolyline.ComputeRightVectors2D(InsideLineInd + i);

				const FVector2D & UV = InsideSegment[i].UV;
				const FVector & Pos = InsideSegment[i].Pos;
				FPolylineVertex PtIn = FPolylineVertex(Pos, FVector2D(TrackTexBorderPartU, UV.Y));
				FPolylineVertex PtOut = FPolylineVertex(Pos + RightVectors2D * BorderWidth * Dir, UV);
				PtOut.Pos.Z = 0;
				if (i == 0)
				{
					VerticesBorder.Vertices.Add(PtIn);
					VerticesBorder.Vertices.Add(PtOut);
				}
				else if (i == InsideSegment.Num() - 1)
				{
					VerticesBorder.Vertices.Add(PtOut);
					VerticesBorder.Vertices.Add(PtIn);
				}
				else
				{
					VerticesBorder.Vertices.Add(PtOut);
					VerticesInner.Vertices.Add(PtIn);
				}
			}
		}
		else
		{
			for (int i = 0; i < InsideSegment.Num(); ++i) VerticesBorder.Vertices.Add(InsideSegment[i]);
		}
		if (bTrackWithCenterLine)
		{
			VerticesBorder.Vertices.Add(FPolylineVertex(CentreSegment[CentreSegment.Num() - 1]));
		}
		if (bWithBorder)
		{
			for (int i = OutsideSegment.Num() - 1; i >= 0; --i)
			{
				FVector RightVectors2D = (IsLastSegment && (i == (OutsideSegment.Num() - 1))) ?
					OutsidePolyline.ComputeRightVectors2D(0) :
					OutsidePolyline.ComputeRightVectors2D(OutsideLineInd + i);

				const FVector2D& UV = OutsideSegment[i].UV;
				const FVector& Pos = OutsideSegment[i].Pos;
				FPolylineVertex PtIn = FPolylineVertex(Pos, FVector2D(1.0 - TrackTexBorderPartU, UV.Y));
				FPolylineVertex PtOut = FPolylineVertex(Pos - RightVectors2D * BorderWidth * Dir, UV);
				PtOut.Pos.Z = 0;
				if (i == 0)
				{
					VerticesBorder.Vertices.Add(PtOut);
					VerticesBorder.Vertices.Add(PtIn);
				}
				else if (i == OutsideSegment.Num() - 1)
				{
					VerticesBorder.Vertices.Add(PtIn);
					VerticesBorder.Vertices.Add(PtOut);
				}
				else
				{
					VerticesBorder.Vertices.Add(PtOut);
					VerticesInner.Vertices.Add(PtIn);
				}
			}
		}
		else
		{
			for (int i = OutsideSegment.Num() - 1; i >= 0; --i) VerticesBorder.Vertices.Add(OutsideSegment[i]);
		}
		if (bTrackWithCenterLine)
		{
			for (int i = 1; i < CentreSegment.Num() - 1; ++i) VerticesInner.Vertices.Add(CentreSegment[i]);
		}

		CenterLineInd = CenterSegmEndInd;
		InsideLineInd = InsideSegmEndInd;
		OutsideLineInd = OutsideSegmEndInd;

		/*
		 * Triangulate
		 */
		UE::Geometry::FConstrainedDelaunay2d Triangulation;
		Triangulation.Vertices.Add({ VerticesBorder.Vertices[0].Pos.X, VerticesBorder.Vertices[0].Pos.Y });
		for (int i = 1; i< VerticesBorder.Vertices.Num(); ++i)
		{
			auto& Point = VerticesBorder.Vertices[i];
			Triangulation.Vertices.Add({ Point.Pos.X, Point.Pos.Y });
			Triangulation.Edges.Add(UE::Geometry::FIndex2i(i-1, i));
		}
		Triangulation.Edges.Add(UE::Geometry::FIndex2i(VerticesBorder.Vertices.Num() - 1, 0));

		for (auto& Point : VerticesInner.Vertices)
		{
			Triangulation.Vertices.Add({ Point.Pos.X, Point.Pos.Y });
		}

		bool bTriangulationSuccess = Triangulation.Triangulate();
		if (!bTriangulationSuccess || Triangulation.Triangles.Num() == 0)
		{
			UE_LOG(LogSoda, Error, TEXT("ATrackBuilder::GenerateMesh(); Can't triangulateh section %i"), MeshID);
			++MeshID;
			continue;
		}

		/*
		 * Build mesh
		 */
		TArray <FVector> Vertices;
		TArray<FVector2D> UVs;

		for (auto& Point : VerticesBorder.Vertices)
		{
			Vertices.Add(Point.Pos);
			UVs.Add(Point.UV);
		}
		for (auto& Point : VerticesInner.Vertices)
		{
			Vertices.Add(Point.Pos);
			UVs.Add(Point.UV);
		}

		TArray <int32> Triangles;
		for (const UE::Geometry::FIndex3i& Tri : Triangulation.Triangles)
		{
			Triangles.Add(Tri.A);
			Triangles.Add(Tri.B);
			Triangles.Add(Tri.C);
		}
		TArray<FColor> Colors;
		Colors.SetNum(Vertices.Num());
		for (auto& It : Colors) It = FColor(50, 50, 50);
		TArray<FVector> Normals;
		TArray<FProcMeshTangent> Tangents;
		UKismetProceduralMeshLibrary::CalculateTangentsForMesh(Vertices, Triangles, UVs, Normals, Tangents);


		UProceduralMeshComponent* ProceduralMesh = NewObject<UProceduralMeshComponent>(this);
		ProceduralMesh->SetupAttachment(RootComponent);
		ProceduralMesh->RegisterComponent();
		ProceduralMesh->CreateMeshSection(MeshID, Vertices, Triangles, Normals, UVs, Colors, Tangents, true);
		ProceduralMesh->bUseComplexAsSimpleCollision = true;
		ProceduralMesh->ComponentTags.Add("CusomSegmTag");
		ProceduralMesh->bRenderCustomDepth = true;
		ProceduralMesh->SetCustomDepthStencilValue(int32(ESegmObjectLabel::Roads));
		ProceduralMesh->SetRenderCustomDepth(true);
		if (IsValid(TrackMaterial))
		{
			ProceduralMesh->SetMaterial(MeshID, TrackMaterial);
		}

		TrackMeshes.Add(ProceduralMesh);

		UE_LOG(LogSoda, Log, TEXT(" ATrackBuilder::GenerateMesh(); ID: %i; Vertices: %i, Triangles: %i"), MeshID, Vertices.Num(), Triangles.Num() / 3);

		++MeshID;
	}

}

void ATrackBuilder::GenerateLineRoutes()
{
	ClearRoutes();
	
	if (CentrePoints.Num() < 3 )
	{
		UE_LOG(LogSoda, Error, TEXT("ATrackBuilder::GenerateLineRoutes(); Centre line is broken"));
		return;
	}

	float Dir = IsClockwise2D(CentrePoints) ? -1 : 1;

	FVector Location = GetActorLocation();
	TArray <TArray<FVector>> RoutesPoints;
	RoutesPoints.SetNum(NumRouteLanes);
	for (size_t i = 0; i < CentrePoints.Num(); ++i)
	{
		FVector ForwardVector = (CentrePoints[(i + 1) % CentrePoints.Num()] - CentrePoints[i]).GetSafeNormal();
		FVector RightVectors2D = FRotationMatrix(ForwardVector.GetSafeNormal2D().Rotation()).GetScaledAxis(EAxis::Y) * Dir;

		FVector PtOut = OutsidePoints[FindNearestPoint2D(CentrePoints[i], OutsidePoints)];
		FVector PtIn = InsidePoints[FindNearestPoint2D(CentrePoints[i], InsidePoints)];

		float TrackWidth = FVector2D(PtOut - PtIn).Size();

		for (int j = 0; j < NumRouteLanes; ++j)
		{
			float CalibCoeff = float(j) / (NumRouteLanes) + 1.0f / NumRouteLanes * 0.5;
			float DZ = (PtIn.Z - PtOut.Z) * CalibCoeff  + PtOut.Z;
			FVector Pt = CentrePoints[i] + RouteOffset + Location + FVector(0, 0, DZ) + RightVectors2D * (CalibCoeff - 0.5) * TrackWidth;
			RoutesPoints[j].Add(Pt);
		}
	}

	for (int i = 0; i < NumRouteLanes; ++i)
	{
		TArray<FVector> RouteSimplified;
		float Length =  SimplifyPlolylane(RoutesPoints[i], RouteMinSegmentLength, RouteSimplified, true);

		if (Length <= 0 || RouteSimplified.Num() < 3)
		{
			UE_LOG(LogSoda, Error, TEXT("ATrackBuilder::GenerateLineRoutes(); Simplified line is broken"));
			return;
		}

		ANavigationRoute* Route = GetWorld()->SpawnActor<ANavigationRoute>(RouteSimplified[0], FRotator(0, 0, 0));
		Route->SetRoutePoints(RouteSimplified);
		Route->bAllowForVehicles = true;
		Route->bAllowForPedestrians = false;
		Routes.Add(Route);

		UE_LOG(LogSoda, Log, TEXT("ATrackBuilder::GenerateLineRoutes(); Added route %i ; Vertices num: %i"), i, RouteSimplified.Num());
	}
}

void ATrackBuilder::GenerateRaceRoute()
{
	if (!RacingPoints.Num())
	{
		UE_LOG(LogSoda, Log, TEXT("ATrackBuilder::GenerateRaceRoute(); Racing Points isn't loaded"));
		return;
	}

	TArray<FVector> RouteSimplified;
	float Length = SimplifyPlolylane(RacingPoints, RouteMinSegmentLength, RouteSimplified, true);


	for (int i = 0; i < RouteSimplified.Num(); ++i)
	{
		int Ind = FindNearestPoint2D(RouteSimplified[i], CentrePoints);
		if (Ind > 0) RouteSimplified[i].Z = CentrePoints[Ind].Z;
		RouteSimplified[i] += RouteOffset;
	}

	if (Length <= 0 || RouteSimplified.Num() < 3)
	{
		UE_LOG(LogSoda, Error, TEXT("ATrackBuilder::GenerateRaceRoute(); Simplified line is broken"));
		return;
	}

	ANavigationRoute* Route = GetWorld()->SpawnActor<ANavigationRoute>(RouteSimplified[0], FRotator(0, 0, 0));
	Route->SetRoutePoints(RouteSimplified);
	Route->bAllowForVehicles = true;
	Route->bAllowForPedestrians = false;
	Routes.Add(Route);

	UE_LOG(LogSoda, Log, TEXT("ATrackBuilder::GenerateRaceRoute(); Added racing route"));
}

void ATrackBuilder::ClearRoutes()
{
	for (auto& Route : Routes)
	{
		if (IsValid(Route)) Route->Destroy();
	}
	Routes.Empty();
}

void ATrackBuilder::GenerateMarkings()
{
	ClearMarkings();
	GenerateMarkingsInner(InsidePoints);
	GenerateMarkingsInner(OutsidePoints);
}

void ATrackBuilder::ClearMarkings()
{
	for (auto& it : MarkingMeshes)
	{
		if (IsValid(it))
		{
			it->ClearAllMeshSections();
			it->DestroyComponent();
		}
	}
	MarkingMeshes.Empty();
}

bool ATrackBuilder::GenerateMarkingsInner(const TArray<FVector>& Points)
{
	if (!bJSONLoaded)
	{
		UE_LOG(LogSoda, Error, TEXT("ATrackBuilder::GenerateMarkingsInner(); JSON isn't loaded"));
		return false;
	}

	if (Points.Num() < 3)
	{
		UE_LOG(LogSoda, Error, TEXT("ATrackBuilder::GenerateMarkingsInner(); Input track is broken"));
		return false;
	}

	FPolyline Polyline(Points, bIsClosedTrack, TrackElevation + 3); // + 3 cm under road

	if (bRelax)
	{
		Polyline.Relax(RelaxSpeed, RelaxIterations, RelaxTargetAngel);
	}

	Polyline = Polyline.Simplify(MarkingsMinSegmentLength);
	
	const float Dir = bIsClockwise2D ? -1 : 1;

	float VTexLength = 0;
	for (int i = 0; i < Polyline.Num(); ++i)
	{
		Polyline[i].UV = FVector2D(0.5, VTexLength);
		VTexLength += (Polyline[i].Pos - Polyline[(i + 1) % Polyline.Num()].Pos).Size() * MarkingsTexScaleV;
	}
	float CalibCoeffTexV = float(int(VTexLength)) / VTexLength;
	for (int i = 0; i < Polyline.Num(); ++i)
	{
		Polyline[i].UV.Y *= CalibCoeffTexV;
	}

	int LineInd = 0;
	int MeshID = 0;

	while (true)
	{
		FPolyline Segment(false);


		const int PointsLeft = Polyline.Num() - LineInd;
		const bool IsLastSegment = PointsLeft < (MarkingsStep + MarkingsStep / 2);

		int SegmEndInd;


		if (IsLastSegment)
		{
			SegmEndInd = Polyline.Num() - 1;

		}
		else
		{
			SegmEndInd = LineInd + MarkingsStep - 1;
		}

		for (int i = LineInd; i <= SegmEndInd; ++i) Segment.Vertices.Add(Polyline[i]);

		

		if (IsLastSegment && bIsClosedTrack)
		{
			if ((Polyline.Vertices[0].Pos - Polyline.Vertices.Last().Pos).Size() > MarkingsMinSegmentLength)
			{
				Segment.Vertices.Add(FPolylineVertex(Polyline.Vertices[0].Pos, FVector2D(0.5, VTexLength * CalibCoeffTexV)));
			}
			else
			{
				Segment.Vertices.Last() = FPolylineVertex(Polyline.Vertices[0].Pos, FVector2D(0.5, VTexLength * CalibCoeffTexV));
			}
		}

		/*
		 * Normalize segment texture coordinates
		 */
		FVector2D FirstUV = Segment[0].GetNormalizedUV() - Segment[0].UV;
		for (int i = 0; i < Segment.Num(); ++i) Segment[i].UV.Y += FirstUV.Y;
		Segment.Normalize();

		TArray<FVector> RightVectors2D;
		RightVectors2D.SetNum(Polyline.Num());
		for (int i = 0; i < Segment.Num(); ++i)
		{
			RightVectors2D[i] = (IsLastSegment && (i == (Segment.Num() - 1))) ?
				Polyline.ComputeRightVectors2D(0) :
				Polyline.ComputeRightVectors2D(LineInd + i);
		}

		FPolyline VerticesBorder(true);
		for (int i = 0; i < Segment.Num(); ++i)
		{
			FPolylineVertex Pt = FPolylineVertex(Segment[i].Pos + RightVectors2D[i] * MarkingsWidth * 0.5 * Dir, FVector2D(0, Segment[i].UV.Y));
			VerticesBorder.Vertices.Add(Pt);
		}
		for (int i = Segment.Num() - 1; i >= 0; --i)
		{
			FPolylineVertex Pt = FPolylineVertex(Segment[i].Pos - RightVectors2D[i] * MarkingsWidth * 0.5 * Dir, FVector2D(1, Segment[i].UV.Y));
			VerticesBorder.Vertices.Add(Pt);
		}

		LineInd = SegmEndInd;

		/*
		 * Triangulate
		 */

		UE::Geometry::FConstrainedDelaunay2d Triangulation;
		Triangulation.Vertices.Add({ VerticesBorder.Vertices[0].Pos.X, VerticesBorder.Vertices[0].Pos.Y });
		for (int i = 1; i < VerticesBorder.Vertices.Num(); ++i)
		{
			auto& Point = VerticesBorder.Vertices[i];
			Triangulation.Vertices.Add({ Point.Pos.X, Point.Pos.Y });
			Triangulation.Edges.Add(UE::Geometry::FIndex2i(i - 1, i));
		}
		Triangulation.Edges.Add(UE::Geometry::FIndex2i(VerticesBorder.Vertices.Num() - 1, 0));

		bool bTriangulationSuccess = Triangulation.Triangulate();
		if (!bTriangulationSuccess || Triangulation.Triangles.Num() == 0)
		{
			UE_LOG(LogSoda, Error, TEXT("ATrackBuilder::GenerateMarkingsInner(); Can't triangulateh section %i"), MeshID);
			++MeshID;
			continue;
		}

		/*
		 * Build mesh
		 */

		TArray <FVector> Vertices;
		TArray<FVector2D> UVs;

		for (auto& Point : VerticesBorder.Vertices)
		{
			Vertices.Add(Point.Pos);
			UVs.Add(Point.UV);
		}

		TArray <int32> Triangles;
		for (const UE::Geometry::FIndex3i& Tri : Triangulation.Triangles)
		{
			Triangles.Add(Tri.A);
			Triangles.Add(Tri.B);
			Triangles.Add(Tri.C);
		}

		TArray<FColor> Colors;
		Colors.SetNum(Vertices.Num());
		for (auto& It : Colors) It = FColor(255, 255, 255);
		TArray<FVector> Normals;
		TArray<FProcMeshTangent> Tangents;
		UKismetProceduralMeshLibrary::CalculateTangentsForMesh(Vertices, Triangles, UVs, Normals, Tangents);


		UProceduralMeshComponent* ProceduralMesh = NewObject<UProceduralMeshComponent>(this);
		ProceduralMesh->SetupAttachment(RootComponent);
		ProceduralMesh->RegisterComponent();
		ProceduralMesh->CreateMeshSection(MeshID, Vertices, Triangles, Normals, UVs, Colors, Tangents, false);
		ProceduralMesh->SetCustomDepthStencilValue(int32(ESegmObjectLabel::RoadLines));
		ProceduralMesh->SetRenderCustomDepth(true);
		ProceduralMesh->SetCastShadow(false);
		if (IsValid(MarkingsMaterial))
		{
			ProceduralMesh->SetMaterial(MeshID, MarkingsMaterial);
		}

		MarkingMeshes.Add(ProceduralMesh);

		UE_LOG(LogSoda, Log, TEXT("ATrackBuilder::GenerateRoadMarkings(); ID: %i; Vertices: %i, Triangles: %i"), MeshID, Vertices.Num(), Triangles.Num() / 3);

		if (IsLastSegment) break;

		++MeshID;
	}

	return true;
}

void ATrackBuilder::UpdateGlobalRefpoint()
{
	if (!bRefPointIsOk)
	{
		UE_LOG(LogSoda, Error, TEXT("ATrackBuilder::UpdateGlobalRefpoint(); Refpoint isn't loaded from JSON file"));
		return;
	}

	if (ALevelState* LevelState = ALevelState::Get())
	{
		LevelState->SetGeoReference(RefPointLat, RefPointLon, RefPointAlt);
	}
	else
	{
		UE_LOG(LogSoda, Error, TEXT("ATrackBuilder::UpdateGlobalRefpoint(); Add ALevelState first"));
	}
}

void ATrackBuilder::GenerateStartLine()
{
	ClearStartLine();

	if (!bJSONLoaded)
	{
		UE_LOG(LogSoda, Error, TEXT("ATrackBuilder::GenerateStartLine(); JSON isn't loaded"));
		return;
	}

	FPolyline InsidePolyline(InsidePoints, true, TrackElevation);
	FPolyline OutsidePolyline(OutsidePoints, true, TrackElevation);
	FPolyline CentrePolyline(CentrePoints, true, TrackElevation);

	FVector CenterPoint = CentrePolyline.GetPointByLength(StartLineOffset);
	FPolylineVertex InsidePoint;
	FPolylineVertex OutsidePoint;
	InsidePolyline.ClosestPoint(CenterPoint, InsidePoint);
	OutsidePolyline.ClosestPoint(CenterPoint, OutsidePoint);
	float Length = (InsidePoint.Pos - OutsidePoint.Pos).Size() + BorderWidth * 2 + 20; // Addition 0.2m
	float Yaw = (InsidePoint.Pos - OutsidePoint.Pos).HeadingAngle() / M_PI * 180;

	StartLineDecal = NewObject<UDecalComponent>(this);
	StartLineDecal->SetupAttachment(RootComponent);
	StartLineDecal->SetDecalMaterial(DecalMaterial);
	StartLineDecal->SetRelativeLocation(CentrePolyline.GetPointByLength(StartLineOffset));
	StartLineDecal->SetRelativeRotation(FRotator(-90, Yaw, 0));
	StartLineDecal->DecalSize = FVector(30, StartLineWidth, Length) / 2.0;
	StartLineDecal->RegisterComponent();
}

void ATrackBuilder::GenerateLapCounter()
{
	ClearLapCounter();

	if (!bJSONLoaded)
	{
		UE_LOG(LogSoda, Error, TEXT("ATrackBuilder::GenerateLapCounter(); JSON isn't loaded"));
		return;
	}

	USodaSubsystem* SodaSubsystem = USodaSubsystem::GetChecked();
	ASodaActorFactory* ActorFactory = SodaSubsystem->GetActorFactory();
	check(ActorFactory);

	FPolyline CentrePolyline(CentrePoints, true, TrackElevation);
	const float Yaw = (CentrePoints[1] - CentrePoints[0]).HeadingAngle() / M_PI * 180.0 + 90.0;
	const FTransform Transform(FRotator(0, Yaw, 0), CentrePolyline.GetPointByLength(StartLineOffset) + GetActorLocation());

	LapCounter = Cast<ALapCounter>(ActorFactory->SpawnActor(ALapCounter::StaticClass(), Transform));

}

void ATrackBuilder::ClearLapCounter()
{
	if (IsValid(LapCounter))
	{
		USodaSubsystem* SodaSubsystem = USodaSubsystem::GetChecked();
		ASodaActorFactory* ActorFactory = SodaSubsystem->GetActorFactory();
		check(ActorFactory);

		ActorFactory->RemoveActor(LapCounter, true);
	}
}

void ATrackBuilder::ClearStartLine()
{
	if (IsValid(StartLineDecal))
	{
		StartLineDecal->DestroyComponent();
		StartLineDecal = nullptr;
	}
}

static FVector SegmentIntersection2D(TArray<FVector> & Vertices, bool bIsClosed, const FVector& PointA, const FVector& PointB)
{
	check(Vertices.Num() > 0);
	const int Num = Vertices.Num() - 1 + int(bIsClosed);
	float NearestDist = (Vertices[0] - PointA).Size();
	FVector NearestIntersection = Vertices[0];
	for (int i = 0; i < Num; ++i)
	{
		const FVector& Pt0 = Vertices[i];
		const FVector& Pt1 = Vertices[(i + 1) % Vertices.Num()];

		FVector Intersection;
		if (FMath::SegmentIntersection2D(Pt0, Pt1, PointA, PointB, Intersection))
		{
			float Dist = (PointA - Intersection).Size();
			if (Dist < NearestDist)
			{
				NearestDist = Dist;
				NearestIntersection = Intersection;
			}
		}
	}
	return NearestIntersection;
}

static float ClosestPointOnPolyline2D(const TArray<FVector>& Vertices, bool bIsClosed, const FVector2D & Point, float & NearestDist)
{
	check(Vertices.Num());
	int Ind = 0;
	NearestDist = (FVector2D(Vertices[Ind]) - Point).Size();
	const int Num = Vertices.Num() - 1 + int(bIsClosed);
	for (int i = 0; i < Num; ++i)
	{
		const FVector2D& Pt0 = FVector2D(Vertices[i]);
		const FVector2D& Pt1 = FVector2D(Vertices[(i + 1) % Vertices.Num()]);
		const FVector2D Pt3 = FMath::ClosestPointOnSegment2D(Point, Pt0, Pt1);
		const float Dist = (Point - Pt3).Size();
		if (Dist < NearestDist)
		{
			NearestDist = Dist;
			Ind = i;
		}
	}
	return Ind;
}

static FVector GetPolylineDir(const TArray<FVector>& Vertices, bool bIsClosed, int Ind)
{
	check(Vertices.Num() && Vertices.Num() > Ind && Ind>=0);

	if (Ind == Vertices.Num() - 1)
	{
		if (bIsClosed)
		{
			return (Vertices[Ind] - Vertices[0]).GetSafeNormal();
		}
		else
		{
			return (Vertices[Vertices.Num() - 2] - Vertices[Ind]).GetSafeNormal();
		}
	}
	else
	{
		return (Vertices[Ind+1] - Vertices[Ind]).GetSafeNormal();
	}
}

bool ATrackBuilder::FindNearestBorder(const FTransform& Transform, float& OutLeftOffset, float& OutRightOffset, float& OutCenterLineYaw) const
{
	const FVector2D Pose = FVector2D(Transform.GetLocation());
	const FVector2D Dir = FVector2D(Transform.GetRotation().GetForwardVector());

	float OutsideDist;
	const int OutsideInd = ClosestPointOnPolyline2D(OutsidePoints, bIsClosedTrack, Pose, OutsideDist);
	const FVector2D OutsidePoint(OutsidePoints[OutsideInd]);
	const FVector2D OutsideDir = OutsidePoint - Pose;
	const float OutsideDot = Dir ^ OutsideDir;

	float InsideDist;
	const int InsideInd = ClosestPointOnPolyline2D(InsidePoints, bIsClosedTrack, Pose, InsideDist);
	const FVector2D InsidePoint(InsidePoints[InsideInd]);
	const FVector2D InsideDir = InsidePoint - Pose;
	const float InsideDot = Dir ^ InsideDir;

	const FVector2D CenterLineDir(GetPolylineDir(OutsidePoints, bIsClosedTrack, OutsideInd) + GetPolylineDir(InsidePoints, bIsClosedTrack, InsideInd));

	if (OutsideDot > 0 && InsideDot > 0)
	{
		OutLeftOffset = 0;
		OutRightOffset = 0;
		return false;
	}
	else if (OutsideDot < 0 && InsideDot < 0)
	{
		OutLeftOffset = 0;
		OutRightOffset = 0;
		return false;
	}
	else if (OutsideDot > 0 && InsideDot < 0)
	{
		OutLeftOffset = InsideDist;
		OutRightOffset = OutsideDist;
	}
	else // OutsideDot < 0 && InsideDot > 0
	{
		OutLeftOffset = OutsideDist;
		OutRightOffset = InsideDist;

		//CenterLineDir = -CenterLineDir;
	}

	OutCenterLineYaw = FMath::Atan2(CenterLineDir.Y, CenterLineDir.X);

	//UE_LOG(LogSoda, Warning, TEXT("ATrackBuilder::FindNearestBorder(); %f %f %f %f"), OutLeftOffset, OutRightOffset, OutsideDot, InsideDot);

	return true;;
}

const FSodaActorDescriptor* ATrackBuilder::GenerateActorDescriptor() const
{
	static FSodaActorDescriptor Desc{
		TEXT("Track Builder"), /*DisplayName*/
		TEXT("Tools"), /*Category*/
		TEXT("Experimental"), /*SubCategory*/
		TEXT("SodaIcons.RaceTrack"), /*Icon*/
		true, /*bAllowTransform*/
		true, /*bAllowSpawn*/
		FVector(0, 0, 50), /*SpawnOffset*/
	};
	return &Desc;
}

TSharedPtr<SWidget> ATrackBuilder::GenerateToolBar()
{
	FUniformToolBarBuilder ToolbarBuilder(TSharedPtr<const FUICommandList>(), FMultiBoxCustomization::None);
	ToolbarBuilder.SetStyle(&FSodaStyle::Get(), "PaletteToolBar");
	ToolbarBuilder.BeginSection(NAME_None);
	ToolbarBuilder.AddToolBarButton(
		FUIAction(FExecuteAction::CreateLambda([this] {
			LoadJson();
		})),
		NAME_None,
		FText::FromString("Load"),
		FText::FromString("Load JSON track"),
		FSlateIcon(FSodaStyle::Get().GetStyleSetName(), "SodaVehicleBar.Import"),
		EUserInterfaceActionType::Button
	);

	ToolbarBuilder.AddToolBarButton(
		FUIAction(FExecuteAction::CreateLambda([this] {
			GenerateTrack();
		})),
		NAME_None,
		FText::FromString("Track"),
		FText::FromString("Generate Track"),
		FSlateIcon(FSodaStyle::Get().GetStyleSetName(), "SodaIcons.RaceTrack"),
		EUserInterfaceActionType::Button
	);

	ToolbarBuilder.AddSeparator();
	ToolbarBuilder.AddToolBarButton(
		FUIAction(FExecuteAction::CreateLambda([this] {
			GenerateMarkings();
		})),
		NAME_None,
		FText::FromString("Marks"),
		FText::FromString("Generate Border Marks"),
		FSlateIcon(FSodaStyle::Get().GetStyleSetName(), "SodaIcons.Road"),
		EUserInterfaceActionType::Button
	);

	ToolbarBuilder.AddSeparator();
	ToolbarBuilder.AddToolBarButton(
		FUIAction(FExecuteAction::CreateLambda([this] {

		})),
		NAME_None,
		FText::FromString("Center"),
		FText::FromString("Generate Center Route"),
		FSlateIcon(FSodaStyle::Get().GetStyleSetName(), "SodaIcons.Route"),
		EUserInterfaceActionType::Button
	);

	ToolbarBuilder.AddSeparator();
	ToolbarBuilder.AddToolBarButton(
		FUIAction(FExecuteAction::CreateLambda([this] {

		})),
		NAME_None,
		FText::FromString("Race"),
		FText::FromString("Generate Race Route"),
		FSlateIcon(FSodaStyle::Get().GetStyleSetName(), "SodaIcons.Route"),
		EUserInterfaceActionType::Button
	);


	ToolbarBuilder.EndSection();

	return 
		SNew(SBorder)
		.BorderImage(FAppStyle::GetBrush("WhiteBrush"))
		.BorderBackgroundColor(FStyleColors::Panel)
		.Padding(3)
		[
			FRuntimeEditorUtils::MakeWidget_HackTooltip(ToolbarBuilder)
		];
}

void ATrackBuilder::ScenarioBegin()
{
	ISodaActor::ScenarioBegin();

	using bsoncxx::builder::stream::document;
	using bsoncxx::builder::stream::finalize;
	using bsoncxx::builder::stream::open_document;
	using bsoncxx::builder::stream::close_document;
	using bsoncxx::builder::stream::open_array;
	using bsoncxx::builder::stream::close_array;

	if (bRecordDataset && soda::FDBGateway::Instance().GetStatus() == soda::EDBGatewayStatus::Connected && soda::FDBGateway::Instance().IsDatasetRecording())
	{		
		bsoncxx::builder::stream::document Doc;
		Doc << "json_file" << TCHAR_TO_UTF8(*LoadedFileName);
		Doc << "json_is_valid" << bJSONLoaded;
		if (bJSONLoaded)
		{
			auto PointsArray = Doc << "outside_points" << open_array;
			for (auto& Pt : OutsidePoints)
			{
				PointsArray << open_array << Pt.X << Pt.Y << Pt.Z << close_array;
			}
			PointsArray << close_array;

			PointsArray = Doc << "inside_points" << open_array;
			for (auto& Pt : InsidePoints)
			{
				PointsArray << open_array << Pt.X << Pt.Y << Pt.Z << close_array;
			}
			PointsArray << close_array;

			PointsArray = Doc << "centre_points" << open_array;
			for (auto& Pt : CentrePoints)
			{
				PointsArray << open_array << Pt.X << Pt.Y << Pt.Z << close_array;
			}
			PointsArray << close_array;

			Doc << "lon" << RefPointLon;
			Doc << "lat" << RefPointLat;
			Doc << "alt" << RefPointAlt;
		}
		auto Dataset = soda::FDBGateway::Instance().CreateActorDataset(GetName(), "trackbuilder", GetClass()->GetName(), Doc);
		if (!Dataset)
		{
			SodaApp.GetSodaSubsystemChecked()->ScenarioStop(EScenarioStopReason::InnerError, EScenarioStopMode::RestartLevel, "Can't create dataset for \"" + GetName() + "\"");
		}
	}
}

void ATrackBuilder::ScenarioEnd()
{
	ISodaActor::ScenarioEnd();
}