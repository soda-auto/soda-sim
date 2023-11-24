// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "Soda/VehicleComponents/VehicleSensorComponent.h"
#include "VertexFactory.h"
#include "PrimitiveSceneProxy.h"
#include "LocalVertexFactory.h"
#include "DynamicMeshBuilder.h"
//#include "PrimitiveViewRelevance.h"
//#include "Engine/Engine.h"
//#include "MaterialShared.h"
//#include "Materials/Material.h"


struct FSensoFOVProxy;

class UNREALSODA_API FSensorSceneProxy final : public FPrimitiveSceneProxy
{

public:
	FSensorSceneProxy(USensorComponent* Component);
	FSensorSceneProxy(const USensorComponent& Component) = delete;
	virtual ~FSensorSceneProxy() {}

	virtual void GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, FMeshElementCollector& Collector) const override;
	virtual FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View) const override;
	virtual bool CanBeOccluded() const override;
	virtual void OnTransformChanged() override;
	virtual uint32 GetMemoryFootprint(void) const override { return(sizeof(*this) + GetAllocatedSize()); }
	virtual SIZE_T GetTypeHash() const override
	{
		static size_t UniquePointer;
		return reinterpret_cast<size_t>(&UniquePointer);
	}

	uint32 GetAllocatedSize(void) const { return(FPrimitiveSceneProxy::GetAllocatedSize()); }

private:
	USensorComponent* Sensor = nullptr;
	UMaterialInterface* Material;
	TArray<TSharedPtr<FSensoFOVProxy>> FOVProxies;
	FMaterialRelevance MaterialRelevance;
	FMaterialRenderProxy* MaterialRenderProxy = nullptr;
};

