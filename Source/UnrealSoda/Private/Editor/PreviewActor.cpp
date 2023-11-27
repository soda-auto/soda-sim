// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "PreviewActor.h"
#include "Components/SceneComponent.h"
#include "DynamicMeshBuilder.h"
#include "Materials/MaterialInterface.h"
#include "Materials/Material.h"
#include "SceneManagement.h"
#include "SceneView.h"

#define INNER_AXIS_CIRCLE_RADIUS  48.0f
#define OUTER_AXIS_CIRCLE_RADIUS  56.0f
#define AXIS_CIRCLE_SIDES  24

static void DrawThickArc(
	FPrimitiveDrawInterface* PDI,
	UMaterialInterface* Material,
	const FVector& InLocation, 
	float InnerRadius, 
	float OuterRadius, 
	const FVector& Axis0, 
	const FVector& Axis1, 
	const float InStartAngle, 
	const float InEndAngle, 
	const FColor& InColor)
{
	if (InColor.A == 0)
	{
		return;
	}

	const int32 NumPoints = FMath::TruncToInt(AXIS_CIRCLE_SIDES * (InEndAngle - InStartAngle) / (PI / 2)) + 1;

	FColor TriangleColor = InColor;
	FColor RingColor = InColor;
	RingColor.A = MAX_uint8;

	FVector ZAxis = Axis0 ^ Axis1;
	FVector LastWorldVertex;

	FDynamicMeshBuilder MeshBuilder(PDI->View->GetFeatureLevel());

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

			FVector VertexDir = Axis0.RotateAngleAxis(AngleDeg, ZAxis);
			VertexDir.Normalize();

			float TCAngle = Percent * (PI / 2);
			FVector2f TC(TCRadius * FMath::Cos(Angle), TCRadius * FMath::Sin(Angle));

			// Keep the vertices in local space so that we don't lose precision when dealing with LWC
			// The local-to-world transform is handled in the MeshBuilder.Draw() call at the end of this function
			const FVector VertexPosition = VertexDir * Radius;
			FVector Normal = VertexPosition;
			Normal.Normalize();

			FDynamicMeshVertex MeshVertex;
			MeshVertex.Position = (FVector3f)VertexPosition;
			MeshVertex.Color = TriangleColor;
			MeshVertex.TextureCoordinate[0] = TC;

			MeshVertex.SetTangents(
				(FVector3f)-ZAxis,
				FVector3f((-ZAxis) ^ Normal),
				(FVector3f)Normal
			);

			MeshBuilder.AddVertex(MeshVertex); //Add bottom vertex

			// Push out the arc line borders so they dont z-fight with the mesh arcs
			// DrawLine needs vertices in world space, but this is fine because it takes FVectors and works with LWC well
			FVector StartLinePos = LastWorldVertex;
			FVector EndLinePos = VertexPosition + InLocation;
			if (VertexIndex != 0)
			{
				PDI->DrawLine(StartLinePos, EndLinePos, RingColor, SDPG_Foreground);
			}
			LastWorldVertex = EndLinePos;
		}
	}

	//Add top/bottom triangles, in the style of a fan.
	int32 InnerVertexStartIndex = NumPoints + 1;
	for (int32 VertexIndex = 0; VertexIndex < NumPoints; VertexIndex++)
	{
		MeshBuilder.AddTriangle(VertexIndex, VertexIndex + 1, InnerVertexStartIndex + VertexIndex);
		MeshBuilder.AddTriangle(VertexIndex + 1, InnerVertexStartIndex + VertexIndex + 1, InnerVertexStartIndex + VertexIndex);
	}


	MeshBuilder.Draw(PDI, FTranslationMatrix(InLocation), Material->GetRenderProxy(), SDPG_Foreground, 0.f);
}

static void DrawSnapMarker(
	FPrimitiveDrawInterface* PDI,
	UMaterialInterface* Material,
	const FVector& InLocation, 
	const FVector& Axis0, 
	const FVector& Axis1, 
	const FColor& InColor, 
	const float InScale, 
	const float InWidthPercent, 
	const float InPercentSize = 1.0)
{
	const float InnerDistance = ((OUTER_AXIS_CIRCLE_RADIUS + 5) * InScale);
	const float OuterDistance = ((OUTER_AXIS_CIRCLE_RADIUS + 20) * InScale);
	const float MaxMarkerHeight = OuterDistance - InnerDistance;
	const float MarkerWidth = MaxMarkerHeight * InWidthPercent;
	const float MarkerHeight = MaxMarkerHeight * InPercentSize;

	FVector LocalVertices[4];
	LocalVertices[0] = (OuterDistance)*Axis0 - (MarkerWidth * .5) * Axis1;
	LocalVertices[1] = LocalVertices[0] + (MarkerWidth)*Axis1;
	LocalVertices[2] = (OuterDistance - MarkerHeight) * Axis0 - (MarkerWidth * .5) * Axis1;
	LocalVertices[3] = LocalVertices[2] + (MarkerWidth)*Axis1;

	//draw at least one line
	// DrawLine needs vertices in world space, but this is fine because it takes FVectors and works with LWC well
	PDI->DrawLine(LocalVertices[0] + InLocation, LocalVertices[2] + InLocation, InColor, SDPG_Foreground);

	//if there should be thickness, draw the other lines
	if (InWidthPercent > 0.0f)
	{
		PDI->DrawLine(LocalVertices[0] + InLocation, LocalVertices[1] + InLocation, InColor, SDPG_Foreground);
		PDI->DrawLine(LocalVertices[1] + InLocation, LocalVertices[3] + InLocation, InColor, SDPG_Foreground);
		PDI->DrawLine(LocalVertices[2] + InLocation, LocalVertices[3] + InLocation, InColor, SDPG_Foreground);


		//fill in the box
		FDynamicMeshBuilder MeshBuilder(PDI->View->GetFeatureLevel());

		for (int32 VertexIndex = 0; VertexIndex < 4; VertexIndex++)
		{
			FDynamicMeshVertex MeshVertex;
			MeshVertex.Position = (FVector3f)LocalVertices[VertexIndex];
			MeshVertex.Color = InColor;
			MeshVertex.TextureCoordinate[0] = FVector2f(0.0f, 0.0f);
			MeshVertex.SetTangents(
				(FVector3f)Axis0,
				(FVector3f)Axis1,
				FVector3f((Axis0) ^ Axis1)
			);
			MeshBuilder.AddVertex(MeshVertex); //Add bottom vertex
		}

		MeshBuilder.AddTriangle(0, 1, 2);
		MeshBuilder.AddTriangle(1, 3, 2);
		MeshBuilder.Draw(PDI, FTranslationMatrix(InLocation), Material->GetRenderProxy(), SDPG_Foreground, 0.f);
	}
}


APreviewActor::APreviewActor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	RootComponent = CreateDefaultSubobject< USceneComponent >(TEXT("Default"));

	TransparentMaterial = (UMaterial*)StaticLoadObject(
		UMaterial::StaticClass(), NULL,
		TEXT("/SodaSim/Assets/CPP/EditorMaterials/WidgetVertexColorMaterial.WidgetVertexColorMaterial"), NULL, LOAD_None, NULL);

	GridMaterial = (UMaterial*)StaticLoadObject(
		UMaterial::StaticClass(), NULL,
		TEXT("/SodaSim/Assets/CPP/EditorMaterials/WidgetGridVertexColorMaterial_Ma.WidgetGridVertexColorMaterial_Ma"), NULL,
		LOAD_None, NULL);

	if (!GridMaterial)
	{
		GridMaterial = TransparentMaterial;
	}
}

void APreviewActor::RenderTarget(FPrimitiveDrawInterface* PDI)
{
	static FVector XAxis = FVector(1, 0, 0);
	static FVector YAxis = FVector(0, 1, 0);
	static FVector ZAxis = FVector(0, 0, 1);

	static FVector Axis0 = XAxis;
	static FVector Axis1 = YAxis;

	FVector Location = GetActorLocation();

	//always draw clockwise, so if we're negative we need to flip the angle
	float StartAngle = -PI;
	float FilledAngle = PI * 2;

	FColor ArcColor = FColor(100, 100, 250, 200);
	const float InnerRadius = (INNER_AXIS_CIRCLE_RADIUS * RenderScale);
	const float OuterRadius = (OUTER_AXIS_CIRCLE_RADIUS * RenderScale);

	FColor OuterColor = ArcColor;
	DrawThickArc(PDI, TransparentMaterial, Location, InnerRadius, OuterRadius, Axis0, Axis1, -PI, PI, OuterColor);

	FColor InnerColor = ArcColor;
	InnerColor.A = 50;
	DrawThickArc(PDI, GridMaterial, Location, 0.0f, InnerRadius, Axis0, Axis1, -PI, PI, InnerColor);

	//draw axis tick marks
	FColor AxisColor = FLinearColor(0.0251f, 0.207f, 0.85f, 0.5).ToFColor(true);

	//Rotate Colors to match Axis 0
	Swap(AxisColor.R, AxisColor.G);
	Swap(AxisColor.B, AxisColor.R);
	DrawSnapMarker(PDI, TransparentMaterial, Location, Axis0, Axis1, AxisColor, RenderScale, .25f);
	DrawSnapMarker(PDI, TransparentMaterial, Location, -Axis0, -Axis1, AxisColor, RenderScale, .25f);

	//Rotate Colors to match Axis 1
	Swap(AxisColor.R, AxisColor.G);
	Swap(AxisColor.B, AxisColor.R);
	DrawSnapMarker(PDI, TransparentMaterial, Location, Axis1, -Axis0, AxisColor, RenderScale, .25f);
	DrawSnapMarker(PDI, TransparentMaterial, Location, -Axis1, Axis0, AxisColor, RenderScale, .25f);
}

