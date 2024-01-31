// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "Engine/World.h"
#include "Collision.h"


struct FSodaPhysicsInterface
{
	static UNREALSODA_API bool RaycastSingleScope(const UWorld* World, TArray<struct FHitResult>& OutHit, const TArray<FVector>& Start, const TArray<FVector>& End, ECollisionChannel TraceChannel, const struct FCollisionQueryParams& Params, const struct FCollisionResponseParams& ResponseParams, const struct FCollisionObjectQueryParams& ObjectParams = FCollisionObjectQueryParams::DefaultObjectQueryParam);
	static UNREALSODA_API bool RaycastMultiScope(const UWorld* World, TArray<TArray<struct FHitResult>>& OutHits, const TArray<FVector>& Start, const TArray<FVector>& End, ECollisionChannel TraceChannel, const struct FCollisionQueryParams& Params, const struct FCollisionResponseParams& ResponseParams, const struct FCollisionObjectQueryParams& ObjectParams = FCollisionObjectQueryParams::DefaultObjectQueryParam);

	static UNREALSODA_API bool GeomSweepSingleScope(const UWorld* World, const struct FCollisionShape& CollisionShape, const FQuat& Rot, TArray<struct FHitResult>& OutHits, const TArray<FVector>& Start, const TArray<FVector>& End, ECollisionChannel TraceChannel, const struct FCollisionQueryParams& Params, const struct FCollisionResponseParams& ResponseParams, const struct FCollisionObjectQueryParams& ObjectParams = FCollisionObjectQueryParams::DefaultObjectQueryParam);
	static UNREALSODA_API bool GeomSweepMultiScope(const UWorld* World, const FPhysicsGeometryCollection& InGeom, const FQuat& InGeomRot, TArray<TArray<struct FHitResult>>& OutHits, const TArray<FVector>& Start, const TArray<FVector>& End, ECollisionChannel TraceChannel, const FCollisionQueryParams& Params, const FCollisionResponseParams& ResponseParams, const FCollisionObjectQueryParams& ObjectParams = FCollisionObjectQueryParams::DefaultObjectQueryParam);
	static UNREALSODA_API bool GeomSweepMultiScope(const UWorld* World, const FCollisionShape& InGeom, const FQuat& InGeomRot, TArray<TArray<struct FHitResult>>& OutHits, const TArray<FVector>& Start, const TArray<FVector>& End, ECollisionChannel TraceChannel, const FCollisionQueryParams& Params, const FCollisionResponseParams& ResponseParams, const FCollisionObjectQueryParams& ObjectParams = FCollisionObjectQueryParams::DefaultObjectQueryParam);
};