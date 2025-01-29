// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

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
#include "Soda/SodaSubsystem.h"
#include "Soda/SodaActorFactory.h"
#include "Soda/Editor/SodaSelection.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/Material.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/StaticMesh.h"
#include "Soda/SodaApp.h"
#include "Soda/SodaCommonSettings.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/PlatformFileManager.h"
#include "Containers/UnrealString.h"
#include "Misc/OutputDeviceDebug.h"
#include "DesktopPlatformModule.h"

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
	ProceduralMesh->SetCollisionResponseToChannel(GetDefault<USodaCommonSettings>()->SelectTraceChannel, ECollisionResponse::ECR_Block);
	ProceduralMesh->SetCollisionObjectType(GetDefault<USodaCommonSettings>()->SelectTraceChannel);
	ProceduralMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	ProceduralMesh->SetupAttachment(RootComponent);

	RandomEngine = CreateDefaultSubobject< USodaRandomEngine >(TEXT("RandomEngine"));
	RandomEngine->Seed(RandomEngine->GenerateRandomSeed());

	//PrimaryActorTick.bCanEverTick = true;
	//PrimaryActorTick.TickGroup = TG_PrePhysics;

	static ConstructorHelpers::FObjectFinder< UMaterial > Mat(TEXT("/SodaSim/Assets/CPP/RouteMat"));
	if (Mat.Succeeded()) MatProcMesh = Mat.Object;
}

bool ANavigationRoute::SetRoutePointsAdv(const TArray<FSplinePointInfo>& RoutePoints, bool bUpdateMesh)
{
	if (RoutePoints.Num() < 2) return false;

	Spline->ClearSplinePoints();

	for (int32 i = 0; i < RoutePoints.Num(); i++) // Skip the header line
	{
		Spline->AddSplinePoint(RoutePoints[i].PosnLocalSpace, ESplineCoordinateSpace::Local, false);
		const int32 PointIndex = Spline->GetNumberOfSplinePoints() - 1;
		Spline->SetTangentAtSplinePoint(PointIndex, RoutePoints[i].TangetLocalSpace, ESplineCoordinateSpace::Local, false);
		Spline->SetSplinePointType(PointIndex, RoutePoints[i].PointTyp, false);
	}

	Spline->UpdateSpline();

	if (bUpdateMesh) UpdateViewMesh();
	return true;
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
					FVector Point = Spline->GetLocationAtSplinePoint(i, ESplineCoordinateSpace::Local);
					FVector Tangent = Spline->GetTangentAtSplinePoint(i, ESplineCoordinateSpace::Local);
					int32 PointType = static_cast<int32>(Spline->GetSplinePointType(i));

					Ar << Point;
					Ar << Tangent;
					Ar << PointType;
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




			TArray<FSplinePointInfo> SplinePoints;
			SplinePoints.Reserve(PointsNum);

			if (PointsNum >= 0)
			{
				FSplinePointInfo NewPoint;
				for (int j = 0; j < PointsNum; ++j)
				{
					FVector Point;
					Ar << Point;
					FVector Tangent;
					Ar << Tangent;
					int32 PointType;
					Ar << PointType;

					NewPoint.PosnLocalSpace = Point;
					NewPoint.TangetLocalSpace = Tangent;
					NewPoint.PointTyp = static_cast<ESplinePointType::Type>(PointType);

					SplinePoints.Add(NewPoint);
				}
			}

			SetRoutePointsAdv(SplinePoints, true);


			if (ANavigationRoute* FoundActor = Cast<ANavigationRoute>(FEditorUtils::FindActorByName(LeftRoute.ToSoftObjectPath(), GetWorld())))
			{
				LeftRoute = FoundActor;
			}
			if (ANavigationRoute* FoundActor = Cast<ANavigationRoute>(FEditorUtils::FindActorByName(RightRoute.ToSoftObjectPath(), GetWorld())))
			{
				RightRoute = FoundActor;
			}
			if (ANavigationRoute* FoundActor = Cast<ANavigationRoute>(FEditorUtils::FindActorByName(PredecessorRoute.ToSoftObjectPath(), GetWorld())))
			{
				PredecessorRoute = FoundActor;
			}
			if (ANavigationRoute* FoundActor = Cast<ANavigationRoute>(FEditorUtils::FindActorByName(SuccessorRoute.ToSoftObjectPath(), GetWorld())))
			{
				SuccessorRoute = FoundActor;
			}
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
	USodaSubsystem* SodaSubsystem = USodaSubsystem::GetChecked();
	ASodaActorFactory * ActorFactory = SodaSubsystem->GetActorFactory();
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

bool ANavigationRoute::GroundHitFilter(const FHitResult& Hit)
{
	return
		Hit.GetActor() != this &&
		!Hit.GetActor()->IsRootComponentMovable() &&
		(Hit.GetComponent()->GetCollisionObjectType() == ECollisionChannel::ECC_WorldStatic || Hit.GetComponent()->GetCollisionObjectType() == ECollisionChannel::ECC_WorldDynamic);
}

void ANavigationRoute::PullToGround()
{
	FVector2D ScreenPosition;
	FVector WorldOrigin;
	FVector WorldDirection;

	APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
	check(PlayerController);
	FHitResult Hit;

	for (int i = 0; i < Spline->GetNumberOfSplinePoints(); ++i)
	{
		const FVector Point = Spline->GetLocationAtSplinePoint(i, ESplineCoordinateSpace::World);
		if(PlayerController->GetWorld()->LineTraceSingleByChannel(
			Hit,
			Point - FVector(0, 0, 100), Point + FVector(0, 0, 1000),
			ECollisionChannel::ECC_GameTraceChannel2)) // ECC_GameTraceChannel2 -  camera (any collision) // ECollisionChannel::ECC_WorldDynamic;
		{
			if (GroundHitFilter(Hit))
			{
				Spline->SetLocationAtSplinePoint(i, Hit.Location + FVector(0, 0, ZOffset), ESplineCoordinateSpace::World, false);
			}
		}
	}

	Spline->UpdateSpline();
	UpdateViewMesh();
}

/*****************************************************************************************
								URoutePlannerEditableNode
*****************************************************************************************/

URoutePlannerEditableNode::URoutePlannerEditableNode(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	static ConstructorHelpers::FObjectFinder< UStaticMesh > MeshAsset(TEXT("/Engine/BasicShapes/Cylinder"));
	static ConstructorHelpers::FObjectFinder< UMaterial > MatAsset(TEXT("/SodaSim/Assets/CPP/RouteNodeMat"));
	if (MeshAsset.Succeeded()) 	SetStaticMesh(MeshAsset.Object);
	if (MatAsset.Succeeded()) 	SetMaterial(0, MatAsset.Object);
	PrimaryComponentTick.bCanEverTick = true;

	//SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	//SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Overlap);
	//SetCollisionEnabled(ECollisionEnabled::QueryOnly);

	SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	SetCollisionResponseToChannel(GetDefault<USodaCommonSettings>()->SelectTraceChannel, ECollisionResponse::ECR_Block);
	SetCollisionObjectType(GetDefault<USodaCommonSettings>()->SelectTraceChannel);
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

		TArray<FHitResult> HitActors; 
		TraceForMousePositionByObject(PlayerController, HitActors, GetDefault<USodaCommonSettings>()->SelectTraceChannel);

		HoverNode = nullptr;		

		if (!SelectedNode && !PreviewNode->IsVisible() && !bLeftCtrPresed)
		{
			// Find Hover Node

			for (auto& Hit : HitActors)
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

			for (auto& Hit : HitActors)
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
			TArray<FHitResult> HitGround;
			if (TraceForMousePositionByChanel(PlayerController, HitGround, ECollisionChannel::ECC_WorldDynamic)) // && GroundHitFilter(Hit))
			{
				for (auto& Hit : HitGround)
				{
					if (!Cast<ISodaActor>(Hit.GetActor()))
					{
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
						else if (SelectedNode->PointIndex == Spline->GetNumberOfSplinePoints() - 1)
						{
							PropagetUpdateSuccessorRoute();
						}
						MarkAsDirty();
						break;
					}
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

bool ANavigationRouteEditable::TraceForMousePositionByChanel(const APlayerController* PlayerController, TArray<FHitResult>& HitResults, const TEnumAsByte<ECollisionChannel> & CollisionChannel)
{
	FVector2D ScreenPosition;
	FVector WorldOrigin;
	FVector WorldDirection;

	const float HitResultTraceDistance = 100000.f;
	if (PlayerController->GetMousePosition(ScreenPosition.X, ScreenPosition.Y) &&
		FEditorUtils::DeprojectScreenToWorld(PlayerController->GetLocalPlayer(), ScreenPosition, WorldOrigin, WorldDirection) &&
		PlayerController->GetWorld()->LineTraceMultiByChannel(
			HitResults,
			WorldOrigin, WorldOrigin + WorldDirection * HitResultTraceDistance, CollisionChannel))
			//ECollisionChannel::ECC_GameTraceChannel2)) // ECC_GameTraceChannel2 -  camera (any collision) // ECollisionChannel::ECC_WorldDynamic;
	{
		return true;
	}

	return false;
}

bool ANavigationRouteEditable::TraceForMousePositionByObject(const APlayerController* PlayerController, TArray<FHitResult>& HitResults, const TEnumAsByte<ECollisionChannel>& CollisionChannel)
{
	FVector2D ScreenPosition;
	FVector WorldOrigin;
	FVector WorldDirection;

	const float HitResultTraceDistance = 100000.f;
	if (PlayerController->GetMousePosition(ScreenPosition.X, ScreenPosition.Y) &&
		FEditorUtils::DeprojectScreenToWorld(PlayerController->GetLocalPlayer(), ScreenPosition, WorldOrigin, WorldDirection) &&
		PlayerController->GetWorld()->LineTraceMultiByObjectType(
			HitResults,
			WorldOrigin, WorldOrigin + WorldDirection * HitResultTraceDistance, FCollisionObjectQueryParams(CollisionChannel)))
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
	else if (PropertyName == GET_MEMBER_NAME_CHECKED(ANavigationRoute, ZOffset))
	{
		PullToGround();
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
	else if (PropertyName == GET_MEMBER_NAME_CHECKED(ANavigationRoute, ZOffset))
	{
		PullToGround();
	}
}

void ANavigationRouteEditable::PullToGround()
{
	Super::PullToGround();
	UpdateNodes();
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




void ANavigationRouteEditable::SaveNavigationRouteToFile()
{
	// Open a dialog to let the user choose the directory
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	if (DesktopPlatform)
	{
		FString SelectedFolder;
		const void* ParentWindowHandle = nullptr; // Can set to a valid window handle if needed

		// Open the directory picker
		bool bFolderSelected = DesktopPlatform->OpenDirectoryDialog(
			ParentWindowHandle,
			TEXT("Select Folder to Save CSV"),
			FPaths::ProjectDir(),
			SelectedFolder
		);

		if (bFolderSelected)
		{
			// Define the CSV filename and full path
			FString FilePath = SelectedFolder / TEXT("SplineInfo.csv");

			// Define the header row and an example data row
			FString CSVContent = TEXT("idx,loc1,loc2,loc3,tan1,tan2,tan3,typ,trans1,trans2,trans3,trans4,trans5,trans6\n");



			const int32 NumPoints = Spline->GetNumberOfSplinePoints();
			for (int32 i = 0; i < NumPoints; i++)
			{
				const FVector Location = Spline->GetLocationAtSplinePoint(i, ESplineCoordinateSpace::Local);
				const FVector Tangent = Spline->GetTangentAtSplinePoint(i, ESplineCoordinateSpace::Local);
				const ESplinePointType::Type Type = Spline->GetSplinePointType(i);

				CSVContent += FString::Printf(TEXT("%d,%f,%f,%f,%f,%f,%f,%d,%f,%f,%f,%f,%f,%f\n"),
					i, Location.X, Location.Y, Location.Z, Tangent.X, Tangent.Y, Tangent.Z, static_cast<int32>(Type),
					GetActorLocation().X, GetActorLocation().Y, GetActorLocation().Z,
					GetActorRotation().Pitch, GetActorRotation().Yaw, GetActorRotation().Roll);
			}



			// Save the CSV content to a file
			if (FFileHelper::SaveStringToFile(CSVContent, *FilePath))
			{
				FNotificationInfo Info(FText::FromString(FString("CSV template was created at: ") + FilePath));
				Info.ExpireDuration = 5.0f;
				Info.Image = FCoreStyle::Get().GetBrush(TEXT("Icons.SuccessWithColor"));
				FSlateNotificationManager::Get().AddNotification(Info);
			}
			else
			{
				FNotificationInfo Info(FText::FromString(FString("CSV template creation FAILED")));
				Info.ExpireDuration = 5.0f;
				Info.Image = FCoreStyle::Get().GetBrush(TEXT("Icons.ErrorWithColor"));
				FSlateNotificationManager::Get().AddNotification(Info);
			}
		}
		else
		{
			FNotificationInfo Info(FText::FromString(FString("CSV template creation: no folder selected")));
			Info.ExpireDuration = 5.0f;
			Info.Image = FCoreStyle::Get().GetBrush(TEXT("Icons.WarningWithColor"));
			FSlateNotificationManager::Get().AddNotification(Info);

		}
	}
}


void ANavigationRouteEditable::RecreateNavigationRouteFromFile()
{
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	if (!DesktopPlatform)
	{
		UE_LOG(LogSoda, Error, TEXT("UVehicleInputOpenLoopComponent::LoadCustomInput() Can't get the IDesktopPlatform ref"));
		return;
	}

	const FString FileTypes = TEXT("(*.csv)|*.csv");

	TArray<FString> OpenFilenames;
	int32 FilterIndex = -1;
	if (!DesktopPlatform->OpenFileDialog(nullptr, TEXT("Import custom input from CSV"), TEXT(""), TEXT(""), FileTypes, EFileDialogFlags::None, OpenFilenames, FilterIndex) || OpenFilenames.Num() <= 0)
	{
		FNotificationInfo Info(FText::FromString(FString("Load input Error: can't open the file")));
		Info.ExpireDuration = 5.0f;
		Info.Image = FCoreStyle::Get().GetBrush(TEXT("Icons.ErrorWithColor"));
		FSlateNotificationManager::Get().AddNotification(Info);
		return;
	}

	FString& FilePath = OpenFilenames[0];

	if (!FPlatformFileManager::Get().GetPlatformFile().FileExists(*FilePath))
	{
		UE_LOG(LogTemp, Error, TEXT("File does not exist: %s"), *FilePath);
		return;
	}


	TArray<FString> FileLines;
	if (!FFileHelper::LoadFileToStringArray(FileLines, *FilePath))
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to load file: %s"), *FilePath);
		return;
	}


	if (FileLines.Num() < 2)
	{
		FNotificationInfo Info(FText::FromString(FString("Load input Error: file has no data") + FilePath));
		Info.ExpireDuration = 5.0f;
		Info.Image = FCoreStyle::Get().GetBrush(TEXT("Icons.ErrorWithColor"));
		FSlateNotificationManager::Get().AddNotification(Info);
		return;
	}

	Spline->ClearSplinePoints();


	bool bErrorWasShown = false;



	for (int32 i = 1; i < FileLines.Num(); i++) // Skip the header line
	{
		TArray<FString> Values;
		FileLines[i].ParseIntoArray(Values, TEXT(","), true);

		if (Values.Num() < 8)
		{
			if (!bErrorWasShown)
			{
				FNotificationInfo Info(FText::FromString(FString("Load input Error: some of the lines are invalid") + FilePath));
				Info.ExpireDuration = 5.0f;
				Info.Image = FCoreStyle::Get().GetBrush(TEXT("Icons.ErrorWithColor"));
				FSlateNotificationManager::Get().AddNotification(Info);
				bErrorWasShown = true;
			}

			UE_LOG(LogTemp, Error, TEXT("Invalid data at line %d"), i);
			continue;
		}

		const FVector Location(FCString::Atof(*Values[1]), FCString::Atof(*Values[2]), FCString::Atof(*Values[3]));
		const FVector Tangent(FCString::Atof(*Values[4]), FCString::Atof(*Values[5]), FCString::Atof(*Values[6]));
		const ESplinePointType::Type Type = static_cast<ESplinePointType::Type>(FCString::Atoi(*Values[7]));

		Spline->AddSplinePoint(Location, ESplineCoordinateSpace::Local, false);
		const int32 PointIndex = Spline->GetNumberOfSplinePoints() - 1;
		Spline->SetTangentAtSplinePoint(PointIndex, Tangent, ESplineCoordinateSpace::Local);
		Spline->SetSplinePointType(PointIndex, Type, false);


		const FVector ActorLocations(FCString::Atof(*Values[8]), FCString::Atof(*Values[9]), FCString::Atof(*Values[10]));
		const FVector ActorRotations(FCString::Atof(*Values[11]), FCString::Atof(*Values[12]), FCString::Atof(*Values[13]));

		FRotator Rotn;

		Rotn.Pitch = ActorRotations.X;
		Rotn.Yaw = ActorRotations.Y;
		Rotn.Roll = ActorRotations.Z;

		SetActorLocation(ActorLocations);
		SetActorRotation(Rotn);

	}

	Spline->UpdateSpline();


	UpdateViewMesh();

	if (bErrorWasShown)
	{
		FNotificationInfo Info(FText::FromString(FString("Information about spline points was incomplete, check input file") + FilePath));
		Info.ExpireDuration = 5.0f;
		Info.Image = FCoreStyle::Get().GetBrush(TEXT("Icons.ErrorWithColor"));
		FSlateNotificationManager::Get().AddNotification(Info);
	}
	else
	{
		FNotificationInfo Info(FText::FromString(FString("Load input: success!")));
		Info.ExpireDuration = 5.0f;
		Info.Image = FCoreStyle::Get().GetBrush(TEXT("Icons.SuccessWithColor"));
		FSlateNotificationManager::Get().AddNotification(Info);
	}



}
