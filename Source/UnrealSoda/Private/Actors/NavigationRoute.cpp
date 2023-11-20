// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/Actors/NavigationRoute.h"
#include "Soda/UnrealSoda.h"
#include "Soda/Misc/SodaRandomEngine.h"
#include "Soda/VehicleComponents/Inputs/VehicleInputAIComponent.h"
#include "Runtime/Json/Public/Dom/JsonObject.h"
#include "Runtime/Json/Public/Serialization/JsonSerializer.h"
#include "Engine/CollisionProfile.h"
#include "DrawDebugHelpers.h"
#include "Soda/Vehicles/SodaVehicle.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"
#include "ProceduralMeshComponent.h"
#include "KismetProceduralMeshLibrary.h"
#include "Slate/SceneViewport.h"
#include "Soda/Misc/EditorUtils.h"
#include "Soda/SodaGameViewportClient.h"
#include "Soda/SodaGameMode.h"
#include "Soda/SodaActorFactory.h"
#include "Soda/Editor/SodaSelection.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/StaticMesh.h"
#include "Soda/SodaApp.h"
#include "Soda/SodaUserSettings.h"

static bool IsSplineValid(const URouteSplineComponent* SplineComponent)
{
	return (SplineComponent != nullptr) &&
		   (SplineComponent->GetNumberOfSplinePoints() > 1);
}

/*****************************************************************************************
                                ANavigationRoute
*****************************************************************************************/

ANavigationRoute::ANavigationRoute(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	Spline = CreateDefaultSubobject< URouteSplineComponent >(TEXT("SplineComponent"));
	Spline->SetMobility(EComponentMobility::Movable);
	Spline->bHiddenInGame = true;
#if WITH_EDITOR
	Spline->EditorUnselectedSplineSegmentColor = FLinearColor(1.f, 0.15f, 0.15f);
#endif // WITH_EDITOR
	RootComponent = Spline;

	TriggerVolume = CreateDefaultSubobject< UBoxComponent >(TEXT("TriggerVolume"));
	TriggerVolume->SetHiddenInGame(true);
	//TriggerVolume->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Overlap);
	//TriggerVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	TriggerVolume->CanCharacterStepUpOn = ECanBeCharacterBase::ECB_Yes;
	//TriggerVolume->SetGenerateOverlapEvents(true);
	TriggerVolume->SetBoxExtent(FVector{ 64.0f, 64.0f, 64.0f });
	//TriggerVolume->SetMobility(EComponentMobility::Movable);
	TriggerVolume->SetupAttachment(RootComponent);

	ProceduralMesh = CreateDefaultSubobject< UProceduralMeshComponent >(TEXT("ProceduralMeshComponent"));
	ProceduralMesh->bUseComplexAsSimpleCollision = true;
	ProceduralMesh->bCastCinematicShadow = false;
	ProceduralMesh->bCastDynamicShadow = false;
	ProceduralMesh->bCastStaticShadow = false;
	ProceduralMesh->bCastFarShadow = false;
	ProceduralMesh->SetCastShadow(false);
	ProceduralMesh->SetCastInsetShadow(false);
	//ProceduralMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Overlap);
	//ProceduralMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	ProceduralMesh->CanCharacterStepUpOn = ECanBeCharacterBase::ECB_No;
	//ProceduralMesh->SetGenerateOverlapEvents(true);
	ProceduralMesh->SetMobility(EComponentMobility::Movable);
	ProceduralMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	ProceduralMesh->SetCollisionResponseToChannel(SodaApp.GetSodaUserSettings()->SelectTraceChannel, ECollisionResponse::ECR_Block);
	ProceduralMesh->SetCollisionObjectType(SodaApp.GetSodaUserSettings()->SelectTraceChannel);
	ProceduralMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	ProceduralMesh->SetupAttachment(RootComponent);

	RandomEngine = CreateDefaultSubobject< USodaRandomEngine >(TEXT("RandomEngine"));
	RandomEngine->Seed(RandomEngine->GenerateRandomSeed());

	//PrimaryActorTick.bCanEverTick = true;
	//PrimaryActorTick.TickGroup = TG_PrePhysics;

	static ConstructorHelpers::FObjectFinder< UMaterial > Mat(TEXT("/SodaSim/Assets/CPP/RouteMat"));
	if (Mat.Succeeded()) MatProcMesh = Mat.Object;
}

bool ANavigationRoute::SetRoutePoints(const TArray<FVector> & RoutePoints, bool bUpdateMesh)
{
	if (RoutePoints.Num() < 2) return false;
	SetActorLocation(RoutePoints[0]);
	Spline->SetSplinePoints(RoutePoints, ESplineCoordinateSpace::World, true);
	if(bUpdateMesh) UpdateViewMesh();
	return true;
}

void ANavigationRoute::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void ANavigationRoute::BeginPlay()
{
	Super::BeginPlay();

	if (MatProcMesh)
	{
		MatProcMeshDyn = UMaterialInstanceDynamic::Create(MatProcMesh, this);
		MatProcMeshDyn->SetVectorParameterValue(TEXT("BaseColor"), BaseColor);
	}

	UpdateViewMesh();
}

void ANavigationRoute::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
}

void ANavigationRoute::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);

	if (Ar.IsSaveGame())
	{
		if (Ar.IsSaving())
		{
			int RoutesNum = 1;
			Ar << RoutesNum;

			if (IsSplineValid(Spline))
			{
				int PointsNum = Spline->GetNumberOfSplinePoints();
				Ar << PointsNum;
				for (int i = 0; i < PointsNum; ++i)
				{
					FVector Point = Spline->GetLocationAtSplinePoint(i, ESplineCoordinateSpace::World);
					Ar << Point;
				}
			}
			else
			{
				int PointsNum = 0;
				Ar << PointsNum;
			}

		}
		else if (Ar.IsLoading())
		{
			int RoutesNum;
			Ar << RoutesNum;

			int PointsNum = 0;
			Ar << PointsNum;
			TArray<FVector> Ponints;
			if (PointsNum >= 0)
			{
				for (int j = 0; j < PointsNum; ++j)
				{
					FVector Point;
					Ar << Point;
					Ponints.Add(Point);
				}

			}
			SetRoutePoints(Ponints, false);
		}
	}
}

/*
ANavigationRoute* ANavigationRoute::GetRandomSuccessor() const
{
	check(RandomEngine);

	if (Successors.Num() == 0)
	{
		return nullptr;
	}

	if (Successors.Num() == 1)
	{
		return Successors[0];
	}

	TArray<float> Probabilities;
	for (int i = 0; i < Successors.Num(); ++i)
	{
		Probabilities.Add(Successors[i]->Probability);
	}
	auto Index = RandomEngine->GetIntWithWeight(Probabilities);
	check((Index >= 0) && (Index < Successors.Num()));
	return Successors[Index];
}
*/


bool ANavigationRoute::UpdateProcedureMeshSegment(int SegmentIndex)
{
	static const float RouteWidth = 40;
	static const float SegmStep = 20.0;
	static const float MaxDistErr = 15;
	static const float MaxCosAngErr = 0.99619469809; //cos(5deg) = 0.99619469809

	static TArray < FVector > Vertices;
	static TArray < int32 > Triangles;
	static TArray < FVector2D > UVs;
	static TArray < FVector > Normals;
	static TArray < FColor > Colors;
	static TArray < FProcMeshTangent > Tangents;

	check(Spline && ProceduralMesh);

	if (SegmentIndex < 0 || SegmentIndex >= (Spline->GetNumberOfSplinePoints() - 1))
	{
		return false;
	}

	auto CreateCap = [this](int NodeIndex, int SectionIndex, float Yaw)
	{
		Vertices.SetNum(0, false);
		Triangles.SetNum(0, false);
		UVs.SetNum(0, false);
		Normals.SetNum(0, false);
		Colors.SetNum(0, false);
		Tangents.SetNum(0, false);

		FQuat Quat = Spline->GetQuaternionAtSplinePoint(NodeIndex, ESplineCoordinateSpace::Local);
		FTransform Transform = Spline->GetTransformAtSplinePoint(NodeIndex, ESplineCoordinateSpace::Local);
		//UpVector = Spline->GetComponentTransform().TransformVectorNoScale(Quat.RotateVector(FVector::UpVector));

		Vertices.Add(Transform.GetLocation());
		const int Step = 10;
		for (int i = 0; i <= Step; ++i)
		{
			FRotator Rot(0, float(i) / Step * 180.0 + Yaw, 0);
			Vertices.Add(Transform.TransformPosition(Rot.RotateVector(FVector(RouteWidth * 0.5, 0, 0))));
		}

		for (int i = 0; i < Step; ++i)
		{
			Triangles.Append({ 0, i + 1, i + 2 });
		}

		if (FProcMeshSection* Section = ProceduralMesh->GetProcMeshSection(SectionIndex))
		{
			ProceduralMesh->UpdateMeshSection(SectionIndex, Vertices, Normals, UVs, Colors, Tangents);
		}
		else
		{
			UKismetProceduralMeshLibrary::CalculateTangentsForMesh(Vertices, Triangles, UVs, Normals, Tangents);
			ProceduralMesh->CreateMeshSection(SectionIndex, Vertices, Triangles, Normals, UVs, Colors, Tangents, true);
			ProceduralMesh->SetMaterial(SectionIndex, MatProcMeshDyn);
		}
	};

	if (SegmentIndex == 0 || ProceduralMesh->GetNumSections() < 2)
	{
		CreateCap(0, 0, 90);
	}

	if (SegmentIndex == Spline->GetNumberOfSplinePoints() - 2 || ProceduralMesh->GetNumSections() < 2)
	{
		CreateCap(Spline->GetNumberOfSplinePoints() - 1, 1, -90);
	}
	
	Vertices.SetNum(0, false);
	Triangles.SetNum(0, false);
	UVs.SetNum(0, false);
	Normals.SetNum(0, false);
	Colors.SetNum(0, false);
	Tangents.SetNum(0, false);

	const float SplineOffest = Spline->GetDistanceAlongSplineAtSplinePoint(SegmentIndex);
	const float SplineLength = Spline->GetSplineLength();
	const float SegmLength = Spline->GetDistanceAlongSplineAtSplinePoint(SegmentIndex + 1) - Spline->GetDistanceAlongSplineAtSplinePoint(SegmentIndex);
	const int SegmCount = int(SegmLength / SegmStep + 0.5);
	const float SegmStepR = SegmLength / SegmCount;
	const float UVSplineOffest = int(SplineOffest / RouteWidth);

	FVector CenterPoint = Spline->GetLocationAtDistanceAlongSpline(SplineOffest, ESplineCoordinateSpace::Local);
	FQuat Quat = Spline->GetQuaternionAtDistanceAlongSpline(SplineOffest, ESplineCoordinateSpace::Local);
	FVector ForwardVector = Quat.RotateVector(FVector::ForwardVector);
	FVector UpVector = Quat.RotateVector(FVector::UpVector);
	FVector RightVector = Quat.RotateVector(FVector::RightVector);
	FVector PreForwardVector = ForwardVector;
	FVector PreCenterPoint = CenterPoint;

	Vertices.Add(CenterPoint + RightVector * RouteWidth * 0.5);
	Vertices.Add(CenterPoint - RightVector * RouteWidth * 0.5);
	Normals.Add(UpVector);
	Normals.Add(UpVector);
	UVs.Add({ 0, SplineOffest / RouteWidth - UVSplineOffest });
	UVs.Add({ 1, SplineOffest / RouteWidth - UVSplineOffest });

	for (int j = 1; j < SegmCount + 1; ++j)
	{
		const float Length = SplineOffest + SegmStepR * j;
		Quat = Spline->GetQuaternionAtDistanceAlongSpline(Length, ESplineCoordinateSpace::Local);
		CenterPoint = Spline->GetLocationAtDistanceAlongSpline(Length, ESplineCoordinateSpace::Local);
		ForwardVector = Quat.RotateVector(FVector::ForwardVector);

		if (j != SegmCount &&
			std::abs(PreForwardVector | ForwardVector) > MaxCosAngErr &&
			(CenterPoint - (PreCenterPoint + PreForwardVector * (CenterPoint - PreCenterPoint).Size())).Size() < MaxDistErr)
		{
			continue;
		}

		PreForwardVector = ForwardVector;
		PreCenterPoint = CenterPoint;
		UpVector = Quat.RotateVector(FVector::UpVector);
		RightVector = Quat.RotateVector(FVector::RightVector);

		Vertices.Add(CenterPoint + RightVector * RouteWidth * 0.5);
		Vertices.Add(CenterPoint - RightVector * RouteWidth * 0.5);
		Normals.Add(UpVector);
		Normals.Add(UpVector);
		UVs.Add({ 0, Length / RouteWidth - UVSplineOffest });
		UVs.Add({ 1, Length / RouteWidth - UVSplineOffest });

		const int vNum = Vertices.Num() - 1;
		Triangles.Add(vNum - 1);
		Triangles.Add(vNum - 2);
		Triangles.Add(vNum - 3);
		Triangles.Add(vNum - 1);
		Triangles.Add(vNum - 0);
		Triangles.Add(vNum - 2);
	}

	const int SectionIndex = SegmentIndex + 2;
	FProcMeshSection* Section =  ProceduralMesh->GetProcMeshSection(SectionIndex);
	if (Section && Section->ProcVertexBuffer.Num() == Vertices.Num())
	{
		ProceduralMesh->UpdateMeshSection(SectionIndex, Vertices, Normals, UVs, Colors, Tangents);
	}
	else
	{
		UKismetProceduralMeshLibrary::CalculateTangentsForMesh(Vertices, Triangles, UVs, Normals, Tangents);
		ProceduralMesh->CreateMeshSection(SectionIndex, Vertices, Triangles, Normals, UVs, Colors, Tangents, true);
		ProceduralMesh->SetMaterial(SectionIndex, MatProcMeshDyn);
	}


	return true;
}

void ANavigationRoute::UpdateViewMesh()
{
	ProceduralMesh->ClearAllMeshSections();
	const int Num = Spline->GetNumberOfSplinePoints();
	for (int i = 0; i < Num - 1; ++i)
	{
		UpdateProcedureMeshSegment(i);
	}
}

void ANavigationRoute::FitActorPosition()
{
	if (Spline->GetNumberOfSplinePoints() == 0) return;
	TArray<FVector> TmpPoints;
	for (int i = 0; i < Spline->GetNumberOfSplinePoints(); ++i)
	{
		TmpPoints.Add(Spline->GetLocationAtSplinePoint(i, ESplineCoordinateSpace::World));
	}
	
	SetRoutePoints(TmpPoints, false);
}

void ANavigationRoute::PropagetUpdatePredecessorRoute()
{
	/*
	if (PredecessorRoute.IsValid())
	{
		PredecessorRoute->Spline->SetLocationAtSplinePoint(PredecessorRoute->Spline->GetNumberOfSplinePoints() - 1, Spline->GetLocationAtSplinePoint(0, ESplineCoordinateSpace::World), ESplineCoordinateSpace::World);
		PredecessorRoute->UpdateViewMesh();
	}
	*/
}

void ANavigationRoute::PropagetUpdateSuccessorRoute()
{
	/*
	if (SuccessorRoute.IsValid())
	{
		SuccessorRoute->Spline->SetLocationAtSplinePoint(0, Spline->GetLocationAtSplinePoint(Spline->GetNumberOfSplinePoints() - 1, ESplineCoordinateSpace::World), ESplineCoordinateSpace::World);
		SuccessorRoute->FitActorPosition();
		SuccessorRoute->UpdateViewMesh();
	}
	*/
}

void ANavigationRoute::UpdateRouteNetwork(const ANavigationRoute* Initiator, bool bIsInitiatorPredecessor)
{
	USodaGameModeComponent* GameMode = USodaGameModeComponent::GetChecked();
	ASodaActorFactory * ActorFactory = GameMode->GetActorFactory();
	check(ActorFactory);

	for (AActor* Actor : ActorFactory->GetActors())
	{
		if (ANavigationRoute* Route = Cast<ANavigationRoute>(Actor))
		{

		}
	}
}

bool ANavigationRoute::IsRouteTagsAllowed(const TSet<FString>& AllowedRouteTags) const
{
	for (auto & It : AllowedRouteTags)
	{
		if (RouteTags.Contains(It))
		{
			return true;
		}
	}
	return false;
}

/*****************************************************************************************
								URoutePlannerEditableNode
*****************************************************************************************/

URoutePlannerEditableNode::URoutePlannerEditableNode(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	static ConstructorHelpers::FObjectFinder< UStaticMesh > MeshAsset(TEXT("/Engine/BasicShapes/Cylinder"));
	static ConstructorHelpers::FObjectFinder< UMaterial > MatAsset(TEXT("/SodaSim/Assets/CPP/AxesMatDense"));
	if (MeshAsset.Succeeded()) 	SetStaticMesh(MeshAsset.Object);
	if (MatAsset.Succeeded()) 	SetMaterial(0, MatAsset.Object);
	PrimaryComponentTick.bCanEverTick = true;

	//SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	//SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Overlap);
	//SetCollisionEnabled(ECollisionEnabled::QueryOnly);

	SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	SetCollisionResponseToChannel(SodaApp.GetSodaUserSettings()->SelectTraceChannel, ECollisionResponse::ECR_Block);
	SetCollisionObjectType(SodaApp.GetSodaUserSettings()->SelectTraceChannel);
	SetCollisionEnabled(ECollisionEnabled::QueryOnly);

	CanCharacterStepUpOn = ECanBeCharacterBase::ECB_No;

	BaseColor = FLinearColor(1.0, 1.0, 1.0);
	HoverColor = FLinearColor(0.2, 0.2, 0.0);
	SetWorldScale3D({ 0.4, 0.4, 0.03 });

	bCastCinematicShadow = false;
	bCastDynamicShadow = false;
	bCastStaticShadow = false;
	bCastFarShadow = false;
	SetCastShadow(false);
	SetCastInsetShadow(false);
}

void URoutePlannerEditableNode::BeginPlay()
{
	Super::BeginPlay();

	if (UMaterialInterface* Mat = GetMaterial(0))
	{
		MatDyn = UMaterialInstanceDynamic::Create(Mat, this);
		SetMaterial(0, MatDyn);
		MatDyn->SetVectorParameterValue(TEXT("BaseColor"), BaseColor);
	}
}

void URoutePlannerEditableNode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
}

void URoutePlannerEditableNode::LightNode(bool bLight)
{
	if (MatDyn)
	{
		MatDyn->SetVectorParameterValue(TEXT("BaseColor"), bLight ? HoverColor : BaseColor);
	}
}

/*****************************************************************************************
								ANavigationRouteEditable
*****************************************************************************************/

ANavigationRouteEditable::ANavigationRouteEditable(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PrePhysics;

	PreviewNode = CreateDefaultSubobject< URoutePlannerEditableNode >(TEXT("PreviewNode"));
	PreviewNode->PointIndex = -1;
	PreviewNode->bHiddenInGame = true;
	PreviewNode->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	PreviewNode->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PreviewNode->SetGenerateOverlapEvents(false);
}

void ANavigationRouteEditable::BeginPlay()
{
	Super::BeginPlay();
}

void ANavigationRouteEditable::EndPlay(EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
}

void ANavigationRouteEditable::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bIsSelected)
	{
		UWorld* World = GetWorld();
		check(World);

		USodaGameViewportClient* GameViewportClient = Cast<USodaGameViewportClient>(World->GetGameViewport());
		if (!GameViewportClient)
		{
			return;
		}

		if (bIsUnfreezeNotified)
		{
			bIsUnfreezeNotified = false;
			GameViewportClient->SetEditorMouseCaptureMode(EEditorMouseCaptureMode::Default);
		}

		/*
		if (GameViewportClient->Viewport->HasMouseCapture())
		{
			return;
		}
		*/

		APlayerController* PlayerController = World->GetFirstPlayerController();
		check(PlayerController);
		FSceneViewport* SceneViewport = GameViewportClient->GetGameViewport();
		check(SceneViewport);

		const bool bMousePresed = SceneViewport->KeyState(EKeys::LeftMouseButton);
		const bool bLeftCtrPresed = PlayerController->IsInputKeyDown(EKeys::LeftControl);
		URoutePlannerEditableNode* PreHoverNode = HoverNode;
		URoutePlannerEditableNode* PreSelectedNode = SelectedNode;
		bool bHidePreviewNode = true;

		TArray<FHitResult> HitResults; 
		TraceForMousePosition(PlayerController, HitResults);

		HoverNode = nullptr;		

		if (!SelectedNode && !PreviewNode->IsVisible() && !bLeftCtrPresed)
		{
			// Find Hover Node
			for (auto& Hit : HitResults)
			{
				if (Hit.GetActor() == this)
				{
					URoutePlannerEditableNode* Node = Cast<URoutePlannerEditableNode>(Hit.GetComponent());
					if (Node)
					{
						HoverNode = Node;
						break;
					}
				}
			}

			// Select Node
			if (bMousePresed && HoverNode)
			{
				SelectedNode = HoverNode;
				SelectedNode->LightNode(true);
				HoverNode = nullptr;
			}
		}

		// Unselect node
		if (SelectedNode && !bMousePresed)
		{
			SelectedNode->LightNode(false);
			SelectedNode = nullptr;
			bIsUnfreezeNotified = true;
		}

		// Light hovered node
		if (PreHoverNode != HoverNode)
		{
			if (PreHoverNode) PreHoverNode->LightNode(false);
			if (HoverNode) HoverNode->LightNode(true);
		}

		// Preview node
		if (bLeftCtrPresed && !SelectedNode)
		{
			// Draw  Preview node
			float Key{};
			FVector Location{};
			for (auto& Hit : HitResults)
			{
				if (Hit.GetComponent() && Hit.GetComponent() == ProceduralMesh)
				{
					Key = Spline->FindInputKeyClosestToWorldLocation(Hit.Location);
					Location = Spline->GetLocationAtSplineInputKey(Key, ESplineCoordinateSpace::World);
					PreviewNode->SetWorldLocation(Location);
					bHidePreviewNode = false;
					break;
				}
			}

			// Add node
			if (!bHidePreviewNode && bMousePresed)
			{
				if (!bHidePreviewNode && SceneViewport->KeyState(EKeys::LeftMouseButton))
				{
					TArray<FVector> TmpPoints;
					for (int i = 0; i < Spline->GetNumberOfSplinePoints(); ++i)
					{
						TmpPoints.Add(Spline->GetLocationAtSplinePoint(i, ESplineCoordinateSpace::World));
					}
					const int InsertNode = int(Key) + 1;
					TmpPoints.Insert(Location, InsertNode);
					SetRoutePoints(TmpPoints, false);
					UpdateViewMesh();
					UpdateNodes();
					SelectedNode = EditableNodes[InsertNode];
					bHidePreviewNode = true;
					MarkAsDirty();
				}
			}
		}

		// Move selected node
		if (SelectedNode)
		{
			for (auto& Hit : HitResults)
			{
				if (Hit.GetActor() != this && !Hit.GetActor()->IsRootComponentMovable() && (Hit.GetComponent()->GetCollisionObjectType() == ECollisionChannel::ECC_WorldStatic || Hit.GetComponent()->GetCollisionObjectType() == ECollisionChannel::ECC_WorldDynamic))
				{
					const float ZOffset = 50;
					const FVector Dir = (Hit.TraceEnd - Hit.TraceStart).GetSafeNormal();
					FVector HitLocation = Hit.Location + Dir * ZOffset / Dir.Z;
					SelectedNode->SetWorldLocation(HitLocation);
					Spline->SetLocationAtSplinePoint(SelectedNode->PointIndex, HitLocation, ESplineCoordinateSpace::World, true);
					UpdateProcedureMeshSegment(SelectedNode->PointIndex - 2);
					UpdateProcedureMeshSegment(SelectedNode->PointIndex - 1);
					UpdateProcedureMeshSegment(SelectedNode->PointIndex + 0);
					UpdateProcedureMeshSegment(SelectedNode->PointIndex + 1);
					UpdateProcedureMeshSegment(SelectedNode->PointIndex + 2);
					if (SelectedNode->PointIndex == 0)
					{
						PropagetUpdatePredecessorRoute();
					}
					else if(SelectedNode->PointIndex == Spline->GetNumberOfSplinePoints() - 1)
					{
						PropagetUpdateSuccessorRoute();
					}
					MarkAsDirty();
					break;
				}
			}
		}

		if (SelectedNode != PreSelectedNode && PreSelectedNode && PreSelectedNode->PointIndex == 0)
		{
			FitActorPosition();
			UpdateViewMesh();
		}

		// Remove node
		if(!SelectedNode && HoverNode && SceneViewport->KeyState(EKeys::RightMouseButton) && Spline->GetNumberOfSplinePoints() > 2)
		{
			Spline->RemoveSplinePoint(HoverNode->PointIndex);
			FitActorPosition();
			UpdateViewMesh();
			UpdateNodes();
			HoverNode = nullptr;
			MarkAsDirty();
		}

		// PreviewNode visibility
		if (PreviewNode->IsVisible() == bHidePreviewNode)
		{
			PreviewNode->SetHiddenInGame(bHidePreviewNode);
		}

		// GameViewportClient mode
		if (HoverNode || SelectedNode || bLeftCtrPresed)
		{
			if (GameViewportClient->GetEditorMouseCaptureMode() != EEditorMouseCaptureMode::Dragging)
			{
				GameViewportClient->SetEditorMouseCaptureMode(EEditorMouseCaptureMode::Dragging);
			}
		}
		else
		{
			if (GameViewportClient->GetEditorMouseCaptureMode() != EEditorMouseCaptureMode::Default)
			{
				GameViewportClient->SetMouseCaptureMode(EMouseCaptureMode::CaptureDuringMouseDown);
				bIsUnfreezeNotified = true;
			}
		}
	}
}

void ANavigationRouteEditable::OnSelect_Implementation(const UPrimitiveComponent* PrimComponent)
{ 
	UpdateNodes();
	bIsSelected = true;
	SelectedNode = nullptr;
	HoverNode = nullptr;
	if(MatProcMeshDyn) MatProcMeshDyn->SetVectorParameterValue(TEXT("BaseColor"), SelectColor );
}

void ANavigationRouteEditable::OnUnselect_Implementation()
{ 
	for (auto& Node : EditableNodes) Node->DestroyComponent();
	EditableNodes.Empty();
	bIsSelected = false;
	SelectedNode = nullptr;
	HoverNode = nullptr;
	if (MatProcMeshDyn) MatProcMeshDyn->SetVectorParameterValue(TEXT("BaseColor"), BaseColor);
}

void ANavigationRouteEditable::OnHover_Implementation()
{
}

void ANavigationRouteEditable::OnUnhover_Implementation()
{
}

void ANavigationRouteEditable::OnComponentHover_Implementation(const UPrimitiveComponent* PrimComponent)
{
	//HoverNode = Cast<URoutePlannerEditableNode>(const_cast<UPrimitiveComponent*>(PrimComponent));
}

void ANavigationRouteEditable::OnComponentUnhover_Implementation(const UPrimitiveComponent* PrimComponent)
{
	//HoverNode = nullptr;
}

void ANavigationRouteEditable::UpdateNodes()
{
	SelectedNode = nullptr;
	HoverNode = nullptr;

	const int Diff = Spline->GetNumberOfSplinePoints() - EditableNodes.Num();
	if (Diff < 0)
	{
		for (int i = 0; i < -Diff; ++i)
		{
			EditableNodes[EditableNodes.Num() - i - 1]->DestroyComponent();
		}
		EditableNodes.SetNum(Spline->GetNumberOfSplinePoints());
	}
	else if (Diff > 0)
	{
		for (int i = 0; i < Diff; ++i)
		{
			URoutePlannerEditableNode* Node = EditableNodes.Add_GetRef(NewObject<URoutePlannerEditableNode>(this));
			Node->RegisterComponent();
		}
	}

	for (int i = 0; i < Spline->GetNumberOfSplinePoints(); ++i)
	{
		EditableNodes[i]->SetWorldLocation(Spline->GetLocationAtSplinePoint(i, ESplineCoordinateSpace::World), false, nullptr, ETeleportType::ResetPhysics);
		EditableNodes[i]->PointIndex = i;
	}
}

bool ANavigationRouteEditable::TraceForMousePosition(const APlayerController* PlayerController, TArray<struct FHitResult>& HitResults)
{
	FVector2D ScreenPosition;
	FVector WorldOrigin;
	FVector WorldDirection;

	FCollisionObjectQueryParams ObjectQueryParams(FCollisionObjectQueryParams::AllObjects);


	const float HitResultTraceDistance = 100000.f;
	if (PlayerController->GetMousePosition(ScreenPosition.X, ScreenPosition.Y) &&
		FEditorUtils::DeprojectScreenToWorld(PlayerController->GetLocalPlayer(), ScreenPosition, WorldOrigin, WorldDirection) &&
		PlayerController->GetWorld()->LineTraceMultiByObjectType(HitResults, WorldOrigin, WorldOrigin + WorldDirection * HitResultTraceDistance, ObjectQueryParams))
	{
		return true;
	}

	return false;
}

void ANavigationRouteEditable::RuntimePostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	ISodaActor::RuntimePostEditChangeProperty(PropertyChangedEvent);

	FProperty* Property = PropertyChangedEvent.Property;
	const FName PropertyName = Property ? Property->GetFName() : NAME_None;

	if (PropertyName == GET_MEMBER_NAME_CHECKED(ANavigationRoute, PredecessorRoute))
	{
		//UpdateRouteNetwork(this, true);
		PropagetUpdatePredecessorRoute();
	}
	else if (PropertyName == GET_MEMBER_NAME_CHECKED(ANavigationRoute, SuccessorRoute))
	{
		//UpdateRouteNetwork(this, false);
		PropagetUpdateSuccessorRoute();
	}
}

void ANavigationRouteEditable::RuntimePostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent)
{
	ISodaActor::RuntimePostEditChangeProperty(PropertyChangedEvent);

	FProperty* Property = PropertyChangedEvent.Property;
	const FName PropertyName = Property ? Property->GetFName() : NAME_None;

	if (PropertyName == GET_MEMBER_NAME_CHECKED(ANavigationRoute, PredecessorRoute))
	{
		//UpdateRouteNetwork(this, true);
		PropagetUpdatePredecessorRoute();
	}
	else if (PropertyName == GET_MEMBER_NAME_CHECKED(ANavigationRoute, SuccessorRoute))
	{
		//UpdateRouteNetwork(this, false);
		PropagetUpdateSuccessorRoute();
	}
}

const FSodaActorDescriptor* ANavigationRouteEditable::GenerateActorDescriptor() const
{
	static FSodaActorDescriptor Desc{
		TEXT("Navigatopn Route"), /*DisplayName*/
		TEXT("Scenario"), /*Category*/
		TEXT(""), /*SubCategory*/
		TEXT("SodaIcons.Route"), /*Icon*/
		false, /*bAllowTransform*/
		true, /*bAllowSpawn*/
		FVector(0, 0, 50), /*SpawnOffset*/
	};
	return &Desc;
}