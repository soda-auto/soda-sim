// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/Misc/SensorSceneProxy.h"
#include "EngineGlobals.h"
#include "RHI.h"
#include "RenderingThread.h"
#include "RenderResource.h"
#include "VertexFactory.h"
#include "LocalVertexFactory.h"
#include "PrimitiveViewRelevance.h"
#include "PrimitiveSceneProxy.h"
#include "Engine/Engine.h"
#include "MaterialShared.h"
#include "Materials/Material.h"
#include "Engine/CollisionProfile.h"
#include "SceneManagement.h"
#include "DynamicMeshBuilder.h"
#include "UObject/UObjectHash.h"
#include "UObject/UObjectIterator.h"
#include "StaticMeshResources.h"
#include "DynamicMeshBuilder.h"
#include "Materials/MaterialInstance.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "SceneInterface.h"

//#include "ConstrainedDelaunay2.h"
//#include "Polygon2.h"
//#include "DynamicMesh/DynamicMesh3.h"


struct FSensoFOVProxy
{
	//FSensoFOVProxy(const FSensoFOVProxy& Other) = default;
	//FSensoFOVProxy(FSensoFOVProxy&& Other) = default;
	FSensoFOVProxy(ERHIFeatureLevel::Type InFeatureLevel) : 
		VertexFactory(InFeatureLevel, "SensoFOVProxy") 
	{
	}

	~FSensoFOVProxy()
	{
		ReleaseResources();
	}

	void InitMesh(TArray<FDynamicMeshVertex>& Vertices, TArray<uint32>& Indices)
	{
		if (Indices.Num() > 3)
		{
			IndexBuffer.Indices = Indices;
			VertexBuffers.InitFromDynamicVertex(&VertexFactory, Vertices);
			BeginInitResource(&VertexBuffers.PositionVertexBuffer);
			BeginInitResource(&VertexBuffers.StaticMeshVertexBuffer);
			BeginInitResource(&VertexBuffers.ColorVertexBuffer);
			BeginInitResource(&VertexFactory);
			BeginInitResource(&IndexBuffer);
		}
	}

	void ReleaseResources()
	{
		VertexBuffers.PositionVertexBuffer.ReleaseResource();
		VertexBuffers.StaticMeshVertexBuffer.ReleaseResource();
		VertexBuffers.ColorVertexBuffer.ReleaseResource();
		VertexFactory.ReleaseResource();
		IndexBuffer.ReleaseResource();
	}


	//TArray<FVector> LanePoints;
	FStaticMeshVertexBuffers VertexBuffers;
	FDynamicMeshIndexBuffer32 IndexBuffer;
	FLocalVertexFactory VertexFactory;
	FMaterialRenderProxy* Material = nullptr;
	//FLinearColor Color{ FColor(255, 255, 255) };
};


FSensorSceneProxy::FSensorSceneProxy(USensorComponent* Sensor)
	: FPrimitiveSceneProxy(Sensor)
	, Sensor(Sensor)

{
	if (Sensor->GetSensorFOVMaterial())
	{
		MaterialRenderProxy = Sensor->GetSensorFOVMaterial()->GetRenderProxy();
		TArray<FSensorFOVMesh> Meshes;
		if (Sensor->GenerateFOVMesh(Meshes))
		{
			for (auto& Mesh : Meshes)
			{
				FSensoFOVProxy& FOVProxy = *FOVProxies.Add_GetRef(MakeShared< FSensoFOVProxy>(GetScene().GetFeatureLevel()));
				FOVProxy.InitMesh(Mesh.Vertices, Mesh.Indices);

			}
		}

		MaterialRelevance |= Sensor->GetSensorFOVMaterial()->GetRelevance_Concurrent(GetScene().GetFeatureLevel());
	}

}

void FSensorSceneProxy::GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, FMeshElementCollector& Collector) const
{
	for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++)
	{
		if (VisibilityMap & (1 << ViewIndex))
		{
			const FSceneView* View = Views[ViewIndex];
			FPrimitiveDrawInterface* PDI = Collector.GetPDI(ViewIndex);

			for (const auto& FOVProxy : FOVProxies)
			{
				FDynamicPrimitiveUniformBuffer& DynamicPrimitiveUniformBuffer = Collector.AllocateOneFrameResource<FDynamicPrimitiveUniformBuffer>();

				DynamicPrimitiveUniformBuffer.Set(GetLocalToWorld(), GetLocalToWorld(), GetBounds(), GetLocalBounds(), true, false, AlwaysHasVelocity());
				

				FMeshBatch& MeshBatch = Collector.AllocateMesh();
				MeshBatch.MaterialRenderProxy = MaterialRenderProxy;
				MeshBatch.VertexFactory = &FOVProxy->VertexFactory;
				MeshBatch.ReverseCulling = IsLocalToWorldDeterminantNegative();
				MeshBatch.Type = PT_TriangleList;
				MeshBatch.DepthPriorityGroup = SDPG_World;
				MeshBatch.bCanApplyViewModeOverrides = false;

				FMeshBatchElement& BatchElement = MeshBatch.Elements[0];
				BatchElement.IndexBuffer = &FOVProxy->IndexBuffer;
				BatchElement.FirstIndex = 0;
				BatchElement.NumPrimitives = FOVProxy->IndexBuffer.Indices.Num() / 3;
				BatchElement.MinVertexIndex = 0;
				BatchElement.MaxVertexIndex = FOVProxy->VertexBuffers.PositionVertexBuffer.GetNumVertices() - 1;
				BatchElement.PrimitiveUniformBufferResource = &DynamicPrimitiveUniformBuffer.UniformBuffer;

				Collector.AddMesh(ViewIndex, MeshBatch);
			}
		}
	}
}

FPrimitiveViewRelevance FSensorSceneProxy::GetViewRelevance(const FSceneView* View) const
{
	FPrimitiveViewRelevance Result;
	Result.bDrawRelevance = IsShown(View);
	Result.bShadowRelevance = IsShadowCast(View);
	Result.bDynamicRelevance = true;
	Result.bRenderInMainPass = ShouldRenderInMainPass();
	Result.bUsesLightingChannels = GetLightingChannelMask() != GetDefaultLightingChannelMask();
	Result.bRenderCustomDepth = ShouldRenderCustomDepth();
	Result.bTranslucentSelfShadow = bCastVolumetricTranslucentShadow;
	MaterialRelevance.SetPrimitiveViewRelevance(Result);
	Result.bVelocityRelevance = DrawsVelocity() && Result.bOpaque && Result.bRenderInMainPass;
	return Result;
}

bool FSensorSceneProxy::CanBeOccluded() const
{
	return !MaterialRelevance.bDisableDepthTest;
}

void FSensorSceneProxy::OnTransformChanged()
{
	FPrimitiveSceneProxy::OnTransformChanged();
}
