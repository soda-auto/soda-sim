// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/VehicleComponents/VehicleSensorComponent.h"
#include "Soda/Vehicles/SodaVehicle.h"
#include "Soda/UnrealSoda.h"
#include "Soda/SodaApp.h"
#include "Soda/Misc/SensorSceneProxy.h"
#include "Components/SceneCaptureComponent2D.h"
#include "UObject/ConstructorHelpers.h"
#include "Materials/Material.h"
#include "RuntimeMetaData.h"

USensorComponent::USensorComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	GUI.IcanName = TEXT("SodaIcons.Sensor");

	SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	//SetGenerateOverlapEvents(false);

	static ConstructorHelpers::FObjectFinder< UMaterial > SensorFOVMaterialFinder(TEXT("/SodaSim/Assets/CPP/SensorFOVMaterial"));
	SensorFOVMaterial = SensorFOVMaterialFinder.Object;
}

void USensorComponent::HideActorComponentsFromSensorView(AActor* Actor)
{
	const TArray<USceneComponent*> Children = GetAttachChildren();

	for (auto& Child : Children)
	{
		USceneCaptureComponent2D* Capture2D = Cast<USceneCaptureComponent2D>(Child);
		if (Capture2D)
		{
			Capture2D->HideActorComponents(Actor);
		}
	}
}

void USensorComponent::HideComponentFromSensorView(UPrimitiveComponent* PrimitiveComponent)
{
	const TArray<USceneComponent*> Children = GetAttachChildren();

	for (auto& Child : Children)
	{
		USceneCaptureComponent2D* Capture2D = Cast<USceneCaptureComponent2D>(Child);
		if (Capture2D)
		{
			Capture2D->HideComponent(PrimitiveComponent);
		}
	}
}

bool USensorComponent::OnActivateVehicleComponent()
{
	bool bRet =  Super::OnActivateVehicleComponent();

	MarkRenderStateDirty();

	return bRet;
}

void USensorComponent::OnDeactivateVehicleComponent()
{
	Super::OnDeactivateVehicleComponent();
}

FPrimitiveSceneProxy* USensorComponent::CreateSceneProxy()
{
	if (NeedRenderSensorFOV())
	{
		return new FSensorSceneProxy(this);
	}
	else
	{
		return nullptr;
	}
}

void USensorComponent::GetUsedMaterials(TArray<UMaterialInterface*>& OutMaterials, bool bGetDebugMaterials) const
{
	if (SensorFOVMaterial)
	{
		OutMaterials.AddUnique(SensorFOVMaterial);
	}
}

void USensorComponent::RuntimePostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::RuntimePostEditChangeProperty(PropertyChangedEvent);

	if (FRuntimeMetaData::HasMetaData(PropertyChangedEvent.Property, "UpdateFOVRendering"))
	{
		MarkRenderStateDirty();
	}
}

void USensorComponent::RuntimePostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent)
{
	Super::RuntimePostEditChangeChainProperty(PropertyChangedEvent);

	if (FRuntimeMetaData::HasMetaData(PropertyChangedEvent.Property, "UpdateFOVRendering"))
	{
		MarkRenderStateDirty();
	}
}




bool FSensorFOVRenderer::NeedRenderSensorFOV(const USensorComponent* Component) const
{
	if (!Component)
	{
		return false;
	}

	bool bIsSelected = Component->IsVehicleComponentSelected();
	bool bIsActivated = Component->IsVehicleComponentActiveted();

	return ((FOVRenderingStrategy == EFOVRenderingStrategy::Ever) ||
		(FOVRenderingStrategy == EFOVRenderingStrategy::OnSelect && bIsSelected) ||
		(FOVRenderingStrategy == EFOVRenderingStrategy::OnSelectWhenActive && bIsSelected && bIsActivated) ||
		(FOVRenderingStrategy == EFOVRenderingStrategy::EverWhenActive && bIsActivated));
}