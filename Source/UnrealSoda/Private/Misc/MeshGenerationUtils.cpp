// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/Misc/MeshGenerationUtils.h"
#include "Soda/UnrealSoda.h"
#include "DrawDebugHelpers.h"
#include "Math/ConvexHull2d.h"
#include "KismetProceduralMeshLibrary.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Soda/Misc/SodaPhysicsInterface.h"
#include "Soda/Misc/MathUtils.hpp"
#include "quickhull/QuickHull.hpp"
#include "UObject/ConstructorHelpers.h"
#include "ConstrainedDelaunay2.h"
#include "DynamicMeshBuilder.h"

namespace FMeshGenerationUtils
{

bool DelaunayTriangulation(const TArray<FVector2D>& Cloud, const TArray<int>& Hull, /*TArray<FVector2D>& OutCloud,*/ TArray<uint32>& OutTriangles, bool bFlip)
{
	//OutCloud.Empty();
	OutTriangles.Empty();

	UE::Geometry::FConstrainedDelaunay2d Triangulation;
	//Triangulation.FillRule = UE::Geometry::FConstrainedDelaunay2d::EFillRule::Positive;

	for (int i = 0; i < Cloud.Num(); ++i)
	{
		Triangulation.Vertices.Add({ Cloud[i].X, Cloud[i].Y });
	}

	for (int i = 0; i < Hull.Num() - 1; ++i)
	{
		Triangulation.Edges.Add(UE::Geometry::FIndex2i(Hull[i], Hull[i+1]));
	}
	Triangulation.Edges.Add(UE::Geometry::FIndex2i(Hull[Hull.Num() - 1], Hull[0]));

	bool bTriangulationSuccess = Triangulation.Triangulate();

	if (!bTriangulationSuccess)
	{
		UE_LOG(LogSoda, Error, TEXT("FMeshGenerationUtils::DelaunayTriangulation() Can't triangulate mesh"));
		return false;
	}

	if (Triangulation.Triangles.Num() == 0)
	{
		return false;
	}

	OutTriangles.Reserve(Triangulation.Triangles.Num() * 3);

	if (bFlip)
	{
		for (const UE::Geometry::FIndex3i& Tri : Triangulation.Triangles)
		{
			OutTriangles.Add(Tri.A);
			OutTriangles.Add(Tri.B);
			OutTriangles.Add(Tri.C);
		}
	}
	else
	{
		for (const UE::Geometry::FIndex3i& Tri : Triangulation.Triangles)
		{
			OutTriangles.Add(Tri.C);
			OutTriangles.Add(Tri.B);
			OutTriangles.Add(Tri.A);
		}
	}

	return true;
}

bool GenerateFOVMesh2D(const TArray<FVector>& InCloud, const TArray<FVector2D>& InUVs, const TArray <int32> & InHull, TArray<FDynamicMeshVertex>& OutVertices, TArray<uint32>& OutIndices)
{
	OutVertices.Empty();
	OutIndices.Empty();
	OutVertices.Reserve(InCloud.Num() + 1);
	
	for (auto& Pt : InCloud)
	{
		OutVertices.Add(FDynamicMeshVertex(FVector3f(Pt)));
	}
	OutVertices.Add(FDynamicMeshVertex(FVector3f(0, 0, 0)));
	
	//UKismetProceduralMeshLibrary::CalculateTangentsForMesh(const TArray<FVector>&Vertices, const TArray<int32>&Triangles, const TArray<FVector2D>&UVs, TArray<FVector>&Normals, TArray<FProcMeshTangent>&Tangents)

	/*
	 * DelaunayTriangulation
	 */
	if (!DelaunayTriangulation(InUVs, InHull, OutIndices, false))
	{
		UE_LOG(LogSoda, Error, TEXT("FMeshGenerationUtils::GenerateFOVMesh2D() . Delaunay triangulation faild"));
		return false;
	}
	
	/*
	 * Build side faces 
	 */

	int CenterPointIndex = OutVertices.Num();
	if (IsClockwise2D(InUVs, InHull))
	{
		for (int i = 0; i < InHull.Num(); ++i)
		{
			OutIndices.Add(CenterPointIndex);
			OutIndices.Add(InHull[i]);
			OutIndices.Add(InHull[(i + 1) % InHull.Num()]);
		}
	}
	else
	{
		for (int i = 0; i < InHull.Num(); ++i)
		{
			OutIndices.Add(CenterPointIndex);
			OutIndices.Add(InHull[(i + 1) % InHull.Num()]);
			OutIndices.Add(InHull[i]);
		}
	}

	return true;
}

bool GenerateConvexHullMesh3D(const TArray<FVector>& Cloud, TArray<FDynamicMeshVertex>& OutVertices, TArray<uint32>& OutIndices)
{
	OutVertices.Empty();
	OutIndices.Empty();
	
	quickhull::QuickHull<FVector::FReal> QuickHull;
	auto Hull = QuickHull.getConvexHull((const FVector::FReal*)Cloud.GetData(), Cloud.Num(), true, false);

	const auto& IndexBuffer = Hull.getIndexBuffer();
	const auto& VertexBuffer = Hull.getVertexBuffer();

	OutVertices.Reserve(Cloud.Num());
	for (auto& Pt : VertexBuffer)
	{
		OutVertices.Add(FDynamicMeshVertex(FVector3f(Pt.x, Pt.y, Pt.z)));
	}

	OutIndices.Reserve(IndexBuffer.size());
	for (auto& Ind : IndexBuffer) OutIndices.Add(Ind);

	return true;
}


bool GenerateSimpleFOVMesh(float HFOV, float VFOV, uint32 HSize, uint32 VSize, float Length, float HRot, float VRot, TArray<FDynamicMeshVertex>& OutVertices, TArray<uint32>& OutIndices)
{
	TArray<FVector> Cloud;
	TArray<FVector2D> CloudUV;

	Cloud.Reserve(VSize * HSize);
	CloudUV.Reserve(VSize * HSize);

	for (uint32 i = 0; i < VSize + 1; i++)
	{
		float VAng = VRot - VFOV / 2 + VFOV / (float)VSize * (float)i;
		for (uint32 j = 0; j < HSize + 1; j++)
		{
			float HAng = HRot - HFOV / 2 + HFOV / (float)HSize * (float)j;
			FVector RayNorm = FRotator(VAng, HAng, 0.0f).RotateVector(FVector(1.0, 0.0, 0.0));

			Cloud.Add(RayNorm * Length);
			CloudUV.Add(FVector2D(j, VSize - i));
		}
	}

	TArray <int32> Hull;
	GenerateHullInexes(HSize + 1, VSize + 1, Hull);
	GenerateFOVMesh2D(Cloud, CloudUV, Hull, OutVertices, OutIndices);

	return true;
}

FBox CalcFOVBounds(float HFOV, float VFOV, float ViewDistance)
{
	TArray<FVector> Points = 
	{ 
		FVector::ZeroVector,
		FVector(1.0, 0.0, 0.0) * ViewDistance,
		FRotator(-VFOV / 2, -HFOV / 2, 0.0f).RotateVector(FVector(1.0, 0.0, 0.0))* ViewDistance,
		FRotator(-VFOV / 2, +HFOV / 2, 0.0f).RotateVector(FVector(1.0, 0.0, 0.0))* ViewDistance,
		FRotator(+VFOV / 2, -HFOV / 2, 0.0f).RotateVector(FVector(1.0, 0.0, 0.0))* ViewDistance,
		FRotator(+VFOV / 2, +HFOV / 2, 0.0f).RotateVector(FVector(1.0, 0.0, 0.0))* ViewDistance,
	};

	if (HFOV > 90)
	{
		Points.Append({ 
			FVector(0.0,  1.0, 0.0) * ViewDistance,
			FVector(0.0, -1.0, 0.0) * ViewDistance 
		});
	}

	if (VFOV > 90)
	{
		Points.Append({
			FVector(0.0, 0.0, 1.0) * ViewDistance,
			FVector(0.0, 0.0, -1.0) * ViewDistance
		});
	}

	return FBox(Points);
}

} // FMeshGenerationUtils