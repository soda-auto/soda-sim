// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/Actors/OpenDriveTool.h"
#include "Soda/UnrealSoda.h"
#include "DrawDebugHelpers.h"
#include "HAL/FileManager.h"
#include "opendrive/OpenDrive.hpp"
#include "opendrive/geometry/CenterLine.hpp"
#include "opendrive/geometry/GeometryGenerator.hpp"
#include "UObject/ConstructorHelpers.h"
#include "Engine/Texture2D.h"
#include "Materials/Material.h"
#include "Soda/DBGateway.h"
#include "Soda/SodaApp.h"
#include "Soda/SodaGameMode.h"
#include "Soda/UI/SMessageBox.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

#include "bsoncxx/builder/stream/helpers.hpp"
#include "bsoncxx/exception/exception.hpp"
#include "bsoncxx/builder/stream/document.hpp"
#include "bsoncxx/builder/stream/array.hpp"
#include "bsoncxx/json.hpp"

FOpenDriveRoadMarkProfile::FOpenDriveRoadMarkProfile()
{
	MarkType = "solid";
	Label = ESegmObjectLabel::RoadLines;
	Color = FColor(255, 255, 255);
	Width = 30;
}

FOpenDriveRoadMarkProfile::FOpenDriveRoadMarkProfile(const FString& InMarakType, ESegmObjectLabel InLabel, const FColor& InColor, float InWidth)
{
	MarkType = InMarakType;
	Label = InLabel;
	Color = InColor;
	Width = InWidth;
}

AOpenDriveTool::AOpenDriveTool(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = false;

	ConstructorHelpers::FObjectFinderOptional<UTexture2D> IconTextureObject(TEXT("/SodaSim/Assets/CPP/ActorSprites/RoadMark"));

	USceneComponent* SceneComponent = ObjectInitializer.CreateDefaultSubobject<USceneComponent>(this, TEXT("SceneComp"));
	RootComponent = SceneComponent;
	RootComponent->Mobility = EComponentMobility::Static;

#if WITH_EDITORONLY_DATA
	SpriteComponent = ObjectInitializer.CreateEditorOnlyDefaultSubobject<UBillboardComponent>(this, TEXT("Sprite"));
	if (SpriteComponent)
	{
		SpriteComponent->Sprite = IconTextureObject.Get();
		SpriteComponent->SpriteInfo.Category = TEXT("GenerateRoadMark");
		SpriteComponent->SpriteInfo.DisplayName = FText::FromString(TEXT("GenerateRoadMark"));
		SpriteComponent->SetupAttachment(RootComponent);
		SpriteComponent->Mobility = EComponentMobility::Static;
		SpriteComponent->SetEditorScale(1.f);
	}
#endif // WITH_EDITORONLY_DATA

	MarkProfile.Add(FOpenDriveRoadMarkProfile(TEXT("solid"), ESegmObjectLabel::RoadLines_Solid, FColor(255, 0, 0), 20 ));
	MarkProfile.Add(FOpenDriveRoadMarkProfile(TEXT("broken"), ESegmObjectLabel::RoadLines_Dashed, FColor(0, 255, 0), 20));
	MarkProfile.Add(FOpenDriveRoadMarkProfile(TEXT("solid solid"), ESegmObjectLabel::RoadLines_DoubleSide, FColor(255, 255, 0), 40));
	MarkProfile.Add(FOpenDriveRoadMarkProfile(TEXT("solid broken"), ESegmObjectLabel::RoadLines_Solid, FColor(0, 0, 255), 40));
	MarkProfile.Add(FOpenDriveRoadMarkProfile(TEXT("broken solid"), ESegmObjectLabel::RoadLines_Solid, FColor(0, 0, 255), 40));
	
	static ConstructorHelpers::FObjectFinder< UMaterial > MarkMaterialPtr(TEXT("/SodaSim/Assets/CPP/LaneMat"));
	if (MarkMaterialPtr.Succeeded()) MarkMaterial = MarkMaterialPtr.Object;
}

#if WITH_EDITOR
void AOpenDriveTool::PostEditChangeProperty(struct FPropertyChangedEvent& Event)
{
	Super::PostEditChangeProperty(Event);

	/*
	const FName PropertyName = (Event.Property != NULL ? Event.Property->GetFName() : NAME_None);
	if (PropertyName == GET_MEMBER_NAME_CHECKED(AOpenDriveTool, bGenerateMarks))
	{
	}
	*/
}
#endif // WITH_EDITOR

FString AOpenDriveTool::FindXDORFile()
{
	FString MapName = GetWorld()->GetMapName();

#if WITH_EDITOR
	{
		FString CorrectedMapName = MapName;
		constexpr auto PIEPrefix = TEXT("UEDPIE_0_");
		CorrectedMapName.RemoveFromStart(PIEPrefix);
		UE_LOG(LogSoda, Log, TEXT("AOpenDriveTool::FindXDORFile(); Corrected map name from %s to %s"), *MapName, *CorrectedMapName);
		MapName = CorrectedMapName;
	}
#endif // WITH_EDITOR

	MapName += TEXT(".xodr");

	/*
	const FString DefaultFilePath =
		FPaths::ProjectContentDir() +
		TEXT("Carla/Maps/OpenDrive/") +
		MapName;
	*/

	const FString DefaultFilePath = FPaths::ProjectDir() / TEXT("OpenDrive") / MapName;

	auto& FileManager = IFileManager::Get();

	if (FileManager.FileExists(*DefaultFilePath))
	{
		UE_LOG(LogSoda, Log, TEXT("AOpenDriveTool::FindXDORFile(); Found .xdor (%s) for LevelName: %s"), *DefaultFilePath, *MapName);
		return DefaultFilePath;
	}

	/*
	TArray<FString> FilesFound;
	FileManager.FindFilesRecursive(
		FilesFound,
		*FPaths::ProjectContentDir(),
		*MapName,
		true,
		false,
		false);

	if (FilesFound.Num())
	{
		UE_LOG(LogSoda, Log, TEXT("AOpenDriveTool::FindXDORFile(); Found .xdor (%s) for LevelName: %s"), *FilesFound[0], *MapName);
		return FilesFound[0];
	}
	*/

	UE_LOG(LogSoda, Warning, TEXT("AOpenDriveTool::FindXDORFile(); Can't find .xdor for LevelName: %s"), *MapName);
	return FString();
}

bool AOpenDriveTool::FindAndLoadXDOR(bool bForceReload)
{
	if (OpenDriveData && !bForceReload)
	{
		return true;
	}

	FString Path = FindXDORFile();
	if (!Path.Len())
	{
		return false;
	}

	OpenDriveData = MakeShared< opendrive::OpenDriveData>();
	if (!opendrive::Load(std::string(TCHAR_TO_UTF8(*Path)), *OpenDriveData.Get()))
	{
		UE_LOG(LogSoda, Error, TEXT("AOpenDriveTool::FindAndLoadXDOR(); Can't parse OpenDrive file: %s"), *Path);
		OpenDriveData.Reset();
		return false;
	}
	return true;
}

bool AOpenDriveTool::BuildRoadMarks()
{
	ClearMarkings();

	if (!FindAndLoadXDOR(false))
	{
		UE_LOG(LogSoda, Error, TEXT("AOpenDriveTool::BuildRoadMarks(); XDOR isn't loaded"));
		return false;
	}

	for (auto& Profile : MarkProfile)
	{
		Profile.Material = UMaterialInstanceDynamic::Create(MarkMaterial, this);
		Profile.Material->SetVectorParameterValue(TEXT("BaseColor"), Profile.Color);
	}

	for (int road_id = 0; road_id < OpenDriveData->roads.size(); ++road_id)
	{
		std::vector<opendrive::LaneMark> LaneMarkers;
		opendrive::geometry::GenerateRoadMarkLines(OpenDriveData->roads[road_id], LaneMarkers, MarkAccuracy / 100.f);

		for (auto& MarkLine : LaneMarkers)
		{
			auto Profile = FindMarkProfile(UTF8_TO_TCHAR(MarkLine.type.c_str()));

			TArray<FVector> CenterLine;
			for (auto& Pt : MarkLine.Edge) CenterLine.Add(FVector(Pt.x * 100, -Pt.y * 100, Pt.z * 100 + MarkHeight));

			if (bMarkDrawDebug)
			{
				TArray<FVector> Positions;
				for (auto& Pt : MarkLine.Edge)
				{
					Positions.Add(FVector(Pt.x * 100, -Pt.y * 100, Pt.z * 100 + MarkHeight));
				}
				DebugMarkLines.Add(Positions);
			}
			
			if (Profile != nullptr)
			{
				TArray < FVector > Vertices;
				TArray < int32 >Triangles;
				TArray<FVector2D> UVs;
				TArray<FVector> Normals;
				TArray<FColor> Colors;
				TArray<FProcMeshTangent> Tangents;

				TArray<FVector> ForwardVectors;
				for (int i = 0; i < CenterLine.Num() - 1; ++i)  ForwardVectors.Add((CenterLine[i + 1] - CenterLine[i]).GetSafeNormal());
				ForwardVectors.Add(FVector(ForwardVectors.Last()));

				TArray<FVector> RightVectors2D;
				for (int i = 0; i < CenterLine.Num(); ++i)
				{
					RightVectors2D.Add(FRotationMatrix(ForwardVectors[i].GetSafeNormal2D().Rotation()).GetScaledAxis(EAxis::Y));
				}
				
				Vertices.Add(CenterLine[0] + RightVectors2D[0] * Profile->Width);
				Vertices.Add(CenterLine[0] - RightVectors2D[0] * Profile->Width);

				for (int i = 1; i< CenterLine.Num(); ++i)
				{
					FVector RightVector2D;
					float Width;
					if (i == CenterLine.Num() - 1)
					{
						RightVector2D = RightVectors2D[i];
						Width = Profile->Width;
					}
					else
					{
						if (bMarkOptimizeMesh)
						{
							FVector RightVector3D = (ForwardVectors[i - 1] - ForwardVectors[i + 1]);
							if (!RightVector3D.Normalize(1.e-6f)) continue;
						}

						RightVector2D = (RightVectors2D[i - 1] + RightVectors2D[i + 1]).GetSafeNormal2D();
						if (RightVector2D.IsZero())
						{
							RightVector2D = RightVectors2D[i];
							Width = Profile->Width;
						}
						else
						{
							float A = FMath::Acos(RightVectors2D[i - 1].CosineAngle2D(-RightVectors2D[i + 1])) / 2;
							Width = std::fabsf(Profile->Width / std::sin(A));
						}
					}

					Vertices.Add(CenterLine[i] + RightVector2D * Width);
					Vertices.Add(CenterLine[i] - RightVector2D * Width);

					int Num = Vertices.Num() - 1;
					Triangles.Add(Num - 1);
					Triangles.Add(Num - 2);
					Triangles.Add(Num - 3);
					Triangles.Add(Num - 1);
					Triangles.Add(Num + 0);
					Triangles.Add(Num - 2);
				}

				UProceduralMeshComponent* MarkMesh = NewObject<UProceduralMeshComponent>(this);
				MarkMesh->RegisterComponent();
				MarkMesh->CreateMeshSection(road_id, Vertices, Triangles, Normals, UVs, Colors, Tangents, false);
				MarkMesh->SetMaterial(road_id, Profile->Material);
				MarkMesh->bCastCinematicShadow = false;
				MarkMesh->bCastDynamicShadow = false;
				MarkMesh->bCastStaticShadow = false;
				MarkMesh->bCastFarShadow = false;
				MarkMesh->SetCastShadow(false);
				MarkMesh->SetCastInsetShadow(false);
				MarkMesh->SetCustomDepthStencilValue((uint8)Profile->Label);
				MarkMesh->SetRenderCustomDepth(true);
				MarkMeshes.Add(MarkMesh);
			}
		}
	}

	return true;
}

bool AOpenDriveTool::BuildRoutes()
{
	ClearRoutes();

	if (!FindAndLoadXDOR(false))
	{
		UE_LOG(LogSoda, Error, TEXT("AOpenDriveTool::BuildRoutes(); XDOR isn't loaded"));
		return false;
	}

	for (int road_id = 0; road_id < OpenDriveData->roads.size(); ++road_id)
	{
		//auto& lanes = OpenDriveData->roads[road_id].lanes;
		auto& laneSections = OpenDriveData->roads[road_id].lanes.lane_sections;
		opendrive::geometry::CenterLine centerLine;

		if (!opendrive::geometry::generateCenterLine(OpenDriveData->roads[road_id], centerLine))
		{
			return false;
		}

		for (auto it = laneSections.begin(); it != laneSections.end(); it++)
		{
			auto& laneSection = *it;
			auto laneSectionIndex = static_cast<uint32_t>(it - laneSections.begin()) + 1;

			// the tolerance at the end is necessary so that the lane offset applies only to the current lane section
			double s0 = laneSection.start_position;
			double s1 = laneSection.end_position;

			/*
			if ((s1 - s0) < MinimumSegmentLength)
			{
				std::cerr << "Invalid lane section length s1, s0\n";
				continue;
			}
			*/

			auto EvalBorderOffset = [&](const std::vector<opendrive::LaneInfo>& LanesInfo, const opendrive::LaneInfo& target_line, double s)
			{
				double borderOffset = 0.;
				for (auto& laneInfo : LanesInfo)
				{
					auto width = opendrive::geometry::laneWidth(laneInfo.lane_width, s - s0);
					borderOffset += width;

					if (&laneInfo == &target_line)
					{
						borderOffset -= width / 2;
						break;
					}
				}

				return borderOffset;
			};


			auto AddRoute = [&](const std::vector<opendrive::LaneInfo>& LanesInfo, const opendrive::LaneInfo& laneInfo, int dir)
			{
				if (laneInfo.attributes.type != opendrive::LaneType::Driving)
				{
					return;
				}

				TArray<FVector> Points;

				int count = int(centerLine.length / (RouteAccuracy / 100.0f) + 1);

				const double step = double(centerLine.length) / count;

				for (int j = 0; j <= count; ++j)
				{
					double s = s0 + step * j;
					auto centerPoint = centerLine.eval(s);
					double height = centerLine.evalElevation(s);
					double borderOffset = EvalBorderOffset(LanesInfo, laneInfo, s);

					switch (dir)
					{
					case -1: //right
						centerPoint.ApplyLateralOffset(-borderOffset);
						break;
					case 1: //left
						centerPoint.ApplyLateralOffset(borderOffset);
						break;
					}
					//laneMark.Edge.push_back({ centerPoint.location.x, centerPoint.location.y, height });

					Points.Add(FVector( centerPoint.location.x * 100, -centerPoint.location.y * 100, height * 100 + RoutesHeight));
				}

				if (Points.Num() > 1)
				{
					if (dir == 1)
					{
						for (int i = 0; i < Points.Num() / 2; ++i)
						{
							std::swap(Points[i], Points[Points.Num() - 1 - i]);
						}
					}
					ANavigationRoute* Route = GetWorld()->SpawnActor<ANavigationRoute>(Points[0], FRotator(0, 0, 0));
					Route->SetRoutePoints(Points);
					Route->bAllowForVehicles = true;
					Route->bAllowForPedestrians = false;
					Routes.Add(Route);
				}
			};


			for (auto& laneInfo : laneSection.left)
				AddRoute(laneSection.left, laneInfo, 1);
			for (auto& laneInfo : laneSection.right)
				AddRoute(laneSection.right, laneInfo, -1);
			AddRoute(laneSection.center, laneSection.center[0], 0);

		}
	}

	return true;
}

void AOpenDriveTool::ClearRoutes()
{
	for (auto& Route : Routes)
	{
		if (IsValid(Route)) Route->Destroy();
	}
	Routes.Empty();
}

void AOpenDriveTool::ClearMarkings()
{
	for (auto& it : MarkMeshes)
	{
		if (IsValid(it))
		{
			it->ClearAllMeshSections();
			it->DestroyComponent();
		}
	}
	MarkMeshes.Empty();
	DebugMarkLines.Empty();
}

void AOpenDriveTool::DrawDebugMarks() const
{
	for (int i = 0; i < DebugMarkLines.Num(); ++i)
	{
		for (int j = 0; j < DebugMarkLines[i].Num() - 1; ++j)
		{
			const FVector p0 = DebugMarkLines[i][j + 0];
			const FVector p1 = DebugMarkLines[i][j + 1];

			DrawDebugLine(GetWorld(), p0, p1, FColor(0, 0, 255), true, -1.f, 0, 10);
		}
	}
}

FOpenDriveRoadMarkProfile* AOpenDriveTool::FindMarkProfile(const FString& MarakType)
{
	for (auto& Profile : MarkProfile)
	{
		if (Profile.MarkType == MarakType) return &Profile;
	}
	return nullptr;
}

void AOpenDriveTool::BeginPlay()
{
	Super::BeginPlay();

	if (bMarkGenerateOnBeginPlay)
	{
		BuildRoadMarks();
	}
}

void AOpenDriveTool::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
}

void AOpenDriveTool::TickActor(float DeltaTime, enum ELevelTick TickType, FActorTickFunction& ThisTickFunction)
{
	Super::TickActor(DeltaTime, TickType, ThisTickFunction);
}

void AOpenDriveTool::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
}

void AOpenDriveTool::ScenarioBegin()
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
		if (OpenDriveData.IsValid())
		{
			bsoncxx::builder::stream::document Doc;

			if (bStoreLanesMarks)
			{
				auto laneArray = Doc << "road_lanes_marks" << open_array;
				for (int road_id = 0; road_id < OpenDriveData->roads.size(); ++road_id)
				{
					std::vector<opendrive::LaneMark> LaneMarkers;
					opendrive::geometry::GenerateRoadMarkLines(OpenDriveData->roads[road_id], LaneMarkers, MarkAccuracy / 100.f);

					auto laneDoc = laneArray << open_document;
					for (auto& MarkLine : LaneMarkers)
					{
						laneDoc << "type" << MarkLine.type;
						auto ptArray = laneDoc << "points" << open_array;
						for (auto& Pt : MarkLine.Edge)
						{
							FVector Pt2{ Pt.x * 100, -Pt.y * 100, Pt.z * 100 };
							ptArray << open_array << Pt2.X << Pt2.Y << Pt2.Z << close_array;
						}
						ptArray << close_array;
					}
					laneDoc << close_document;
				}
				laneArray << close_array;
			}

			if (bStoreXDOR)
			{
				FString FileName = FindXDORFile();
				FString FileContent;
				if (!FileName.IsEmpty())
				{
					if (FFileHelper::LoadFileToString(FileContent, *FileName))
					{
						Doc << "xdor" << TCHAR_TO_UTF8(*FileContent);
					}
					else
					{
						UE_LOG(LogSoda, Error, TEXT("AOpenDriveTool::ScenarioBegin(); Can't load %s"), *FileName);
					}
				}
				else
				{
					UE_LOG(LogSoda, Error, TEXT("AOpenDriveTool::ScenarioBegin(); Can't find XDOR file"));
				}
			}

			auto Dataset = soda::FDBGateway::Instance().CreateActorDataset(GetName(), "opendrive", GetClass()->GetName(), Doc);
			if (!Dataset)
			{
				SodaApp.GetGameModeChecked()->ScenarioStop(EScenarioStopReason::InnerError, EScenarioStopMode::RestartLevel, "Can't create dataset for \"" + GetName() + "\"");
			}
		}
		else
		{
			UE_LOG(LogSoda, Error, TEXT("AOpenDriveTool::ScenarioBegin(); Can't find OpenDriveData isn't valid"));
		}
	}
}

void AOpenDriveTool::ScenarioEnd()
{
	ISodaActor::ScenarioEnd();
}

const FSodaActorDescriptor* AOpenDriveTool::GenerateActorDescriptor() const
{
	static FSodaActorDescriptor Desc{
		TEXT("Open Drive Tool"), /*DisplayName*/
		TEXT("Tools"), /*Category*/
		TEXT(""), /*SubCategory*/
		TEXT("SodaIcons.Road"), /*Icon*/
		false, /*bAllowTransform*/
		true, /*bAllowSpawn*/
		FVector(0, 0, 50), /*SpawnOffset*/
	};
	return &Desc;
}