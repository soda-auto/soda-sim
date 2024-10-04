// Copyright Epic Games, Inc. All Rights Reserved.
// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "DynamicMeshBuilder.h"


namespace MeshUtils
{

inline uint32 AddVertex (const FVector3f& InPosition, const FVector2f& InTextureCoordinate, const FVector3f& InTangentX, const FVector3f& InTangentY, const FVector3f& InTangentZ, const FColor& InColor, TArray<FDynamicMeshVertex>& InOutVertices)
{
	int32 VertexIndex = InOutVertices.Num();
	auto& Vertex = InOutVertices.Add_GetRef({});
	Vertex.Position = InPosition;
	Vertex.TextureCoordinate[0] = InTextureCoordinate;
	Vertex.TangentX = InTangentX;
	Vertex.TangentZ = InTangentZ;
	Vertex.TangentZ.Vector.W = GetBasisDeterminantSignByte(InTangentX, InTangentY, InTangentZ);
	Vertex.Color = InColor;
	return VertexIndex;
}

//void BuildConeVerts(float Angle1, float Angle2, float Scale, float XOffset, uint32 NumSides, TArray<FDynamicMeshVertex>& OutVerts, TArray<uint32>& OutIndices);

inline void BuildCylinderVerts(const FVector& Base, const FVector& XAxis, const FVector& YAxis, const FVector& ZAxis, double Radius, double HalfHeight, uint32 Sides, TArray<FDynamicMeshVertex>& OutVerts, TArray<uint32>& OutIndices)
{
	const float	AngleDelta = 2.0f * UE_PI / Sides;
	FVector	LastVertex = Base + XAxis * Radius;

	FVector2D TC = FVector2D(0.0f, 0.0f);
	float TCStep = 1.0f / Sides;

	FVector TopOffset = HalfHeight * ZAxis;

	int32 BaseVertIndex = OutVerts.Num();

	//Compute vertices for base circle.
	for (uint32 SideIndex = 0; SideIndex < Sides; SideIndex++)
	{
		const FVector Vertex = Base + (XAxis * FMath::Cos(AngleDelta * (SideIndex + 1)) + YAxis * FMath::Sin(AngleDelta * (SideIndex + 1))) * Radius;
		FVector Normal = Vertex - Base;
		Normal.Normalize();

		FDynamicMeshVertex MeshVertex;

		MeshVertex.Position = FVector3f(Vertex - TopOffset);
		MeshVertex.TextureCoordinate[0] = FVector2f(TC);

		MeshVertex.SetTangents(
			(FVector3f)-ZAxis,
			FVector3f((-ZAxis) ^ Normal),
			(FVector3f)Normal
		);

		OutVerts.Add(MeshVertex); //Add bottom vertex

		LastVertex = Vertex;
		TC.X += TCStep;
	}

	LastVertex = Base + XAxis * Radius;
	TC = FVector2D(0.0f, 1.0f);

	//Compute vertices for the top circle
	for (uint32 SideIndex = 0; SideIndex < Sides; SideIndex++)
	{
		const FVector Vertex = Base + (XAxis * FMath::Cos(AngleDelta * (SideIndex + 1)) + YAxis * FMath::Sin(AngleDelta * (SideIndex + 1))) * Radius;
		FVector Normal = Vertex - Base;
		Normal.Normalize();

		FDynamicMeshVertex MeshVertex;

		MeshVertex.Position = FVector3f(Vertex + TopOffset);
		MeshVertex.TextureCoordinate[0] = FVector2f(TC);

		MeshVertex.SetTangents(
			(FVector3f)-ZAxis,
			FVector3f((-ZAxis) ^ Normal),
			(FVector3f)Normal
		);

		OutVerts.Add(MeshVertex); //Add top vertex

		LastVertex = Vertex;
		TC.X += TCStep;
	}

	//Add top/bottom triangles, in the style of a fan.
	//Note if we wanted nice rendering of the caps then we need to duplicate the vertices and modify
	//texture/tangent coordinates.
	for (uint32 SideIndex = 1; SideIndex < Sides; SideIndex++)
	{
		int32 V0 = BaseVertIndex;
		int32 V1 = BaseVertIndex + SideIndex;
		int32 V2 = BaseVertIndex + ((SideIndex + 1) % Sides);

		//bottom
		OutIndices.Add(V0);
		OutIndices.Add(V1);
		OutIndices.Add(V2);

		// top
		OutIndices.Add(Sides + V2);
		OutIndices.Add(Sides + V1);
		OutIndices.Add(Sides + V0);
	}

	//Add sides.

	for (uint32 SideIndex = 0; SideIndex < Sides; SideIndex++)
	{
		int32 V0 = BaseVertIndex + SideIndex;
		int32 V1 = BaseVertIndex + ((SideIndex + 1) % Sides);
		int32 V2 = V0 + Sides;
		int32 V3 = V1 + Sides;

		OutIndices.Add(V0);
		OutIndices.Add(V2);
		OutIndices.Add(V1);

		OutIndices.Add(V2);
		OutIndices.Add(V3);
		OutIndices.Add(V1);
	}
}

void DrawBox( const FVector& Radii, const FMaterialRenderProxy* MaterialRenderProxy, uint8 DepthPriorityGroup, TArray<FDynamicMeshVertex>& OutVertices, TArray<uint32>& OutIndices)
{
	// Calculate verts for a face pointing down Z
	FVector3f Positions[4] =
	{
		FVector3f(-1, -1, +1),
		FVector3f(-1, +1, +1),
		FVector3f(+1, +1, +1),
		FVector3f(+1, -1, +1)
	};
	FVector2f UVs[4] =
	{
		FVector2f(0,0),
		FVector2f(0,1),
		FVector2f(1,1),
		FVector2f(1,0),
	};

	// Then rotate this face 6 times
	FRotator3f FaceRotations[6];
	FaceRotations[0] = FRotator3f(0, 0, 0);
	FaceRotations[1] = FRotator3f(90.f, 0, 0);
	FaceRotations[2] = FRotator3f(-90.f, 0, 0);
	FaceRotations[3] = FRotator3f(0, 0, 90.f);
	FaceRotations[4] = FRotator3f(0, 0, -90.f);
	FaceRotations[5] = FRotator3f(180.f, 0, 0);


	for (int32 f = 0; f < 6; f++)
	{
		FMatrix44f FaceTransform = FRotationMatrix44f(FaceRotations[f]);

		uint32 VertexIndices[4];
		for (int32 VertexIndex = 0; VertexIndex < 4; VertexIndex++)
		{
			VertexIndices[VertexIndex] = AddVertex(
				FaceTransform.TransformPosition(Positions[VertexIndex]),
				UVs[VertexIndex],
				FaceTransform.TransformVector(FVector3f(1, 0, 0)),
				FaceTransform.TransformVector(FVector3f(0, 1, 0)),
				FaceTransform.TransformVector(FVector3f(0, 0, 1)),
				FColor::White,
				OutVertices
			);
		}

		OutIndices.Append({ VertexIndices[0], VertexIndices[1], VertexIndices[2] });
		OutIndices.Append({ VertexIndices[0], VertexIndices[2], VertexIndices[3] });
	}
}

inline void DrawCornerHelper(const FMatrix& LocalToWorld, const FVector& Length, float Thickness, TArray<FDynamicMeshVertex> & OutVertices, TArray<uint32> & OutIndices)
{
	const float TH = Thickness;

	float TX = Length.X/2;
	float TY = Length.Y/2;
	float TZ = Length.Z/2;

	// Top
	{
		uint32 VertexIndices[4];
		VertexIndices[0] = AddVertex( FVector3f(-TX, -TY, +TZ), FVector2f::ZeroVector, FVector3f(1,0,0), FVector3f(0,1,0), FVector3f(0,0,1), FColor::White, OutVertices);
		VertexIndices[1] = AddVertex( FVector3f(-TX, +TY, +TZ), FVector2f::ZeroVector, FVector3f(1,0,0), FVector3f(0,1,0), FVector3f(0,0,1), FColor::White, OutVertices);
		VertexIndices[2] = AddVertex( FVector3f(+TX, +TY, +TZ), FVector2f::ZeroVector, FVector3f(1,0,0), FVector3f(0,1,0), FVector3f(0,0,1), FColor::White, OutVertices);
		VertexIndices[3] = AddVertex( FVector3f(+TX, -TY, +TZ), FVector2f::ZeroVector, FVector3f(1,0,0), FVector3f(0,1,0), FVector3f(0,0,1), FColor::White, OutVertices);

		OutIndices.Append({ VertexIndices[0],VertexIndices[1],VertexIndices[2] });
		OutIndices.Append({ VertexIndices[0],VertexIndices[2],VertexIndices[3] });
	}

	//Left
	{
		uint32 VertexIndices[4];
		VertexIndices[0] = AddVertex( FVector3f(-TX,  -TY, TZ-TH),	FVector2f::ZeroVector, FVector3f(0,0,1), FVector3f(0,1,0), FVector3f(-1,0,0), FColor::White, OutVertices);
		VertexIndices[1] = AddVertex( FVector3f(-TX, -TY, TZ),		FVector2f::ZeroVector, FVector3f(0,0,1), FVector3f(0,1,0), FVector3f(-1,0,0), FColor::White, OutVertices);
		VertexIndices[2] = AddVertex( FVector3f(-TX, +TY, TZ),		FVector2f::ZeroVector, FVector3f(0,0,1), FVector3f(0,1,0), FVector3f(-1,0,0), FColor::White, OutVertices);
		VertexIndices[3] = AddVertex( FVector3f(-TX, +TY, TZ-TH),	FVector2f::ZeroVector, FVector3f(0,0,1), FVector3f(0,1,0), FVector3f(-1,0,0), FColor::White, OutVertices);

		OutIndices.Append({ VertexIndices[0],VertexIndices[1],VertexIndices[2] });
		OutIndices.Append({ VertexIndices[0],VertexIndices[2],VertexIndices[3] });
	}

	// Front
	{
		uint32 VertexIndices[5];
		VertexIndices[0] = AddVertex( FVector3f(-TX,	+TY, TZ-TH),	FVector2f::ZeroVector, FVector3f(1,0,0), FVector3f(0,0,-1), FVector3f(0,1,0), FColor::White, OutVertices);
		VertexIndices[1] = AddVertex( FVector3f(-TX,	+TY, +TZ  ),	FVector2f::ZeroVector, FVector3f(1,0,0), FVector3f(0,0,-1), FVector3f(0,1,0), FColor::White, OutVertices);
		VertexIndices[2] = AddVertex( FVector3f(+TX-TH, +TY, +TX  ),	FVector2f::ZeroVector, FVector3f(1,0,0), FVector3f(0,0,-1), FVector3f(0,1,0), FColor::White, OutVertices);
		VertexIndices[3] = AddVertex( FVector3f(+TX,	+TY, +TZ  ),	FVector2f::ZeroVector, FVector3f(1,0,0), FVector3f(0,0,-1), FVector3f(0,1,0), FColor::White, OutVertices);
		VertexIndices[4] = AddVertex( FVector3f(+TX-TH, +TY, TZ-TH),	FVector2f::ZeroVector, FVector3f(1,0,0), FVector3f(0,0,-1), FVector3f(0,1,0), FColor::White, OutVertices);

		OutIndices.Append({ VertexIndices[0],VertexIndices[1],VertexIndices[2] });
		OutIndices.Append({ VertexIndices[0],VertexIndices[2],VertexIndices[4] });
		OutIndices.Append({ VertexIndices[4],VertexIndices[2],VertexIndices[3] });
	}

	// Back
	{
		uint32 VertexIndices[5];
		VertexIndices[0] = AddVertex( FVector3f(-TX,	-TY, TZ-TH),	FVector2f::ZeroVector, FVector3f(1,0,0), FVector3f(0,0,1), FVector3f(0,-1,0), FColor::White, OutVertices);
		VertexIndices[1] = AddVertex( FVector3f(-TX,	-TY, +TZ),		FVector2f::ZeroVector, FVector3f(1,0,0), FVector3f(0,0,1), FVector3f(0,-1,0), FColor::White, OutVertices);
		VertexIndices[2] = AddVertex( FVector3f(+TX-TH, -TY, +TX),		FVector2f::ZeroVector, FVector3f(1,0,0), FVector3f(0,0,1), FVector3f(0,-1,0), FColor::White, OutVertices);
		VertexIndices[3] = AddVertex( FVector3f(+TX,	-TY, +TZ),		FVector2f::ZeroVector, FVector3f(1,0,0), FVector3f(0,0,1), FVector3f(0,-1,0), FColor::White, OutVertices);
		VertexIndices[4] = AddVertex( FVector3f(+TX-TH, -TY, TZ-TH),	FVector2f::ZeroVector, FVector3f(1,0,0), FVector3f(0,0,1), FVector3f(0,-1,0), FColor::White, OutVertices);

		OutIndices.Append({ VertexIndices[0],VertexIndices[1],VertexIndices[2] });
		OutIndices.Append({ VertexIndices[0],VertexIndices[2],VertexIndices[4] });
		OutIndices.Append({ VertexIndices[4],VertexIndices[2],VertexIndices[3] });
	}
	// Bottom
	{
		uint32 VertexIndices[4];
		VertexIndices[0] = AddVertex( FVector3f(-TX, -TY, TZ-TH),		FVector2f::ZeroVector, FVector3f(1,0,0), FVector3f(0,0,-1), FVector3f(0,0,1), FColor::White, OutVertices);
		VertexIndices[1] = AddVertex( FVector3f(-TX, +TY, TZ-TH),		FVector2f::ZeroVector, FVector3f(1,0,0), FVector3f(0,0,-1), FVector3f(0,0,1), FColor::White, OutVertices);
		VertexIndices[2] = AddVertex( FVector3f(+TX-TH, +TY, TZ-TH),	FVector2f::ZeroVector, FVector3f(1,0,0), FVector3f(0,0,-1), FVector3f(0,0,1), FColor::White, OutVertices);
		VertexIndices[3] = AddVertex( FVector3f(+TX-TH, -TY, TZ-TH),	FVector2f::ZeroVector, FVector3f(1,0,0), FVector3f(0,0,-1), FVector3f(0,0,1), FColor::White, OutVertices);

		OutIndices.Append({ VertexIndices[0],VertexIndices[1],VertexIndices[2] });
		OutIndices.Append({ VertexIndices[0],VertexIndices[2],VertexIndices[3] });
	}
}

inline void DrawColoredSphere(const FVector& Center, const FRotator& Orientation, FColor Color, const FVector& Radii, uint32 NumSides, uint32 NumRings, TArray<FDynamicMeshVertex>& OutVertices, TArray<uint32>& OutIndices)
{

	// The first/last arc are on top of each other.
	uint32 NumVerts = (NumSides + 1) * (NumRings + 1);

	TArray<FDynamicMeshVertex> Verts;
	Verts.SetNum(NumVerts);

	TArray<FDynamicMeshVertex> ArcVerts;
	ArcVerts.SetNum(NumRings + 1);

	for (uint32 i = 0; i < NumRings + 1; i++)
	{
		FDynamicMeshVertex* ArcVert = &ArcVerts[i];

		float angle = ((float)i / NumRings) * PI;

		ArcVert->Color = Color;
		// Note- unit sphere, so position always has mag of one. We can just use it for normal!			
		ArcVert->Position.X = 0.0f;
		ArcVert->Position.Y = FMath::Sin(angle);
		ArcVert->Position.Z = FMath::Cos(angle);

		ArcVert->SetTangents(
			FVector3f(1, 0, 0),
			FVector3f(0.0f, -ArcVert->Position.Z, ArcVert->Position.Y),
			ArcVert->Position
		);

		ArcVert->TextureCoordinate[0].X = 0.0f;
		ArcVert->TextureCoordinate[0].Y = ((float)i / NumRings);
	}

	// Then rotate this arc NumSides+1 times.
	for (uint32 s = 0; s < NumSides + 1; s++)
	{
		FRotator3f ArcRotator(0, 360.f * (float)s / NumSides, 0);
		FRotationMatrix44f ArcRot(ArcRotator);
		float XTexCoord = ((float)s / NumSides);

		for (uint32 v = 0; v < NumRings + 1; v++)
		{
			int32 VIx = (NumRings + 1)*s + v;
			Verts[VIx].Color = Color;
			Verts[VIx].Position = ArcRot.TransformPosition(ArcVerts[v].Position);

			Verts[VIx].SetTangents(
				ArcRot.TransformVector(ArcVerts[v].TangentX.ToFVector3f()),
				ArcRot.TransformVector(ArcVerts[v].GetTangentY()),
				ArcRot.TransformVector(ArcVerts[v].TangentZ.ToFVector3f())
			);

			Verts[VIx].TextureCoordinate[0].X = XTexCoord;
			Verts[VIx].TextureCoordinate[0].Y = ArcVerts[v].TextureCoordinate[0].Y;
		}
	}

	// Add all of the vertices we generated to the mesh builder.
	for (uint32 VertIdx = 0; VertIdx < NumVerts; VertIdx++)
	{
		OutVertices.Add(Verts[VertIdx]);
	}

	// Add all of the triangles we generated to the mesh builder.
	for (uint32 s = 0; s < NumSides; s++)
	{
		uint32 a0start = (s + 0) * (NumRings + 1);
		uint32 a1start = (s + 1) * (NumRings + 1);

		for (uint32 r = 0; r < NumRings; r++)
		{
			OutIndices.Append({ a0start + r + 0, a1start + r + 0, a0start + r + 1 });
			OutIndices.Append({ a1start + r + 0, a1start + r + 1, a0start + r + 1 });
		}
	}
}

/**
 * Renders a portion of an arc for the rotation widget
 * @param InParams - Material, Radii, etc
 * @param InStartAxis - Start of the arc, in radians
 * @param InEndAxis - End of the arc, in radians
 * @param InColor - Color to use for the arc
 */
inline void DrawThickArc (uint32 CircleSides, float InnerRadius, float OuterRadius, const FVector& Axis0, const FVector& Axis1, const float InStartAngle, const float InEndAngle, const FColor& InColor, bool bIsOrtho, TArray<FDynamicMeshVertex>& OutVertices, TArray<uint32>& OutIndices)
{
	if (InColor.A == 0)
	{
		return;
	}

	// Add more sides to the circle if we've been scaled up to keep the circle looking circular
	// An extra side for every 5 extra unreal units seems to produce a nice result
	const uint32 NumPoints = FMath::TruncToInt(CircleSides * (InEndAngle-InStartAngle)/(PI/2)) + 1;

	FColor TriangleColor = InColor;
	FColor RingColor = InColor;
	RingColor.A = MAX_uint8;

	FVector ZAxis = Axis0 ^ Axis1;
	FVector LastWorldVertex;

	for (uint32 RadiusIndex = 0; RadiusIndex < 2; ++RadiusIndex)
	{
		float Radius = (RadiusIndex == 0) ? OuterRadius : InnerRadius;
		float TCRadius = Radius / (float) OuterRadius;
		//Compute vertices for base circle.
		for(uint32 VertexIndex = 0;VertexIndex <= NumPoints;VertexIndex++)
		{
			float Percent = VertexIndex/(float)NumPoints;
			float Angle = FMath::Lerp(InStartAngle, InEndAngle, Percent);
			float AngleDeg = FRotator::ClampAxis(Angle * 180.f / PI);

			FVector VertexDir = Axis0.RotateAngleAxis(AngleDeg, ZAxis);
			VertexDir.Normalize();

			float TCAngle = Percent*(PI/2);
			FVector2f TC(TCRadius*FMath::Cos(Angle), TCRadius*FMath::Sin(Angle));

			// Keep the vertices in local space so that we don't lose precision when dealing with LWC
			const FVector VertexPosition = VertexDir*Radius;
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

			OutVertices.Add(MeshVertex); //Add bottom vertex

			// Push out the arc line borders so they dont z-fight with the mesh arcs
			// DrawLine needs vertices in world space, but this is fine because it takes FVectors and works with LWC well
			FVector StartLinePos = LastWorldVertex;
			FVector EndLinePos = VertexPosition;// + InParams.Position;
			if (VertexIndex != 0)
			{
				//PDI->DrawLine(StartLinePos,EndLinePos,RingColor,SDPG_Foreground);
			}
			LastWorldVertex = EndLinePos;
		}
	}

	//Add top/bottom triangles, in the style of a fan.
	uint32 InnerVertexStartIndex = NumPoints + 1;
	for(uint32 VertexIndex = 0; VertexIndex < NumPoints; VertexIndex++)
	{
		OutIndices.Append({ VertexIndex, VertexIndex + 1, InnerVertexStartIndex + VertexIndex });
		OutIndices.Append({ VertexIndex + 1, InnerVertexStartIndex + VertexIndex + 1, InnerVertexStartIndex + VertexIndex });
	}
}

/**
 * Draws protractor like ticks where the rotation widget would snap too.
 * Also, used to draw the wider axis tick marks
 * @param PDI - Drawing interface
 * @param InLocation - The Origin of the widget
 * @param Axis0 - The Axis that describes a 0 degree rotation
 * @param Axis1 - The Axis that describes a 90 degree rotation
 * @param InAngle - The Angle to rotate about the axis of rotation, the vector (Axis0 ^ Axis1)
 * @param InColor - The color to use for line/poly drawing
 * @param InScale - Multiplier to maintain a constant screen size for rendering the widget
 * @param InWidthPercent - The percent of the distance between the outer ring and inner ring to use for tangential thickness
 * @param InPercentSize - The percent of the distance between the outer ring and inner ring to use for radial distance
 */
inline void DrawSnapMarker(float InnerDistance, float OuterDistance, const FVector& InLocation, const FVector& Axis0, const FVector& Axis1, const FColor& InColor, const float InScale, const float InWidthPercent, const float InPercentSize, TArray<FDynamicMeshVertex>& OutVertices, TArray<uint32>& OutIndices)
{
	InnerDistance = (InnerDistance * InScale);
	OuterDistance = (OuterDistance * InScale);
	const float MaxMarkerHeight = OuterDistance - InnerDistance;
	const float MarkerWidth = MaxMarkerHeight*InWidthPercent;
	const float MarkerHeight = MaxMarkerHeight*InPercentSize;

	FVector LocalVertices[4];
	LocalVertices[0] = (OuterDistance)*Axis0 - (MarkerWidth*.5)*Axis1;
	LocalVertices[1] = LocalVertices[0] + (MarkerWidth)*Axis1;
	LocalVertices[2] = (OuterDistance-MarkerHeight)*Axis0 - (MarkerWidth*.5)*Axis1;
	LocalVertices[3] = LocalVertices[2] + (MarkerWidth)*Axis1;

	//draw at least one line
	// DrawLine needs vertices in world space, but this is fine because it takes FVectors and works with LWC well
	//PDI->DrawLine(LocalVertices[0] + InLocation, LocalVertices[2] + InLocation, InColor, SDPG_Foreground);

	//if there should be thickness, draw the other lines
	if (InWidthPercent > 0.0f)
	{
		//PDI->DrawLine(LocalVertices[0] + InLocation, LocalVertices[1] + InLocation, InColor, SDPG_Foreground);
		//PDI->DrawLine(LocalVertices[1] + InLocation, LocalVertices[3] + InLocation, InColor, SDPG_Foreground);
		//PDI->DrawLine(LocalVertices[2] + InLocation, LocalVertices[3] + InLocation, InColor, SDPG_Foreground);

		//fill in the box
		for(int32 VertexIndex = 0;VertexIndex < 4; VertexIndex++)
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
			OutVertices.Add(MeshVertex); //Add bottom vertex
		}

		OutIndices.Append({ 0, 1, 2 });
		OutIndices.Append({ 1, 3, 2 });
	}
}

/**
 * Draw Start/Stop Marker to show delta rotations along the arc of rotation
 * @param PDI - Drawing interface
 * @param InLocation - The Origin of the widget
 * @param Axis0 - The Axis that describes a 0 degree rotation
 * @param Axis1 - The Axis that describes a 90 degree rotation
 * @param InAngle - The Angle to rotate about the axis of rotation, the vector (Axis0 ^ Axis1), units are degrees
 * @param InColor - The color to use for line/poly drawing
 * @param InScale - Multiplier to maintain a constant screen size for rendering the widget
 */
inline void DrawStartStopMarker(float InnerDistance, float OuterDistance, const FVector& InLocation, const FVector& Axis0, const FVector& Axis1, const float InAngle, const FColor& InColor, const float InScale, TArray<FDynamicMeshVertex>& OutVertices, TArray<uint32>& OutIndices)
{
	const float ArrowHeightPercent = .8f;
	InnerDistance = (InnerDistance * InScale);
	OuterDistance = (OuterDistance * InScale);
	const float RingHeight = OuterDistance - InnerDistance;
	const float ArrowHeight = RingHeight*ArrowHeightPercent;
	const float ThirtyDegrees = PI / 6.0f;
	const float HalfArrowidth = ArrowHeight*FMath::Tan(ThirtyDegrees);

	FVector ZAxis = Axis0 ^ Axis1;
	FVector RotatedAxis0 = Axis0.RotateAngleAxis(InAngle, ZAxis);
	FVector RotatedAxis1 = Axis1.RotateAngleAxis(InAngle, ZAxis);

	FVector LocalVertices[3];
	LocalVertices[0] = (OuterDistance)*RotatedAxis0;
	LocalVertices[1] = LocalVertices[0] + (ArrowHeight)*RotatedAxis0 - HalfArrowidth*RotatedAxis1;
	LocalVertices[2] = LocalVertices[1] + (2*HalfArrowidth)*RotatedAxis1;

	// DrawLine needs vertices in world space, but this is fine because it takes FVectors and works with LWC well
	//PDI->DrawLine(LocalVertices[0] + InLocation, LocalVertices[1] + InLocation, InColor, SDPG_Foreground);
	//PDI->DrawLine(LocalVertices[1] + InLocation, LocalVertices[2] + InLocation, InColor, SDPG_Foreground);
	//PDI->DrawLine(LocalVertices[0] + InLocation, LocalVertices[2] + InLocation, InColor, SDPG_Foreground);

	if (InColor.A > 0)
	{
		//fill in the box

		for(int32 VertexIndex = 0;VertexIndex < 3; VertexIndex++)
		{
			FDynamicMeshVertex MeshVertex;
			MeshVertex.Position = (FVector3f)LocalVertices[VertexIndex];
			MeshVertex.Color = InColor;
			MeshVertex.TextureCoordinate[0] = FVector2f(0.0f, 0.0f);
			MeshVertex.SetTangents(
				(FVector3f)RotatedAxis0,
				(FVector3f)RotatedAxis1,
				FVector3f((RotatedAxis0) ^ RotatedAxis1)
				);
			OutVertices.Add(MeshVertex); //Add bottom vertex
		}

		OutIndices.Append({ 0, 1, 2 });
	}
}

} // namespace MeshUtils