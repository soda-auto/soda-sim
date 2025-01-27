// Copyright Epic Games, Inc. All Rights Reserved.
// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "SodaWidget.h"
#include "Soda/Misc/EditorUtils.h"
#include "CanvasItem.h"
#include "CanvasTypes.h"
#include "DynamicMeshBuilder.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "KismetProceduralMeshLibrary.h"
#include "Soda/SodaGameViewportClient.h"
#include "Soda/SodaApp.h"
#include "Soda/SodaCommonSettings.h"

#include "MeshUtils.hpp"

constexpr float ASodaWidget::AXIS_LENGTH;
constexpr float ASodaWidget::TRANSLATE_ROTATE_AXIS_CIRCLE_RADIUS;
constexpr float ASodaWidget::TWOD_AXIS_CIRCLE_RADIUS;
constexpr float ASodaWidget::INNER_AXIS_CIRCLE_RADIUS;
constexpr float ASodaWidget::OUTER_AXIS_CIRCLE_RADIUS;
constexpr float ASodaWidget::ROTATION_TEXT_RADIUS;
constexpr int32 ASodaWidget::AXIS_CIRCLE_SIDES;
constexpr float ASodaWidget::ARCALL_RELATIVE_INNER_SIZE;
constexpr float ASodaWidget::AXIS_LENGTH_SCALE_OFFSET;
constexpr uint8 ASodaWidget::LargeInnerAlpha;
constexpr uint8 ASodaWidget::SmallInnerAlpha;
constexpr uint8 ASodaWidget::LargeOuterAlpha;
constexpr uint8 ASodaWidget::SmallOuterAlpha;

static void SetCapsuleSize(UCapsuleComponent* CapsuleComponent, const FVector& Begin, const FVector& End, float Radius)
{
	const float Height = (Begin - End).Size();
	const FVector Center = (Begin + End) / 2;
	const FQuat Quat = (End - Begin).ToOrientationQuat() * FQuat(FRotator(90, 0, 0));
	CapsuleComponent->SetCapsuleSize(Radius, Height / 2, true);
	CapsuleComponent->SetRelativeTransform(FTransform(Quat, Center));
}

static UAxisCapsuleComponent* CreateCapsuleHelper(UObject* Parent, USceneComponent* ParentAttachment, const FVector& Begin, const FVector& End, float Radius, EAxisList::Type AxisList)
{
	UAxisCapsuleComponent* Capsule = NewObject<UAxisCapsuleComponent>(Parent);
	check(Capsule);
	if (ParentAttachment)Capsule->SetupAttachment(ParentAttachment);
	Capsule->RegisterComponent();
	Capsule->SetHiddenInGame(true);
	Capsule->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	Capsule->SetCollisionResponseToChannel(GetDefault<USodaCommonSettings>()->SelectTraceChannel, ECollisionResponse::ECR_Block);
	Capsule->SetCollisionObjectType(GetDefault<USodaCommonSettings>()->SelectTraceChannel);
	Capsule->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	SetCapsuleSize(Capsule, Begin, End, Radius);
	Capsule->AxisList = AxisList;
	return Capsule;
}

static UAxisBoxComponent* CreateBoxHelper(UObject* Parent, USceneComponent* ParentAttachment, const FVector& Orign, float Size, EAxisList::Type AxisList)
{
	UAxisBoxComponent* Box = NewObject<UAxisBoxComponent>(Parent);
	check(Box);
	if (ParentAttachment) Box->SetupAttachment(ParentAttachment);
	Box->RegisterComponent();
	Box->SetHiddenInGame(true);
	Box->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	Box->SetCollisionResponseToChannel(GetDefault<USodaCommonSettings>()->SelectTraceChannel, ECollisionResponse::ECR_Block);
	Box->SetCollisionObjectType(GetDefault<USodaCommonSettings>()->SelectTraceChannel);
	Box->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Box->SetBoxExtent(FVector(Size));
	Box->SetRelativeLocation(Orign);
	Box->AxisList = AxisList;
	return Box;
}

static UAxisSphereComponent* CreateSphereHelper(UObject* Parent, USceneComponent* ParentAttachment, const FVector& Orign, float Radius, EAxisList::Type AxisList)
{
	UAxisSphereComponent* Sphere = NewObject<UAxisSphereComponent>(Parent);
	check(Sphere);
	if (ParentAttachment) Sphere->SetupAttachment(ParentAttachment);
	Sphere->RegisterComponent();
	Sphere->SetHiddenInGame(true);
	Sphere->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	Sphere->SetCollisionResponseToChannel(GetDefault<USodaCommonSettings>()->SelectTraceChannel, ECollisionResponse::ECR_Block);
	Sphere->SetCollisionObjectType(GetDefault<USodaCommonSettings>()->SelectTraceChannel);
	Sphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Sphere->SetSphereRadius(Radius);
	Sphere->SetRelativeLocation(Orign);
	Sphere->AxisList = AxisList;
	return Sphere;
}

static UAxisProceduralMeshComponent* CreateArcHelper(UObject* Parent, USceneComponent* ParentAttachment, const FVector& Axis0, const FVector& Axis1, const FMatrix& CoordSystem, const FVector& InDirectionToWidget, float InEndAngle, float InStartAngle, float InnerRadius, float OuterRadius, EAxisList::Type AxisList)
{
	UAxisProceduralMeshComponent* ProceduralMesh = NewObject<UAxisProceduralMeshComponent>(Parent);
	check(ProceduralMesh);
	if (ParentAttachment) ProceduralMesh->SetupAttachment(ParentAttachment);
	ProceduralMesh->RegisterComponent();
	ProceduralMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	ProceduralMesh->SetCollisionResponseToChannel(GetDefault<USodaCommonSettings>()->SelectTraceChannel, ECollisionResponse::ECR_Block);
	ProceduralMesh->SetCollisionObjectType(GetDefault<USodaCommonSettings>()->SelectTraceChannel);
	ProceduralMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	ProceduralMesh->SetHiddenInGame(true);
	ProceduralMesh->CreateArc(Axis0, Axis1, CoordSystem, InDirectionToWidget, InEndAngle, InStartAngle, InnerRadius, OuterRadius);
	ProceduralMesh->AxisList = AxisList;
	return ProceduralMesh;
}


void UAxisProceduralMeshComponent::CreateArc(const FVector& Axis0, const FVector& Axis1, const FMatrix& CoordSystem, const FVector& InDirectionToWidget, float InEndAngle, float InStartAngle, float InnerRadius, float OuterRadius)
{
	StoreAxis0 = Axis0;
	StoreAxis1 = Axis1;
	StoreEndAngle = InEndAngle;
	StoreStartAngle = InStartAngle;
	StoreInnerRadius = InnerRadius;
	StoreOuterRadius = OuterRadius;

	const int32 CircleSides = 24;// AXIS_CIRCLE_SIDES;
	const int32 NumPoints = FMath::TruncToInt(CircleSides * (InEndAngle - InStartAngle) / (PI / 2)) + 1;

	TArray<FVector> Vertices;
	TArray<int32> Triangles;
	TArray<FVector> Normals;
	TArray<FVector2D> UVs;
	TArray<FColor> VertexColors;
	TArray<FProcMeshTangent> Tangents;


	bool bMirrorAxis0 = ((FVector(CoordSystem.TransformVector(Axis0)) | InDirectionToWidget) <= 0.0f);
	bool bMirrorAxis1 = ((FVector(CoordSystem.TransformVector(Axis1)) | InDirectionToWidget) <= 0.0f);

	FVector RenderAxis0 = bMirrorAxis0 ? Axis0 : -Axis0;
	FVector RenderAxis1 = bMirrorAxis1 ? Axis1 : -Axis1;
	FVector ZAxis = RenderAxis0 ^ RenderAxis1;

	bool bMirrorAxis2 = ((ZAxis | InDirectionToWidget) <= 0.0f);

	//UE_LOG(LogTemp, Error, TEXT("*** %i %i %i"), bMirrorAxis0, bMirrorAxis1, bMirrorAxis2);

	for (int32 RadiusIndex = 0; RadiusIndex < 2; ++RadiusIndex)
	{
		float Radius = (RadiusIndex == 0) ? OuterRadius : InnerRadius;
		float TCRadius = Radius / (float)OuterRadius;
		//Compute vertices for base circle.
		for (int32 VertexIndex = 0; VertexIndex <= NumPoints; VertexIndex++)
		{
			float Percent = VertexIndex / (float)NumPoints;
			float Angle = FMath::Lerp(InStartAngle, InEndAngle, Percent);
			float AngleDeg = FRotator::ClampAxis(Angle * 180.f / PI);

			FVector VertexDir = RenderAxis0.RotateAngleAxis(AngleDeg, ZAxis);
			VertexDir.Normalize();

			float TCAngle = Percent * (PI / 2);
			FVector2D TC(TCRadius * FMath::Cos(Angle), TCRadius * FMath::Sin(Angle));

			// Keep the vertices in local space so that we don't lose precision when dealing with LWC
			// The local-to-world transform is handled in the MeshBuilder.Draw() call at the end of this function
			const FVector VertexPosition = VertexDir * Radius;
			FVector Normal = VertexPosition;
			Normal.Normalize();

			Vertices.Add(VertexPosition);
			UVs.Add(TC);
			Normals.Add(Normal);
		}
	}

	//Add top/bottom triangles, in the style of a fan.
	int32 InnerVertexStartIndex = NumPoints + 1;
	for (int32 VertexIndex = 0; VertexIndex < NumPoints; VertexIndex++)
	{
		bool f = bMirrorAxis0 ^ bMirrorAxis1;
		if ((f && !bMirrorAxis2) || (!f && !bMirrorAxis2))
		{

			Triangles.Add(VertexIndex);
			Triangles.Add(VertexIndex + 1);
			Triangles.Add(InnerVertexStartIndex + VertexIndex);

			Triangles.Add(VertexIndex + 1);
			Triangles.Add(InnerVertexStartIndex + VertexIndex + 1);
			Triangles.Add(InnerVertexStartIndex + VertexIndex);
		}
		else
		{
			Triangles.Add(InnerVertexStartIndex + VertexIndex);
			Triangles.Add(VertexIndex + 1);
			Triangles.Add(VertexIndex);

			Triangles.Add(InnerVertexStartIndex + VertexIndex);
			Triangles.Add(InnerVertexStartIndex + VertexIndex + 1);
			Triangles.Add(VertexIndex + 1);
		}
	}

	UKismetProceduralMeshLibrary::CalculateTangentsForMesh(Vertices, Triangles, UVs, Normals, Tangents);

	ClearAllMeshSections();
	CreateMeshSection(0, Vertices, Triangles, Normals, UVs, VertexColors, Tangents, true);
}


ASodaWidget::ASodaWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	//EditorModeTools      = NULL;
	TotalDeltaRotation   = 0;
	CurrentDeltaRotation = 0;

	AxisColorX       = FLinearColor(0.594f, 0.0197f, 0.0f);
	AxisColorY       = FLinearColor(0.1349f, 0.3959f, 0.0f);
	AxisColorZ       = FLinearColor(0.0251f, 0.207f, 0.85f);
	ScreenAxisColor  = FLinearColor(0.76, 0.72, 0.14f);
	PlaneColorXY     = FColor::Yellow;
	ArcBallColor     = FColor(128, 128, 128, 6);
	ScreenSpaceColor = FColor(196, 196, 196);
	CurrentColor     = FColor::Yellow;

	CurrentAxis = EAxisList::None;

	bAbsoluteTranslationInitialOffsetCached = false;
	InitialTranslationOffset                = FVector::ZeroVector;
	InitialTranslationPosition              = FVector(0, 0, 0);

	bDragging               = false;
	bSnapEnabled            = false;
	bIsOrthoDrawingFullRing = false;

	XAxisDir     = FVector2D::ZeroVector;
	YAxisDir     = FVector2D::ZeroVector;
	ZAxisDir     = FVector2D::ZeroVector;

	DragStartPos = FVector2D::ZeroVector;
	LastDragPos  = FVector2D::ZeroVector;

	AxisMaterialBase = (UMaterial*)StaticLoadObject(
		UMaterial::StaticClass(), NULL,
		TEXT("/SodaSim/Assets/CPP/EditorMaterials/GizmoMaterial.GizmoMaterial"), NULL, LOAD_None, NULL);
	check(AxisMaterialBase);

	TransparentPlaneMaterialXY = (UMaterial*)StaticLoadObject(
		UMaterial::StaticClass(), NULL,
		TEXT("/SodaSim/Assets/CPP/EditorMaterials/WidgetVertexColorMaterial.WidgetVertexColorMaterial"), NULL, LOAD_None, NULL);
	check(TransparentPlaneMaterialXY);

	GridMaterial = (UMaterial*)StaticLoadObject(
		UMaterial::StaticClass(), NULL,
		TEXT("/SodaSim/Assets/CPP/EditorMaterials/WidgetGridVertexColorMaterial_Ma.WidgetGridVertexColorMaterial_Ma"), NULL,
		LOAD_None, NULL);
	check(GridMaterial);
}

EAxisList::Type ASodaWidget::GetAxisListAtScreenPos(UWorld* World, const FVector2D& ScreenPos)
{
	check(World);

	FVector Origin;
	FVector Direction;
	if (FEditorUtils::DeprojectScreenToWorld(World, ScreenPos, Origin, Direction))
	{
		TArray<FHitResult> Hits;
		FCollisionQueryParams Param(SCENE_QUERY_STAT(WidgetTrace), true);
		const FVector RayEnd = Origin + Direction * HALF_WORLD_MAX;
		if (World->LineTraceMultiByObjectType(Hits, Origin, RayEnd, FCollisionObjectQueryParams(GetDefault<USodaCommonSettings>()->SelectTraceChannel), Param))
		{
			float ClosestHitDistanceSqr = TNumericLimits<float>::Max();
			IAxisInterface* ClosestAxis = nullptr;
			for (const FHitResult& Hit : Hits)
			{
				if (IAxisInterface* Axis = Cast<IAxisInterface>(Hit.Component.Get()))
				{
					const float DistanceToHitSqr = (Hit.ImpactPoint - Origin).SizeSquared();
					if (DistanceToHitSqr < ClosestHitDistanceSqr)
					{
						ClosestHitDistanceSqr = DistanceToHitSqr;
						ClosestAxis = Axis;
					}
				}
			}

			if (ClosestAxis)
			{
				return ClosestAxis->AxisList;
			}
		}
	}

	return EAxisList::None;
}


void ASodaWidget::TickWidget(USodaGameViewportClient* ViewportClient)
{
	check(ViewportClient);

	if (!RenderData.bUpdated) return;

	if (ViewportClient->GetWidgetMode() != RenderData.WidgetMode)
	{
		RenderData.WidgetMode = ViewportClient->GetWidgetMode();

		for (auto It : WidgetAxisComponents) It->DestroyComponent();
		WidgetAxisComponents.Empty();

		switch (RenderData.WidgetMode)
		{
		case soda::EWidgetMode::WM_Translate:
		{
			UAxisSphereComponent* RootAxisComponent = CreateSphereHelper(this, nullptr, FVector(0), 5, EAxisList::Screen);
			SetRootComponent(RootAxisComponent);
			WidgetAxisComponents.Add(RootAxisComponent);

			WidgetAxisComponents.Add(CreateCapsuleHelper(this, RootAxisComponent, FVector(0), FVector(AXIS_LENGTH + 15, 0, 0), 3, EAxisList::X)); // X
			WidgetAxisComponents.Add(CreateCapsuleHelper(this, RootAxisComponent, FVector(0), FVector(0, AXIS_LENGTH + 15, 0), 3, EAxisList::Y)); // Y
			WidgetAxisComponents.Add(CreateCapsuleHelper(this, RootAxisComponent, FVector(0), FVector(0, 0, AXIS_LENGTH + 15), 3, EAxisList::Z)); // Z

			const float CL = 13;

			WidgetAxisComponents.Add(CreateCapsuleHelper(this, RootAxisComponent, FVector(CL, 0, 0), FVector(CL, CL, 0), 2, EAxisList::XY)); // XY_X
			WidgetAxisComponents.Add(CreateCapsuleHelper(this, RootAxisComponent, FVector(0, CL, 0), FVector(CL, CL, 0), 2, EAxisList::XY)); // XY_Y

			WidgetAxisComponents.Add(CreateCapsuleHelper(this, RootAxisComponent, FVector(CL, 0, 0), FVector(CL, 0, CL), 2, EAxisList::XZ)); // XZ_X
			WidgetAxisComponents.Add(CreateCapsuleHelper(this, RootAxisComponent, FVector(0, 0, CL), FVector(CL, 0, CL), 2, EAxisList::XZ)); // XZ_Z

			WidgetAxisComponents.Add(CreateCapsuleHelper(this, RootAxisComponent, FVector(0, CL, 0), FVector(0, CL, CL), 2, EAxisList::YZ)); // YZ_Y
			WidgetAxisComponents.Add(CreateCapsuleHelper(this, RootAxisComponent, FVector(0, 0, CL), FVector(0, CL, CL), 2, EAxisList::YZ)); // YZ_Z

			break;
		}

		case soda::EWidgetMode::WM_Rotate:
		{
			USceneComponent* RootAxisComponent = NewObject<USceneComponent>(this);
			SetRootComponent(RootAxisComponent);

			FVector XAxis = FVector(1, 0, 0);
			FVector YAxis = FVector(0, 1, 0);
			FVector ZAxis = FVector(0, 0, 1);

			WidgetAxisComponents.Add(CreateArcHelper(this, RootAxisComponent, ZAxis, YAxis, ViewportClient->GetWidgetCoordSystem(), RenderData.DirectionToWidget, PI / 2, 0, INNER_AXIS_CIRCLE_RADIUS, OUTER_AXIS_CIRCLE_RADIUS, EAxisList::X));
			WidgetAxisComponents.Add(CreateArcHelper(this, RootAxisComponent, XAxis, ZAxis, ViewportClient->GetWidgetCoordSystem(), RenderData.DirectionToWidget, PI / 2, 0, INNER_AXIS_CIRCLE_RADIUS, OUTER_AXIS_CIRCLE_RADIUS, EAxisList::Y));
			WidgetAxisComponents.Add(CreateArcHelper(this, RootAxisComponent, XAxis, YAxis, ViewportClient->GetWidgetCoordSystem(), RenderData.DirectionToWidget, PI / 2, 0, INNER_AXIS_CIRCLE_RADIUS, OUTER_AXIS_CIRCLE_RADIUS, EAxisList::Z));

			break;
		}

		case soda::EWidgetMode::WM_Scale:
		{

			UAxisBoxComponent* RootAxisComponent = CreateBoxHelper(this, nullptr, FVector(0), 5, EAxisList::XYZ);
			SetRootComponent(RootAxisComponent);
			WidgetAxisComponents.Add(RootAxisComponent);

			WidgetAxisComponents.Add(CreateCapsuleHelper(this, RootAxisComponent, FVector(0), FVector(AXIS_LENGTH, 0, 0), 3, EAxisList::X)); // X
			WidgetAxisComponents.Add(CreateCapsuleHelper(this, RootAxisComponent, FVector(0), FVector(0, AXIS_LENGTH, 0), 3, EAxisList::Y)); // Y
			WidgetAxisComponents.Add(CreateCapsuleHelper(this, RootAxisComponent, FVector(0), FVector(0, 0, AXIS_LENGTH), 3, EAxisList::Z)); // Z

			WidgetAxisComponents.Add(CreateBoxHelper(this, RootAxisComponent, FVector(AXIS_LENGTH - 2, 0, 0), 5, EAxisList::X)); // X
			WidgetAxisComponents.Add(CreateBoxHelper(this, RootAxisComponent, FVector(0, AXIS_LENGTH - 2, 0), 5, EAxisList::Y)); // Y
			WidgetAxisComponents.Add(CreateBoxHelper(this, RootAxisComponent, FVector(0, 0, AXIS_LENGTH - 2), 5, EAxisList::Z)); // Z

			const float CL = 24;
			WidgetAxisComponents.Add(CreateCapsuleHelper(this, RootAxisComponent, FVector(CL, 0, 0), FVector(0, CL, 0), 2, EAxisList::XY)); // XY
			WidgetAxisComponents.Add(CreateCapsuleHelper(this, RootAxisComponent, FVector(CL, 0, 0), FVector(0, 0, CL), 2, EAxisList::XZ)); // XZ
			WidgetAxisComponents.Add(CreateCapsuleHelper(this, RootAxisComponent, FVector(0, CL, 0), FVector(0, 0, CL), 2, EAxisList::YZ)); // YZ

			break;

		}

		}

	}
	else
	{
		if (ViewportClient->GetWidgetMode() == soda::EWidgetMode::WM_Rotate)
		{
			const FVector XAxis = FVector(1, 0, 0);
			const FVector YAxis = FVector(0, 1, 0);
			const FVector ZAxis = FVector(0, 0, 1);

			const FQuat4d Quat = ViewportClient->GetWidgetCoordSystem().ToQuat();
			const FVector XAxisRotated = Quat.GetForwardVector();
			const FVector YAxisRotated = Quat.GetRightVector();
			const FVector ZAxisRotated = Quat.GetUpVector();

			const bool bMirrorAxis0 = ((XAxisRotated | RenderData.DirectionToWidget) <= 0.0f);
			const bool bMirrorAxis1 = ((YAxisRotated | RenderData.DirectionToWidget) <= 0.0f);
			const bool bMirrorAxis2 = ((ZAxisRotated | RenderData.DirectionToWidget) <= 0.0f);

			if (bMirrorAxis0 != bPrevMirrorAxis0 || bMirrorAxis1 != bPrevMirrorAxis1 || bMirrorAxis2 != bPrevMirrorAxis2)
			{
				for (auto It : WidgetAxisComponents) It->DestroyComponent();
				WidgetAxisComponents.Empty();

				USceneComponent* RootAxisComponent = NewObject<USceneComponent>(this);
				SetRootComponent(RootAxisComponent);

				WidgetAxisComponents.Add(CreateArcHelper(this, RootAxisComponent, ZAxis, YAxis, ViewportClient->GetWidgetCoordSystem(), RenderData.DirectionToWidget, PI / 2, 0, INNER_AXIS_CIRCLE_RADIUS, OUTER_AXIS_CIRCLE_RADIUS, EAxisList::X));
				WidgetAxisComponents.Add(CreateArcHelper(this, RootAxisComponent, XAxis, ZAxis, ViewportClient->GetWidgetCoordSystem(), RenderData.DirectionToWidget, PI / 2, 0, INNER_AXIS_CIRCLE_RADIUS, OUTER_AXIS_CIRCLE_RADIUS, EAxisList::Y));
				WidgetAxisComponents.Add(CreateArcHelper(this, RootAxisComponent, XAxis, YAxis, ViewportClient->GetWidgetCoordSystem(), RenderData.DirectionToWidget, PI / 2, 0, INNER_AXIS_CIRCLE_RADIUS, OUTER_AXIS_CIRCLE_RADIUS, EAxisList::Z));

				bPrevMirrorAxis0 = bMirrorAxis0;
				bPrevMirrorAxis1 = bMirrorAxis1;
				bPrevMirrorAxis2 = bMirrorAxis2;
			}
		}
	}

	SetActorTransform(FTransform(RenderData.CoordSystem.ToQuat(), RenderData.WidgetLocation, FVector(RenderData.UniformScale)));
}

void ASodaWidget::BeginPlay()
{
	Super::BeginPlay();


	AxisMaterialX = UMaterialInstanceDynamic::Create(AxisMaterialBase, NULL);
	AxisMaterialX->SetVectorParameterValue("GizmoColor", AxisColorX);

	AxisMaterialY = UMaterialInstanceDynamic::Create(AxisMaterialBase, NULL);
	AxisMaterialY->SetVectorParameterValue("GizmoColor", AxisColorY);

	AxisMaterialZ = UMaterialInstanceDynamic::Create(AxisMaterialBase, NULL);
	AxisMaterialZ->SetVectorParameterValue("GizmoColor", AxisColorZ);

	CurrentAxisMaterial = UMaterialInstanceDynamic::Create(AxisMaterialBase, NULL);
	CurrentAxisMaterial->SetVectorParameterValue("GizmoColor", CurrentColor);

	OpaquePlaneMaterialXY = UMaterialInstanceDynamic::Create(AxisMaterialBase, NULL);
	OpaquePlaneMaterialXY->SetVectorParameterValue("GizmoColor", FLinearColor::White);
}

void ASodaWidget::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
}


/*
void ASodaWidget::SetUsesEditorModeTools(FEditorModeTools* InEditorModeTools)
{
	EditorModeTools = InEditorModeTools;
}
*/

void ASodaWidget::ConvertMouseToAxis_Translate(FVector2D DragDir, FVector& InOutDelta, FVector& OutDrag) const
{
	// Get drag delta in widget axis space
	OutDrag = FVector((CurrentAxis & EAxisList::X) ? FVector2D::DotProduct(XAxisDir, DragDir) : 0.0f,
	                  (CurrentAxis & EAxisList::Y) ? FVector2D::DotProduct(YAxisDir, DragDir) : 0.0f,
	                  (CurrentAxis & EAxisList::Z) ? FVector2D::DotProduct(ZAxisDir, DragDir) : 0.0f);

	// Snap to grid in widget axis space
	//const FVector GridSize = FVector(GEditor->GetGridSize());
	//FSnappingUtils::SnapPointToGrid(OutDrag, GridSize);

	// Convert to effective screen space delta, and replace input delta, adjusted for inverted screen space Y axis
	const FVector2D EffectiveDelta = OutDrag.X * XAxisDir + OutDrag.Y * YAxisDir + OutDrag.Z * ZAxisDir;
	InOutDelta                     = FVector(EffectiveDelta.X, -EffectiveDelta.Y, 0.0f);

	// Transform drag delta into world space
	OutDrag = RenderData.CoordSystem.TransformPosition(OutDrag);
}

void ASodaWidget::ConvertMouseToAxis_Rotate(FVector2D TangentDir, FVector2D DragDir, FSceneView* InView,
                                        USodaGameViewportClient* InViewportClient, FVector& InOutDelta,
                                        FRotator& OutRotation)
{
	if (CurrentAxis == EAxisList::X)
	{
		FRotator Rotation;
		FVector2D EffectiveDelta;
		// Get screen direction representing positive rotation
		const FVector2D AxisDir = bIsOrthoDrawingFullRing ? TangentDir : XAxisDir;

		// Get rotation in widget local space
		Rotation = FRotator(0, 0, FVector2D::DotProduct(AxisDir, DragDir));
		//FSnappingUtils::SnapRotatorToGrid(Rotation);

		// Record delta rotation (used by the widget to render the accumulated delta)
		CurrentDeltaRotation = Rotation.Roll;

		// Use to calculate the new input delta
		EffectiveDelta = AxisDir * Rotation.Roll;
		// Adjust the input delta according to how much rotation was actually applied
		InOutDelta = FVector(EffectiveDelta.X, -EffectiveDelta.Y, 0.0f);
		// Need to get the delta rotation in the current coordinate space of the widget
		OutRotation = (RenderData.CoordSystem.Inverse() * FRotationMatrix(Rotation) * RenderData.CoordSystem).Rotator();
	}
	else if (CurrentAxis == EAxisList::Y)
	{
		FRotator Rotation;
		FVector2D EffectiveDelta;
		// TODO: Determine why -TangentDir is necessary here, and fix whatever is causing it
		const FVector2D AxisDir = bIsOrthoDrawingFullRing ? -TangentDir : YAxisDir;

		Rotation = FRotator(FVector2D::DotProduct(AxisDir, DragDir), 0, 0);
		//FSnappingUtils::SnapRotatorToGrid(Rotation);

		CurrentDeltaRotation = Rotation.Pitch;
		EffectiveDelta       = AxisDir * Rotation.Pitch;
		// Adjust the input delta according to how much rotation was actually applied
		InOutDelta = FVector(EffectiveDelta.X, -EffectiveDelta.Y, 0.0f);
		// Need to get the delta rotation in the current coordinate space of the widget
		OutRotation = (RenderData.CoordSystem.Inverse() * FRotationMatrix(Rotation) * RenderData.CoordSystem).Rotator();
	}
	else if (CurrentAxis == EAxisList::Z)
	{
		FRotator Rotation;
		FVector2D EffectiveDelta;
		const FVector2D AxisDir = bIsOrthoDrawingFullRing ? TangentDir : ZAxisDir;

		Rotation = FRotator(0, FVector2D::DotProduct(AxisDir, DragDir), 0);
		//FSnappingUtils::SnapRotatorToGrid(Rotation);

		CurrentDeltaRotation = Rotation.Yaw;
		EffectiveDelta       = AxisDir * Rotation.Yaw;
		// Adjust the input delta according to how much rotation was actually applied
		InOutDelta = FVector(EffectiveDelta.X, -EffectiveDelta.Y, 0.0f);
		// Need to get the delta rotation in the current coordinate space of the widget
		OutRotation = (RenderData.CoordSystem.Inverse() * FRotationMatrix(Rotation) * RenderData.CoordSystem).Rotator();
	}
	else if (CurrentAxis == EAxisList::XYZ) //arcball rotate
	{
		/*
		//To do this we need to calculate the rotation axis and rotation angle.
		//The Axis is the cross product of the current ray from eye to pixel in world space with the previous ray.
		//The Angle is angle amount we rotate from the object's location to the imaginary sphere that matches up with the difference between the current and previous ray
		//From the Camera. Those rays form a triangle, which we can bisect with a common side.
		FVector2D MousePosition(InViewportClient->Viewport->GetMouseX(), InViewportClient->Viewport->GetMouseY());
		FViewportCursorLocation OldMouseViewportRay(InView, InViewportClient, LastDragPos.X, LastDragPos.Y);
		FViewportCursorLocation MouseViewportRay(InView, InViewportClient, MousePosition.X, MousePosition.Y);

		LastDragPos               = MousePosition;
		FVector DirectionToWidget = InViewportClient->GetWidgetLocation() - MouseViewportRay.GetOrigin();
		float Length              = DirectionToWidget.Size();
		if (!FMath::IsNearlyZero(Length))
		{
			//Calc Axis
			DirectionToWidget /= Length;
			const FVector CameraToPixelDir    = MouseViewportRay.GetDirection();
			const FVector OldCameraToPixelDir = OldMouseViewportRay.GetDirection();
			FVector RotationAxis              = FVector::CrossProduct(OldCameraToPixelDir, CameraToPixelDir);
			RotationAxis.Normalize();
			float RotationAngle     = 0.0f;
			FVector4 ScreenLocation = InView->WorldToScreen(InViewportClient->GetWidgetLocation());
			FVector2D PixelLocation;
			InView->ScreenToPixel(ScreenLocation, PixelLocation);
			float Distance      = FVector2D::Distance(PixelLocation, MousePosition);
			const float MaxDiff = 2.0f * (INNER_AXIS_CIRCLE_RADIUS);
			//If outside radius, do screen rotate instead, like other DCC's

			if (Distance > MaxDiff)
			{
				FPlane Plane(InViewportClient->GetWidgetLocation(), DirectionToWidget);
				FVector StartOnPlane =
				    FMath::RayPlaneIntersection(MouseViewportRay.GetOrigin(), CameraToPixelDir, Plane);
				FVector OldOnPlane =
				    FMath::RayPlaneIntersection(MouseViewportRay.GetOrigin(), OldCameraToPixelDir, Plane);
				StartOnPlane -= InViewportClient->GetWidgetLocation();
				OldOnPlane -= InViewportClient->GetWidgetLocation();
				StartOnPlane.Normalize();
				OldOnPlane.Normalize();
				RotationAngle = FMath::Acos(FVector::DotProduct(StartOnPlane, OldOnPlane));

				FVector Cross = FVector::CrossProduct(OldCameraToPixelDir, CameraToPixelDir);
				if (FVector::DotProduct(DirectionToWidget, Cross) < 0.0f)
				{
					RotationAngle *= -1.0f;
				}
				RotationAxis = DirectionToWidget;
			}
			else
			{
				const float Scale = ScreenLocation.W *
				    (4.0f / InView->UnscaledViewRect.Width() / InView->ViewMatrices.GetProjectionMatrix().M[0][0]);
				const float InnerRadius = (INNER_AXIS_CIRCLE_RADIUS * Scale) + TransformWidgetSizeAdjustment;
				const float LengthOfAdjacent = Length - InnerRadius;
				RotationAngle                = FMath::Acos(FVector::DotProduct(OldCameraToPixelDir, CameraToPixelDir));
				const float OppositeSize     = FMath::Tan(RotationAngle) * LengthOfAdjacent;
				RotationAngle                = FMath::Atan(OppositeSize / InnerRadius);
				RotationAngle                = RotationAngle < 0.0f ? RotationAngle : -RotationAngle;
			}

			const FQuat QuatRotation(RotationAxis, RotationAngle);
			OutRotation = FRotator(QuatRotation);
		}
		*/
		return;
	}
	else if (CurrentAxis == EAxisList::Screen)
	{
		/*
		FVector2D MousePosition(InViewportClient->Viewport->GetMouseX(), InViewportClient->Viewport->GetMouseY());
		FViewportCursorLocation OldMouseViewportRay(InView, InViewportClient, LastDragPos.X, LastDragPos.Y);
		FViewportCursorLocation MouseViewportRay(InView, InViewportClient, MousePosition.X, MousePosition.Y);

		LastDragPos               = MousePosition;
		FVector DirectionToWidget = InViewportClient->GetWidgetLocation() - MouseViewportRay.GetOrigin();
		float Length              = DirectionToWidget.Size();

		if (!FMath::IsNearlyZero(Length))
		{
			DirectionToWidget /= Length;

			const FVector CameraToPixelDir    = MouseViewportRay.GetDirection();
			const FVector OldCameraToPixelDir = OldMouseViewportRay.GetDirection();
			FPlane Plane(InViewportClient->GetWidgetLocation(), DirectionToWidget);
			FVector StartOnPlane = FMath::RayPlaneIntersection(MouseViewportRay.GetOrigin(), CameraToPixelDir, Plane);
			FVector OldOnPlane = FMath::RayPlaneIntersection(MouseViewportRay.GetOrigin(), OldCameraToPixelDir, Plane);
			StartOnPlane -= InViewportClient->GetWidgetLocation();
			OldOnPlane -= InViewportClient->GetWidgetLocation();
			StartOnPlane.Normalize();
			OldOnPlane.Normalize();
			float RotationAngle = FMath::Acos(FVector::DotProduct(StartOnPlane, OldOnPlane));
			FVector Cross       = FVector::CrossProduct(OldCameraToPixelDir, CameraToPixelDir);
			if (FVector::DotProduct(DirectionToWidget, Cross) < 0.0f)
			{
				RotationAngle *= -1.0f;
			}
			const FQuat QuatRotation(DirectionToWidget, RotationAngle);
			OutRotation = FRotator(QuatRotation);
		}
		*/
		return;
	}
}

void ASodaWidget::ConvertMouseToAxis_Scale(FVector2D DragDir, FVector& InOutDelta, FVector& OutScale)
{
	FVector2D AxisDir = FVector2D::ZeroVector;

	if (CurrentAxis & EAxisList::X)
	{
		AxisDir += XAxisDir;
	}

	if (CurrentAxis & EAxisList::Y)
	{
		AxisDir += YAxisDir;
	}

	if (CurrentAxis & EAxisList::Z)
	{
		AxisDir += ZAxisDir;
	}

	AxisDir.Normalize();
	const float ScaleDelta = FVector2D::DotProduct(AxisDir, DragDir);

	OutScale =
	    FVector((CurrentAxis & EAxisList::X) ? ScaleDelta : 0.0f, (CurrentAxis & EAxisList::Y) ? ScaleDelta : 0.0f,
	            (CurrentAxis & EAxisList::Z) ? ScaleDelta : 0.0f);

	// Snap to grid in widget axis space
	//const FVector GridSize = FVector(GEditor->GetGridSize());
	//FSnappingUtils::SnapScale(OutScale, GridSize);

	// Convert to effective screen space delta, and replace input delta, adjusted for inverted screen space Y axis
	const float ScaleMax           = OutScale.GetMax();
	const float ScaleMin           = OutScale.GetMin();
	const float ScaleApplied       = (ScaleMax > -ScaleMin) ? ScaleMax : ScaleMin;
	const FVector2D EffectiveDelta = AxisDir * ScaleApplied;
	InOutDelta                     = FVector(EffectiveDelta.X, -EffectiveDelta.Y, 0.0f);
}

void ASodaWidget::ConvertMouseToAxis_TranslateRotateZ(FVector2D TangentDir, FVector2D DragDir, FVector& InOutDelta,
                                                  FVector& OutDrag, FRotator& OutRotation)
{
	if (CurrentAxis == EAxisList::ZRotation)
	{
		const FVector2D AxisDir = bIsOrthoDrawingFullRing ? TangentDir : ZAxisDir;
		FRotator Rotation       = FRotator(0, FVector2D::DotProduct(AxisDir, DragDir), 0);
		//FSnappingUtils::SnapRotatorToGrid(Rotation);
		CurrentDeltaRotation = Rotation.Yaw;

		const FVector2D EffectiveDelta = AxisDir * Rotation.Yaw;
		InOutDelta                     = FVector(EffectiveDelta.X, -EffectiveDelta.Y, 0.0f);

		OutRotation = (RenderData.CoordSystem.Inverse() * FRotationMatrix(Rotation) * RenderData.CoordSystem).Rotator();
	}
	else
	{
		// Get drag delta in widget axis space
		OutDrag = FVector((CurrentAxis & EAxisList::X) ? FVector2D::DotProduct(XAxisDir, DragDir) : 0.0f,
		                  (CurrentAxis & EAxisList::Y) ? FVector2D::DotProduct(YAxisDir, DragDir) : 0.0f,
		                  (CurrentAxis & EAxisList::Z) ? FVector2D::DotProduct(ZAxisDir, DragDir) : 0.0f);

		// Snap to grid in widget axis space
		//const FVector GridSize = FVector(GEditor->GetGridSize());
		//FSnappingUtils::SnapPointToGrid(OutDrag, GridSize);

		// Convert to effective screen space delta, and replace input delta, adjusted for inverted screen space Y axis
		const FVector2D EffectiveDelta = OutDrag.X * XAxisDir + OutDrag.Y * YAxisDir + OutDrag.Z * ZAxisDir;
		InOutDelta                     = FVector(EffectiveDelta.X, -EffectiveDelta.Y, 0.0f);

		// Transform drag delta into world space
		OutDrag = RenderData.CoordSystem.TransformPosition(OutDrag);
	}
}

void ASodaWidget::ConvertMouseToAxis_WM_2D(FVector2D TangentDir, FVector2D DragDir, FVector& InOutDelta, FVector& OutDrag,
                                       FRotator& OutRotation)
{
	if (CurrentAxis == EAxisList::Rotate2D)
	{
		// TODO: Determine why -TangentDir is necessary here, and fix whatever is causing it
		const FVector2D AxisDir = bIsOrthoDrawingFullRing ? -TangentDir : YAxisDir;

		FRotator Rotation = FRotator(FVector2D::DotProduct(AxisDir, DragDir), 0, 0);
		//FSnappingUtils::SnapRotatorToGrid(Rotation);

		CurrentDeltaRotation     = Rotation.Pitch;
		FVector2D EffectiveDelta = AxisDir * Rotation.Pitch;


		// Adjust the input delta according to how much rotation was actually applied
		InOutDelta = FVector(EffectiveDelta.X, -EffectiveDelta.Y, 0.0f);

		// Need to get the delta rotation in the current coordinate space of the widget
		OutRotation = (RenderData.CoordSystem.Inverse() * FRotationMatrix(Rotation) * RenderData.CoordSystem).Rotator();
	}
	else
	{
		// Get drag delta in widget axis space
		OutDrag = FVector((CurrentAxis & EAxisList::X) ? FVector2D::DotProduct(XAxisDir, DragDir) : 0.0f,
		                  (CurrentAxis & EAxisList::Y) ? FVector2D::DotProduct(YAxisDir, DragDir) : 0.0f,
		                  (CurrentAxis & EAxisList::Z) ? FVector2D::DotProduct(ZAxisDir, DragDir) : 0.0f);

		// Snap to grid in widget axis space
		//const FVector GridSize = FVector(GEditor->GetGridSize());
		//FSnappingUtils::SnapPointToGrid(OutDrag, GridSize);

		// Convert to effective screen space delta, and replace input delta, adjusted for inverted screen space Y axis
		const FVector2D EffectiveDelta = OutDrag.X * XAxisDir + OutDrag.Y * YAxisDir + OutDrag.Z * ZAxisDir;
		InOutDelta                     = FVector(EffectiveDelta.X, -EffectiveDelta.Y, 0.0f);

		// Transform drag delta into world space
		OutDrag = RenderData.CoordSystem.TransformPosition(OutDrag);
	}
}

/**
 * Converts mouse movement on the screen to widget axis movement/rotation.
 */
void ASodaWidget::ConvertMouseMovementToAxisMovement(FSceneView* InView, USodaGameViewportClient* InViewportClient,
                                                 bool bInUsedDragModifier, FVector& InOutDelta, FVector& OutDrag,
                                                 FRotator& OutRotation, FVector& OutScale)
{
	OutDrag     = FVector::ZeroVector;
	OutRotation = FRotator::ZeroRotator;
	OutScale    = FVector::ZeroVector;

	const int32 WidgetMode = InViewportClient->GetWidgetMode();

	// Get input delta as 2D vector, adjusted for inverted screen space Y axis
	const FVector2D DragDir = FVector2D(InOutDelta.X, -InOutDelta.Y);

	// Get offset of the drag start position from the widget origin
	const FVector2D DirectionToMousePos = FVector2D(DragStartPos - RenderData.Origin).GetSafeNormal();

	// For rotations which display as a full ring, calculate the tangent direction representing a clockwise movement
	FVector2D TangentDir = bInUsedDragModifier ?
	    // If a drag modifier has been used, this implies we are not actually touching the widget, so don't attempt to
	    // calculate the tangent dir based on the relative offset of the cursor from the widget location.
	    FVector2D(1, 1).GetSafeNormal() :
	    // Treat the tangent dir as perpendicular to the relative offset of the cursor from the widget location.
	    FVector2D(-DirectionToMousePos.Y, DirectionToMousePos.X);

	switch (WidgetMode)
	{
	case soda::EWidgetMode::WM_Translate:
		ConvertMouseToAxis_Translate(DragDir, InOutDelta, OutDrag);
		break;
	case soda::EWidgetMode::WM_Rotate:
		ConvertMouseToAxis_Rotate(TangentDir, DragDir, InView, InViewportClient, InOutDelta, OutRotation);
		break;
	case soda::EWidgetMode::WM_Scale:
		ConvertMouseToAxis_Scale(DragDir, InOutDelta, OutScale);
		break;
	default:
		break;
	}
}

/**
 * For axis movement, get the "best" planar normal and axis mask
 * @param InAxis - Axis of movement
 * @param InDirToPixel -
 * @param OutPlaneNormal - Normal of the plane to project the mouse onto
 * @param OutMask - Used to mask out the component of the planar movement we want
 */
void GetAxisPlaneNormalAndMask(const FMatrix& InCoordSystem, const FVector& InAxis, const FVector& InDirToPixel,
                               FVector& OutPlaneNormal, FVector& NormalToRemove)
{
	FVector XAxis = InCoordSystem.TransformVector(FVector(1, 0, 0));
	FVector YAxis = InCoordSystem.TransformVector(FVector(0, 1, 0));
	FVector ZAxis = InCoordSystem.TransformVector(FVector(0, 0, 1));

	float XDot = FMath::Abs(InDirToPixel | XAxis);
	float YDot = FMath::Abs(InDirToPixel | YAxis);
	float ZDot = FMath::Abs(InDirToPixel | ZAxis);

	if ((InAxis | XAxis) > .1f)
	{
		OutPlaneNormal = (YDot > ZDot) ? YAxis : ZAxis;
		NormalToRemove = (YDot > ZDot) ? ZAxis : YAxis;
	}
	else if ((InAxis | YAxis) > .1f)
	{
		OutPlaneNormal = (XDot > ZDot) ? XAxis : ZAxis;
		NormalToRemove = (XDot > ZDot) ? ZAxis : XAxis;
	}
	else
	{
		OutPlaneNormal = (XDot > YDot) ? XAxis : YAxis;
		NormalToRemove = (XDot > YDot) ? YAxis : XAxis;
	}
}

/**
 * For planar movement, get the "best" planar normal and axis mask
 * @param InAxis - Axis of movement
 * @param OutPlaneNormal - Normal of the plane to project the mouse onto
 * @param OutMask - Used to mask out the component of the planar movement we want
 */
void GetPlaneNormalAndMask(const FVector& InAxis, FVector& OutPlaneNormal, FVector& NormalToRemove)
{
	OutPlaneNormal = InAxis;
	NormalToRemove = InAxis;
}

void ASodaWidget::AbsoluteConvertMouseToAxis_Translate(FSceneView* InView, const FMatrix& InputCoordSystem,
                                                   FAbsoluteMovementParams& InOutParams, FVector& OutDrag)
{
	switch (CurrentAxis)
	{
	case EAxisList::X:
		GetAxisPlaneNormalAndMask(InputCoordSystem, InOutParams.XAxis, InOutParams.CameraDir, InOutParams.PlaneNormal,
		                          InOutParams.NormalToRemove);
		break;
	case EAxisList::Y:
		GetAxisPlaneNormalAndMask(InputCoordSystem, InOutParams.YAxis, InOutParams.CameraDir, InOutParams.PlaneNormal,
		                          InOutParams.NormalToRemove);
		break;
	case EAxisList::Z:
		GetAxisPlaneNormalAndMask(InputCoordSystem, InOutParams.ZAxis, InOutParams.CameraDir, InOutParams.PlaneNormal,
		                          InOutParams.NormalToRemove);
		break;
	case EAxisList::XY:
		GetPlaneNormalAndMask(InOutParams.ZAxis, InOutParams.PlaneNormal, InOutParams.NormalToRemove);
		break;
	case EAxisList::XZ:
		GetPlaneNormalAndMask(InOutParams.YAxis, InOutParams.PlaneNormal, InOutParams.NormalToRemove);
		break;
	case EAxisList::YZ:
		GetPlaneNormalAndMask(InOutParams.XAxis, InOutParams.PlaneNormal, InOutParams.NormalToRemove);
		break;
	case EAxisList::Screen:
		InOutParams.XAxis = InView->ViewMatrices.GetViewMatrix().GetColumn(0);
		InOutParams.YAxis = InView->ViewMatrices.GetViewMatrix().GetColumn(1);
		InOutParams.ZAxis = InView->ViewMatrices.GetViewMatrix().GetColumn(2);
		GetPlaneNormalAndMask(InOutParams.ZAxis, InOutParams.PlaneNormal, InOutParams.NormalToRemove);
		//do not damp the movement in this case, we also want to snap
		InOutParams.bMovementLockedToCamera = false;
		break;
	}

	OutDrag = GetAbsoluteTranslationDelta(InOutParams);
}

/**
 * Absolute Translation conversion from mouse movement on the screen to widget axis movement/rotation.
 */
void ASodaWidget::AbsoluteTranslationConvertMouseMovementToAxisMovement(FSceneView* InView,
                                                                    USodaGameViewportClient* InViewportClient,
                                                                    const FVector& InLocation,
                                                                    const FVector2D& InMousePosition, FVector& OutDrag,
                                                                    FRotator& OutRotation, FVector& OutScale)
{
	
	// Compute a world space ray from the screen space mouse coordinates
	FEditorUtils::FViewportCursorLocation MouseViewportRay(InView, InViewportClient, InMousePosition.X, InMousePosition.Y);

	FAbsoluteMovementParams Params;
	Params.EyePos    = MouseViewportRay.GetOrigin();
	Params.PixelDir  = MouseViewportRay.GetDirection();
	Params.CameraDir = InView->GetViewDirection();
	Params.Position  = InLocation;
	//dampen by
	Params.bMovementLockedToCamera = false; // InViewportClient->IsShiftPressed();
	Params.bPositionSnapping       = true;

	FMatrix InputCoordSystem = InViewportClient->GetWidgetCoordSystem();

	Params.XAxis = InputCoordSystem.TransformVector(FVector(1, 0, 0));
	Params.YAxis = InputCoordSystem.TransformVector(FVector(0, 1, 0));
	Params.ZAxis = InputCoordSystem.TransformVector(FVector(0, 0, 1));

	switch (InViewportClient->GetWidgetMode())
	{
	case soda::EWidgetMode::WM_Translate:
		AbsoluteConvertMouseToAxis_Translate(InView, InputCoordSystem, Params, OutDrag);
		break;
	case soda::EWidgetMode::WM_Rotate:
	case soda::EWidgetMode::WM_Scale:
	case soda::EWidgetMode::WM_None:
	case soda::EWidgetMode::WM_Max:
		break;
	}
	
}

/** Only some modes support Absolute Translation Movement */
bool ASodaWidget::AllowsAbsoluteTranslationMovement(soda::EWidgetMode WidgetMode)
{
	if (WidgetMode == soda::EWidgetMode::WM_Translate)
	{
		return true;
	}
	return false;
}

/** Only some modes support Absolute Rotation Movement/arcball*/
bool ASodaWidget::AllowsAbsoluteRotationMovement(soda::EWidgetMode WidgetMode, EAxisList::Type InAxisType)
{
	if (WidgetMode == soda::EWidgetMode::WM_Rotate && (InAxisType == EAxisList::XYZ || InAxisType == EAxisList::Screen))
	{
		return true;
	}
	return false;
}

#define CAMERA_LOCK_DAMPING_FACTOR .1f
#define MAX_CAMERA_MOVEMENT_SPEED 512.0f
/**
 * Returns the Delta from the current position that the absolute movement system wants the object to be at
 * @param InParams - Structure containing all the information needed for absolute movement
 * @return - The requested delta from the current position
 */
FVector ASodaWidget::GetAbsoluteTranslationDelta(const FAbsoluteMovementParams& InParams)
{
	FPlane MovementPlane(InParams.Position, InParams.PlaneNormal);
	FVector ProposedEndofEyeVector =
	    InParams.EyePos + (InParams.PixelDir * (InParams.Position - InParams.EyePos).Size());

	//default to not moving
	FVector RequestedPosition = InParams.Position;

	float DotProductWithPlaneNormal = InParams.PixelDir | InParams.PlaneNormal;
	//check to make sure we're not co-planar
	if (FMath::Abs(DotProductWithPlaneNormal) > DELTA)
	{
		//Get closest point on plane
		RequestedPosition = FMath::LinePlaneIntersection(InParams.EyePos, ProposedEndofEyeVector, MovementPlane);
	}

	//drag is a delta position, so just update the different between the previous position and the new position
	FVector DeltaPosition = RequestedPosition - InParams.Position;

	//Retrieve the initial offset, passing in the current requested position and the current position
	FVector InitialOffset = GetAbsoluteTranslationInitialOffset(RequestedPosition, InParams.Position);

	//subtract off the initial offset (where the widget was clicked) to prevent popping
	DeltaPosition -= InitialOffset;

	//remove the component along the normal we want to mute
	float MovementAlongMutedAxis = DeltaPosition | InParams.NormalToRemove;
	FVector OutDrag              = DeltaPosition - (InParams.NormalToRemove * MovementAlongMutedAxis);

	if (InParams.bMovementLockedToCamera)
	{
		//DAMPEN ABSOLUTE MOVEMENT when the camera is locked to the object
		OutDrag *= CAMERA_LOCK_DAMPING_FACTOR;
		OutDrag.X = FMath::Clamp<FVector::FReal>(OutDrag.X, -MAX_CAMERA_MOVEMENT_SPEED, MAX_CAMERA_MOVEMENT_SPEED);
		OutDrag.Y = FMath::Clamp<FVector::FReal>(OutDrag.Y, -MAX_CAMERA_MOVEMENT_SPEED, MAX_CAMERA_MOVEMENT_SPEED);
		OutDrag.Z = FMath::Clamp<FVector::FReal>(OutDrag.Z, -MAX_CAMERA_MOVEMENT_SPEED, MAX_CAMERA_MOVEMENT_SPEED);
	}

	//the they requested position snapping and we're not moving with the camera
	if (InParams.bPositionSnapping && !InParams.bMovementLockedToCamera && bSnapEnabled)
	{
		FVector MovementAlongAxis =
		    FVector(OutDrag | InParams.XAxis, OutDrag | InParams.YAxis, OutDrag | InParams.ZAxis);
		//translation (either xy plane or z)
		//FSnappingUtils::SnapPointToGrid( MovementAlongAxis, FVector(GEditor->GetGridSize(), GEditor->GetGridSize(), GEditor->GetGridSize()));

		OutDrag = MovementAlongAxis.X * InParams.XAxis + MovementAlongAxis.Y * InParams.YAxis +
		    MovementAlongAxis.Z * InParams.ZAxis;
	}

	//get the distance from the original position to the new proposed position
	FVector DeltaFromStart = InParams.Position + OutDrag - InitialTranslationPosition;

	//Get the vector from the eye to the proposed new position (to make sure it's not behind the camera
	FVector EyeToNewPosition        = (InParams.Position + OutDrag) - InParams.EyePos;
	float BehindTheCameraDotProduct = EyeToNewPosition | InParams.CameraDir;

	//Don't let the requested position go behind the camera
	if (BehindTheCameraDotProduct <= 0)
	{
		OutDrag = OutDrag.ZeroVector;
	}
	return OutDrag;
}

/**
 * Returns the offset from the initial selection point
 */
FVector ASodaWidget::GetAbsoluteTranslationInitialOffset(const FVector& InNewPosition, const FVector& InCurrentPosition)
{
	if (!bAbsoluteTranslationInitialOffsetCached)
	{
		bAbsoluteTranslationInitialOffsetCached = true;
		InitialTranslationOffset                = InNewPosition - InCurrentPosition;
		InitialTranslationPosition              = InCurrentPosition;
	}
	return InitialTranslationOffset;
}


/**
 * Returns true if we're in Local Space editing mode
 */
bool ASodaWidget::IsRotationLocalSpace() const
{
	return (RenderData.CoordSystemSpace == soda::COORD_Local);
}

void ASodaWidget::UpdateDeltaRotation()
{
	TotalDeltaRotation += CurrentDeltaRotation;
	if ((TotalDeltaRotation <= -360.f) || (TotalDeltaRotation >= 360.f))
	{
		TotalDeltaRotation = FRotator::ClampAxis(TotalDeltaRotation);
	}
}

/**
 * Returns the angle in degrees representation of how far we have just rotated
 */
float ASodaWidget::GetDeltaRotation() const
{
	return TotalDeltaRotation;
}

uint32 ASodaWidget::GetDominantAxisIndex(const FVector& InDiff, USodaGameViewportClient* ViewportClient) const
{
	uint32 DominantIndex = 0;
	if (FMath::Abs(InDiff.X) < FMath::Abs(InDiff.Y))
	{
		DominantIndex = 1;
	}
	return DominantIndex;
}

bool ASodaWidget::IsWidgetDisabled() const
{
	return false; //EditorModeTools ? (EditorModeTools->IsDefaultModeActive() && GEditor->HasLockedActors()) : false;
}
