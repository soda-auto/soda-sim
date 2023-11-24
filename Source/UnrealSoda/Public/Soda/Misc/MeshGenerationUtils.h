// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "GameFramework/Actor.h"
#include "ProceduralMeshComponent.h"

namespace FMeshGenerationUtils
{
	/**
	* Generate FOV mesh based on 2D (UVs) map. The UVs map is projection of sensor rays on the forward plane;  
	* Each UVs point corresponds to a direction vector (Cloud). Size of UVs == size of Cloud.
	* This type of mesh generation is ideal for sensors such as lidars, cameras and radars.
	*/
	bool UNREALSODA_API GenerateFOVMesh2D(const TArray<FVector>& Cloud, const TArray<FVector2D>& UVs, const TArray<int32> & Hull, TArray<FDynamicMeshVertex> & OutVertices, TArray<uint32> & OutIndices);

	/**
	* Generate convex hull FOV mesh based on the cloud of points.
	*/
	bool UNREALSODA_API GenerateConvexHullMesh3D(const TArray<FVector>& Cloud, TArray<FDynamicMeshVertex>& OutVertices, TArray<uint32>& OutIndices);

	bool UNREALSODA_API GenerateSimpleFOVMesh(float HFOV, float VFOV, uint32 HSize, uint32 VSize, float Length, float HRot, float VRot, TArray<FDynamicMeshVertex>& OutVertices, TArray<uint32>& OutIndices);

	FBox UNREALSODA_API CalcFOVBounds(float HFOV, float VFOV, float ViewDistance);

	/**
	* Generate a hull for a matrix like this:
	* 0  1  2  3
	* 4  5  6  7
	* 8  9  10 11
	* Output hull indexes: 0, 1, 2, 3, 7, 11, 10, 9, 8, 4
	*/
	inline void GenerateHullInexes(int Colls, int Rows, TArray <int32>& Hull)
	{
		Hull.Empty();
		for (int u = 0; u < Colls; ++u) Hull.Add(u);
		for (int v = Colls * 2 - 1; v < Colls * Rows; v = v + Colls)  Hull.Add(v);
		for (int u = Rows * Colls - 2; u >= Colls * Rows - Colls; --u) Hull.Add(u);
		for (int v = Colls * (Rows - 2); v > 0; v = v - Colls)  Hull.Add(v);
	}

}
