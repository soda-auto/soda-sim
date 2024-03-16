// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/Misc/EditorUtils.h"
#include "Slate/SGameLayerManager.h"
#include "Engine/EngineTypes.h"
#include "CollisionQueryParams.h"
#include "SceneView.h"
#include "Engine/GameViewportClient.h"
#include "Engine/LocalPlayer.h"
#include "Engine/EngineTypes.h"
#include "Engine/World.h"

FEditorUtils::FViewportCursorLocation::FViewportCursorLocation(const FSceneView* View, USodaGameViewportClient* InViewportClient, int32 X, int32 Y)
	: Origin(ForceInit), Direction(ForceInit), CursorPos(X, Y)
{

	FVector4 ScreenPos = View->PixelToScreen(X, Y, 0);

	const FMatrix InvViewMatrix = View->ViewMatrices.GetInvViewMatrix();
	const FMatrix InvProjMatrix = View->ViewMatrices.GetInvProjectionMatrix();

	const float ScreenX = ScreenPos.X;
	const float ScreenY = ScreenPos.Y;

	ViewportClient = InViewportClient;

	//if (ViewportClient->IsPerspective())
	{
		Origin = View->ViewMatrices.GetViewOrigin();
		Direction = InvViewMatrix.TransformVector(FVector(InvProjMatrix.TransformFVector4(FVector4(ScreenX * GNearClippingPlane, ScreenY * GNearClippingPlane, 0.0f, GNearClippingPlane)))).GetSafeNormal();
	}
	/*
	else
	{
		Origin = InvViewMatrix.TransformFVector4(InvProjMatrix.TransformFVector4(FVector4(ScreenX, ScreenY, 0.5f, 1.0f)));
		Direction = InvViewMatrix.TransformVector(FVector(0, 0, 1)).GetSafeNormal();
	}
	*/
}

bool FEditorUtils::AbsoluteToScreenPosition(const UGameViewportClient* ViewportClient, const FVector2D & AbsoluteCoordinate, FVector2D & OutScreenPos)
{
	TSharedPtr<IGameLayerManager> GameLayerManager = ViewportClient->GetGameLayerManager();
	if (GameLayerManager.IsValid())
	{
		FVector2D ViewportSize;
		ViewportClient->GetViewportSize(ViewportSize);
		const FGeometry& ViewportGeometry = GameLayerManager->GetViewportWidgetHostGeometry();
		const FVector2D ViewportPosition = ViewportGeometry.AbsoluteToLocal(AbsoluteCoordinate);
		OutScreenPos = (ViewportPosition / ViewportGeometry.GetLocalSize()) * ViewportSize;
		return true;
	}
	return false;
}

void FEditorUtils::DeprojectScreenToWorld(const FVector2D& ScreenPos, const FIntRect& ViewRect, const FMatrix& InvViewProjMatrix, FVector& out_WorldOrigin, FVector& out_WorldDirection)
{
	float PixelX = ScreenPos.X;
	float PixelY = ScreenPos.Y;

	// Get the eye position and direction of the mouse cursor in two stages (inverse transform projection, then inverse transform view).
	// This avoids the numerical instability that occurs when a view matrix with large translation is composed with a projection matrix

	// Get the pixel coordinates into 0..1 normalized coordinates within the constrained view rectangle
	const float NormalizedX = (PixelX - ViewRect.Min.X) / ((float)ViewRect.Width());
	const float NormalizedY = (PixelY - ViewRect.Min.Y) / ((float)ViewRect.Height());

	// Get the pixel coordinates into -1..1 projection space
	const float ScreenSpaceX = (NormalizedX - 0.5f) * 2.0f;
	const float ScreenSpaceY = ((1.0f - NormalizedY) - 0.5f) * 2.0f;

	// The start of the ray trace is defined to be at mousex,mousey,1 in projection space (z=1 is near, z=0 is far - this gives us better precision)
	// To get the direction of the ray trace we need to use any z between the near and the far plane, so let's use (mousex, mousey, 0.5)
	// !!! But here we changed 0.5 to 0.01 compared to the original code
	const FVector4 RayStartProjectionSpace = FVector4(ScreenSpaceX, ScreenSpaceY, 1.0f, 1.0f);
	const FVector4 RayEndProjectionSpace = FVector4(ScreenSpaceX, ScreenSpaceY, 0.01f, 1.0f);

	// Projection (changing the W coordinate) is not handled by the FMatrix transforms that work with vectors, so multiplications
	// by the projection matrix should use homogeneous coordinates (i.e. FPlane).
	const FVector4 HGRayStartWorldSpace = InvViewProjMatrix.TransformFVector4(RayStartProjectionSpace);
	const FVector4 HGRayEndWorldSpace = InvViewProjMatrix.TransformFVector4(RayEndProjectionSpace);
	FVector RayStartWorldSpace(HGRayStartWorldSpace.X, HGRayStartWorldSpace.Y, HGRayStartWorldSpace.Z);
	FVector RayEndWorldSpace(HGRayEndWorldSpace.X, HGRayEndWorldSpace.Y, HGRayEndWorldSpace.Z);
	// divide vectors by W to undo any projection and get the 3-space coordinate
	if (HGRayStartWorldSpace.W != 0.0f)
	{
		RayStartWorldSpace /= HGRayStartWorldSpace.W;
	}
	if (HGRayEndWorldSpace.W != 0.0f)
	{
		RayEndWorldSpace /= HGRayEndWorldSpace.W;
	}
	const FVector RayDirWorldSpace = (RayEndWorldSpace - RayStartWorldSpace).GetSafeNormal();

	// Finally, store the results in the outputs
	out_WorldOrigin = RayStartWorldSpace;
	out_WorldDirection = RayDirWorldSpace;
}

bool FEditorUtils::DeprojectScreenToWorld(const ULocalPlayer* LocalPlayer, const FVector2D& ScreenPos, FVector& WorldPosition, FVector& WorldDirection)
{
	if (LocalPlayer && LocalPlayer->ViewportClient)
	{
		// get the projection data
		FSceneViewProjectionData ProjectionData;
		if (LocalPlayer->GetProjectionData(LocalPlayer->ViewportClient->Viewport, ProjectionData))
		{
			FMatrix const InvViewProjMatrix = ProjectionData.ComputeViewProjectionMatrix().Inverse();
			DeprojectScreenToWorld(ScreenPos, ProjectionData.GetConstrainedViewRect(), InvViewProjMatrix, WorldPosition, WorldDirection);
			return true;
		}
	}

	// something went wrong, zero things and return false
	WorldPosition = FVector::ZeroVector;
	WorldDirection = FVector::ZeroVector;
	return false;
}

FEditorUtils::FActorPositionTraceResult FEditorUtils::TraceWorldForPosition(const UWorld* InWorld, const FVector& RayStart, const FVector& RayEnd, const TArray<AActor*>* IgnoreActors)
{
	TArray<FHitResult> Hits;

	FCollisionQueryParams Param(SCENE_QUERY_STAT(DragDropTrace), true);

	if (IgnoreActors)
	{
		Param.AddIgnoredActors(*IgnoreActors);
	}

	FActorPositionTraceResult Results;
	if (InWorld->LineTraceMultiByObjectType(Hits, RayStart, RayEnd, FCollisionObjectQueryParams(FCollisionObjectQueryParams::InitType::AllObjects), Param))
	{
		// Go through all hits and find closest
		float ClosestHitDistanceSqr = TNumericLimits<float>::Max();

		for (const FHitResult& Hit : Hits)
		{
			const float DistanceToHitSqr = (Hit.ImpactPoint - RayStart).SizeSquared();
			if (DistanceToHitSqr < ClosestHitDistanceSqr)
			{
				ClosestHitDistanceSqr = DistanceToHitSqr;
				Results.Location = Hit.Location;
				Results.SurfaceNormal = Hit.Normal.GetSafeNormal();
				Results.State = FActorPositionTraceResult::HitSuccess;
				Results.HitActor = Hit.HitObjectHandle.GetManagingActor();
			}
		}
	}

	if (Results.State == FActorPositionTraceResult::Failed)
	{
		Results.State = FActorPositionTraceResult::Default;
		const float DistanceMultiplier = 20000; // And put it in front of the camera
		Results.Location =  RayStart  + (RayEnd - RayStart).GetSafeNormal() * DistanceMultiplier;
	}

	return Results;
}

FEditorUtils::FActorPositionTraceResult FEditorUtils::TraceWorldForPosition(const UWorld* InWorld, const FVector2D& ScreenPos, const TArray<AActor*>* IgnoreActors)
{
	FVector Origin;
	FVector Direction;
	if (!DeprojectScreenToWorld(InWorld, ScreenPos, Origin, Direction))
	{
		return FEditorUtils::FActorPositionTraceResult();
	}

	const FVector RayEnd = Origin + Direction * HALF_WORLD_MAX;

	return TraceWorldForPosition(InWorld, Origin, RayEnd, IgnoreActors);
}


bool FEditorUtils::DeprojectScreenToWorld(const UWorld* InWorld, const FVector2D& ScreenPos, FVector& Origin, FVector& Direction)
{
	if (!InWorld)
	{
		return false;
	}

	ULocalPlayer* LocalPlayer = InWorld->GetFirstLocalPlayerFromController();
	if (!LocalPlayer)
	{
		return false;
	}

	/*
	UGameViewportClient* ViewportClient = InWorld->GetGameViewport();
	if (!ViewportClient)
	{
		return false;
	}

	
	FSceneViewFamilyContext ViewFamily(
		FSceneViewFamily::ConstructionValues(
			ViewportClient->Viewport,
			InWorld->Scene,
			ViewportClient->EngineShowFlags)
	);

	FVector OutViewLocation;
	FRotator OutViewRotation;
	FSceneView* View = LocalPlayer->CalcSceneView(
		&ViewFamily,
		OutViewLocation,
		OutViewRotation,
		ViewportClient->Viewport
	);

	if (!View)
	{
		return false;
	}
	*/

	/*
	FSceneViewProjectionData ProjectionData;
	if (!LocalPlayer->GetProjectionData(LocalPlayer->ViewportClient->Viewport, ProjectionData))
	{
		return false;
	}
	*/

	if (!DeprojectScreenToWorld(LocalPlayer, ScreenPos, Origin, Direction))
	{
		return false;
	}

	return true;
}

FName FEditorUtils::MakeUniqueObjectName(UObject* Parent, const UClass* Class, FName InBaseName)
{
	check(Class && Parent);

	if (InBaseName != NAME_None)
	{
		if (!StaticFindObjectFast(NULL, Parent, InBaseName))
		{
			return InBaseName;
		}
	}

	FName BaseName;

	if (InBaseName == NAME_None)
	{
		if (Class->HasAnyClassFlags(CLASS_CompiledFromBlueprint))
		{
			FString StrName = Class->GetFName().ToString();
			StrName.RemoveFromEnd(TEXT("_C"), ESearchCase::CaseSensitive);
			BaseName = FName(*StrName);
		}
		else
		{
			BaseName = Class->GetFName();
		}
	}
	else
	{
		BaseName = InBaseName;
	}

	FName TestName;
	UObject* ExistingObject;
	do
	{
		int32 NameNumber = 0;
		NameNumber = UpdateSuffixForNextNewObject(Parent, Class, [](int32& Index) { ++Index; });
		TestName = FName(BaseName, NameNumber);
		ExistingObject = StaticFindObjectFast(NULL, Parent, TestName);
	} while (ExistingObject);

	return TestName;
}

AActor* FEditorUtils::FindOuterActor(const UObject* InObject)
{
	for (UObject* Outer = InObject->GetOuter(); Outer != nullptr; Outer = Outer->GetOuter())
	{
		if (AActor* Actor = Cast<AActor>(Outer))
		{
			return Actor;
		}
		if (UActorComponent* Component = Cast<UActorComponent>(Outer))
		{
			return Component->GetOwner();
		}
	}
	return nullptr;
}