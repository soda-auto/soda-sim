// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/Misc/SodaPhysicsInterface.h"
#include "Engine/World.h"
#include "CollisionDebugDrawingPublic.h"
#include "Physics/Experimental/PhysScene_Chaos.h"
#include "Physics/PhysicsInterfaceUtils.h"
#include "Runtime/Engine/Private/Collision/CollisionConversions.h"
#include "PhysicsEngine/ClusterUnionComponent.h"
#include "Runtime/Engine/Private/PhysicsEngine/CollisionQueryFilterCallback.h"
#include "PhysicsEngine/ExternalSpatialAccelerationPayload.h"
#include "Runtime/Engine/Private/PhysicsEngine/ScopedSQHitchRepeater.h"
#include "Runtime/Engine/Private/Collision/CollisionDebugDrawing.h"

float DebugLineLifetime_ = 2.f;

//#include "PhysicsEngine/CollisionAnalyzerCapture.h"

#include "Physics/Experimental/ChaosInterfaceWrapper.h"
#include "PBDRigidsSolver.h"

namespace ECAQueryMode
{
	enum Type
	{
		Test,
		Single,
		Multi
	};
}


//CSV_DEFINE_CATEGORY(SceneQuery, false);

enum class ESingleMultiOrTest : uint8
{
	Single,
	Multi,
	Test
};

enum class ESweepOrRay : uint8
{
	Raycast,
	Sweep,
};

struct FGeomSQAdditionalInputs
{
	FGeomSQAdditionalInputs(const FCollisionShape& InCollisionShape, const FQuat& InGeomRot)
		: ShapeAdapter(InGeomRot, InCollisionShape)
		, CollisionShape(InCollisionShape)
	{
	}

	const FPhysicsGeometry* GetGeometry() const
	{
		return &ShapeAdapter.GetGeometry();
	}

	const FQuat* GetGeometryOrientation() const
	{
		return &ShapeAdapter.GetGeomOrientation();
	}


	const FCollisionShape* GetCollisionShape() const
	{
		return &CollisionShape;
	}

	FPhysicsShapeAdapter ShapeAdapter;
	const FCollisionShape& CollisionShape;
};

struct FGeomCollectionSQAdditionalInputs
{
	FGeomCollectionSQAdditionalInputs(const FPhysicsGeometryCollection& InCollection, const FQuat& InGeomRot)
		: Collection(InCollection)
		, GeomRot(InGeomRot)
	{
	}

	const FPhysicsGeometry* GetGeometry() const
	{
		return &Collection.GetGeometry();
	}

	const FQuat* GetGeometryOrientation() const
	{
		return &GeomRot;
	}

	const FPhysicsGeometryCollection* GetCollisionShape() const
	{
		return &Collection;
	}

	const FPhysicsGeometryCollection& Collection;
	const FQuat& GeomRot;
};

struct FPhysicsGeometrySQAdditionalInputs
{
	FPhysicsGeometrySQAdditionalInputs(const FPhysicsGeometry& InGeometry, const FQuat& InGeomRot)
		: Collection(FChaosEngineInterface::GetGeometryCollection(InGeometry))
		, GeomRot(InGeomRot)
	{
	}

	const FPhysicsGeometry* GetGeometry() const
	{
		return &Collection.GetGeometry();
	}

	const FQuat* GetGeometryOrientation() const
	{
		return &GeomRot;
	}

	const FPhysicsGeometryCollection* GetCollisionShape() const
	{
		return &Collection;
	}
private:
	const FPhysicsGeometryCollection Collection;
	const FQuat& GeomRot;
};

struct FRaycastSQAdditionalInputs
{
	const FPhysicsGeometry* GetGeometry() const
	{
		return nullptr;
	}

	const FQuat* GetGeometryOrientation() const
	{
		return nullptr;
	}

	const FCollisionShape* GetCollisionShape() const
	{
		return nullptr;
	}
};


template<typename TGeom>
using TGeomSQInputs = std::conditional_t<std::is_same_v<TGeom, FPhysicsGeometryCollection>, FGeomCollectionSQAdditionalInputs,
	std::conditional_t<std::is_same_v<TGeom, FCollisionShape>, FGeomSQAdditionalInputs,
	std::conditional_t<std::is_same_v<TGeom, FPhysicsGeometry>, FPhysicsGeometrySQAdditionalInputs,
	void>>>;

template<typename TGeom>
TGeomSQInputs<TGeom> GeomToSQInputs(const TGeom& Geom, const FQuat& Rot)
{
	static_assert(!std::is_same_v<TGeomSQInputs<TGeom>, void>, "Invalid geometry passed to SQ.");
	if constexpr (std::is_same_v<TGeom, FPhysicsGeometryCollection>)
	{
		return FGeomCollectionSQAdditionalInputs(Geom, Rot);
	}
	else if constexpr (std::is_same_v<TGeom, FPhysicsGeometry>)
	{
		return FPhysicsGeometrySQAdditionalInputs(Geom, Rot);
	}
	else if constexpr (std::is_same_v<TGeom, FCollisionShape>)
	{
		return FGeomSQAdditionalInputs(Geom, Rot);
	}
}

template <typename InHitType, ESweepOrRay InGeometryQuery, ESingleMultiOrTest InSingleMultiOrTest>
struct TSQTraits
{
	static const ESingleMultiOrTest SingleMultiOrTest = InSingleMultiOrTest;
	static const ESweepOrRay GeometryQuery = InGeometryQuery;
	using THitType = InHitType;
	using TOutHits = typename TChooseClass<InSingleMultiOrTest == ESingleMultiOrTest::Multi, TArray<FHitResult>, FHitResult>::Result;
	using THitBuffer = typename TChooseClass<InSingleMultiOrTest == ESingleMultiOrTest::Multi, FDynamicHitBuffer<InHitType>, FSingleHitBuffer<InHitType>>::Result;

	// GetNumHits - multi
	template <ESingleMultiOrTest T = SingleMultiOrTest>
	static typename TEnableIf<T == ESingleMultiOrTest::Multi, int32>::Type GetNumHits(const THitBuffer& HitBuffer)
	{
		return HitBuffer.GetNumHits();
	}

	// GetNumHits - single/test
	template <ESingleMultiOrTest T = SingleMultiOrTest>
	static typename TEnableIf<T != ESingleMultiOrTest::Multi, int32>::Type GetNumHits(const THitBuffer& HitBuffer)
	{
		return GetHasBlock(HitBuffer) ? 1 : 0;
	}

	//GetHits - multi
	template <ESingleMultiOrTest T = SingleMultiOrTest>
	static typename TEnableIf<T == ESingleMultiOrTest::Multi, THitType*>::Type GetHits(THitBuffer& HitBuffer)
	{
		return HitBuffer.GetHits();
	}

	//GetHits - single/test
	template <ESingleMultiOrTest T = SingleMultiOrTest>
	static typename TEnableIf<T != ESingleMultiOrTest::Multi, THitType*>::Type GetHits(THitBuffer& HitBuffer)
	{
		return GetBlock(HitBuffer);
	}

	//SceneTrace - ray
	template <typename TAccelContainer, typename TGeomInputs, ESweepOrRay T = GeometryQuery>
	static typename TEnableIf<T == ESweepOrRay::Raycast, void>::Type SceneTrace(const TAccelContainer& Container, const TGeomInputs& GeomInputs, const FVector& Dir, float DeltaMag, const FTransform& StartTM, THitBuffer& HitBuffer, EHitFlags OutputFlags, EQueryFlags QueryFlags, const FCollisionFilterData& FilterData, const FCollisionQueryParams& Params, ICollisionQueryFilterCallbackBase* QueryCallback)
	{
		using namespace ChaosInterface;
		FQueryFilterData QueryFilterData = MakeQueryFilterData(FilterData, QueryFlags, Params);
		FQueryDebugParams DebugParams;
#if !(UE_BUILD_TEST || UE_BUILD_SHIPPING)
		DebugParams.bDebugQuery = Params.bDebugQuery;
#endif
		LowLevelRaycast(Container, StartTM.GetLocation(), Dir, DeltaMag, HitBuffer, OutputFlags, QueryFlags, FilterData, QueryFilterData, QueryCallback, DebugParams);	//todo(ocohen): namespace?
	}

	//SceneTrace - sweep
	template <typename TAccelContainer, typename TGeomInputs, ESweepOrRay T = GeometryQuery>
	static typename TEnableIf<T == ESweepOrRay::Sweep, void>::Type SceneTrace(const TAccelContainer& Container, const TGeomInputs& GeomInputs, const FVector& Dir, float DeltaMag, const FTransform& StartTM, THitBuffer& HitBuffer, EHitFlags OutputFlags, EQueryFlags QueryFlags, const FCollisionFilterData& FilterData, const FCollisionQueryParams& Params, ICollisionQueryFilterCallbackBase* QueryCallback)
	{
		using namespace ChaosInterface;
		FQueryFilterData QueryFilterData = MakeQueryFilterData(FilterData, QueryFlags, Params);
		FQueryDebugParams DebugParams;
#if !(UE_BUILD_TEST || UE_BUILD_SHIPPING)
		DebugParams.bDebugQuery = Params.bDebugQuery;
#endif
		LowLevelSweep(Container, *GeomInputs.GetGeometry(), StartTM, Dir, DeltaMag, HitBuffer, OutputFlags, QueryFlags, FilterData, QueryFilterData, QueryCallback, DebugParams);	//todo(ocohen): namespace?
	}

	/*
	static void ResetOutHits(TArray<FHitResult>& OutHits, const FVector& Start, const FVector& End)
	{
		OutHits.Reset();
	}

	static void ResetOutHits(FHitResult& OutHit, const FVector& Start, const FVector& End)
	{
		OutHit = FHitResult();
		OutHit.TraceStart = Start;
		OutHit.TraceEnd = End;
	}
	*/

	static void DrawTraces(const UWorld* World, const FVector& Start, const FVector& End, const FPhysicsGeometry* PGeom, const FQuat* PGeomRot, const TArray<FHitResult>& Hits)
	{
		if (IsRay())
		{
			DrawLineTraces(World, Start, End, Hits, DebugLineLifetime_);
		}
		else
		{
			DrawGeomSweeps(World, Start, End, *PGeom, *PGeomRot, Hits, DebugLineLifetime_);
		}
	}

	static void DrawTraces(const UWorld* World, const FVector& Start, const FVector& End, const FPhysicsGeometry* PGeom, const FQuat* GeomRotation, const FHitResult& Hit)
	{
		TArray<FHitResult> Hits;
		Hits.Add(Hit);

		DrawTraces(World, Start, End, PGeom, GeomRotation, Hits);
	}

	template <typename TGeomInputs>
	static void CaptureTraces(const UWorld* World, const FVector& Start, const FVector& End, const TGeomInputs& GeomInputs, ECollisionChannel TraceChannel, const FCollisionQueryParams& Params, const FCollisionResponseParams& ResponseParams, const FCollisionObjectQueryParams& ObjectParams, const TArray<FHitResult>& Hits, bool bHaveBlockingHit, double StartTime)
	{
#if ENABLE_COLLISION_ANALYZER
		ECAQueryMode::Type QueryMode = IsMulti() ? ECAQueryMode::Multi : (IsSingle() ? ECAQueryMode::Single : ECAQueryMode::Test);
		if (IsRay())
		{
			CAPTURERAYCAST(World, Start, End, QueryMode, TraceChannel, Params, ResponseParams, ObjectParams, Hits);
		}
		else
		{
			CAPTUREGEOMSWEEP(World, Start, End, *GeomInputs.GetGeometryOrientation(), QueryMode, *GeomInputs.GetCollisionShape(), TraceChannel, Params, ResponseParams, ObjectParams, Hits);
		}
#endif
	}

	template <typename TGeomInputs>
	static void CaptureTraces(const UWorld* World, const FVector& Start, const FVector& End, const TGeomInputs& GeomInputs, ECollisionChannel TraceChannel, const FCollisionQueryParams& Params, const FCollisionResponseParams& ResponseParams, const FCollisionObjectQueryParams& ObjectParams, const FHitResult& Hit, bool bHaveBlockingHit, double StartTime)
	{
		TArray<FHitResult> Hits;
		if (bHaveBlockingHit)
		{
			Hits.Add(Hit);
		}
		CaptureTraces(World, Start, End, GeomInputs, TraceChannel, Params, ResponseParams, ObjectParams, Hits, bHaveBlockingHit, StartTime);
	}

	static EHitFlags GetHitFlags()
	{
		if (IsTest())
		{
			return EHitFlags::None;
		}
		else
		{
			if (IsRay())
			{
				return EHitFlags::Position | EHitFlags::Normal | EHitFlags::Distance | EHitFlags::MTD | EHitFlags::FaceIndex;
			}
			else
			{
				if (IsSingle())
				{
					return EHitFlags::Position | EHitFlags::Normal | EHitFlags::Distance | EHitFlags::MTD;
				}
				else
				{
					return EHitFlags::Position | EHitFlags::Normal | EHitFlags::Distance | EHitFlags::MTD | EHitFlags::FaceIndex;
				}
			}
		}
	}

	static EQueryFlags GetQueryFlags()
	{
		if (IsRay())
		{
			return (IsTest() ? (EQueryFlags::PreFilter | EQueryFlags::AnyHit) : EQueryFlags::PreFilter);
		}
		else
		{
			if (IsTest())
			{
				return (EQueryFlags::PreFilter | EQueryFlags::PostFilter | EQueryFlags::AnyHit);
			}
			else if (IsSingle())
			{
				return EQueryFlags::PreFilter;
			}
			else
			{
				return (EQueryFlags::PreFilter | EQueryFlags::PostFilter);
			}
		}
	}

	CA_SUPPRESS(6326);
	constexpr static bool IsSingle() { return SingleMultiOrTest == ESingleMultiOrTest::Single; }

	CA_SUPPRESS(6326);
	constexpr static bool IsTest() { return SingleMultiOrTest == ESingleMultiOrTest::Test; }

	CA_SUPPRESS(6326);
	constexpr static bool IsMulti() { return SingleMultiOrTest == ESingleMultiOrTest::Multi; }

	CA_SUPPRESS(6326);
	constexpr static bool IsRay() { return GeometryQuery == ESweepOrRay::Raycast; }

	CA_SUPPRESS(6326);
	constexpr static bool IsSweep() { return GeometryQuery == ESweepOrRay::Sweep; }

	/** Easy way to query whether this SQ trait is for the GT or the PT based on the based in hit type. */
	constexpr static bool IsExternalData() { return std::is_base_of_v<ChaosInterface::FActorShape, InHitType>; }

};

enum class EThreadQueryContext
{
	GTData,		//use interpolated GT data
	PTDataWithGTObjects,	//use pt data, but convert back to GT when possible
	PTOnlyData,	//use only the PT data and don't try to convert anything back to GT
};

static EThreadQueryContext GetThreadQueryContext(const Chaos::FPhysicsSolver& Solver)
{
	if (Solver.IsGameThreadFrozen())
	{
		//If the game thread is frozen the solver is currently in fixed tick mode (i.e. fixed tick callbacks are being executed on GT)
		if (IsInGameThread() || IsInParallelGameThread())
		{
			//Since we are on GT or parallel GT we must be in fixed tick, so use PT data and convert back to GT where possible
			return EThreadQueryContext::PTDataWithGTObjects;
		}
		else
		{
			//The solver can't be running since it's calling fixed tick callbacks on gt, so it must be an unrelated thread task (audio, animation, etc...) so just use interpolated gt data
			return EThreadQueryContext::GTData;
		}
	}
	else
	{
		//TODO: need a way to know we are on a physics thread task (this isn't supported yet)
		//For now just use interpolated data
		return EThreadQueryContext::GTData;
	}
}

struct FClusterUnionHit
{
	bool bIsClusterUnion = false;
	bool bHit = false;
};

struct FDefaultAccelContainer
{
	static constexpr bool HasAccelerationStructureOverride()
	{
		return false;
	}
};

template<typename TAccel>
struct FOverrideAccelContainer
{
	explicit FOverrideAccelContainer(const TAccel& InSpatialAcceleration)
		: SpatialAcceleration(InSpatialAcceleration)
	{}

	static constexpr bool HasAccelerationStructureOverride()
	{
		return true;
	}

	const TAccel& GetSpatialAcceleration() const
	{
		return SpatialAcceleration;
	}

private:
	const TAccel& SpatialAcceleration;
};

template <typename Traits, typename TGeomInputs, typename TAccelContainer>
bool TSceneCastCommonImp(const UWorld* World, TArray<typename Traits::TOutHits>& OutHits, const TGeomInputs& GeomInputs, const TArray<FVector>& Start, const TArray<FVector>& End, ECollisionChannel TraceChannel, const struct FCollisionQueryParams& Params, const struct FCollisionResponseParams& ResponseParams, const struct FCollisionObjectQueryParams& ObjectParams, const TAccelContainer& AccelContainer)
{
	using namespace ChaosInterface;

	FScopeCycleCounter Counter(Params.StatId);
	//STARTQUERYTIMER();

	if (!Traits::IsTest())
	{
		//Traits::ResetOutHits(OutHits, Start, End);
		OutHits.Reset();
	}

	const int Num = FMath::Min(Start.Num(), End.Num());
	OutHits.SetNum(Num);

	// Track if we get any 'blocking' hits
	bool bHaveBlockingHit = false;

	// Enable scene locks, in case they are required
	FPhysScene& PhysScene = *World->GetPhysicsScene();

	FScopedSceneReadLock SceneLocks(PhysScene);

	ParallelFor(Num, [&](int i)
	{
		const FVector& StartRef = Start[i];
		const FVector& EndtRef = End[i];

		FVector Delta = EndtRef - StartRef;
		float DeltaSize = Delta.Size();
		float DeltaMag = FMath::IsNearlyZero(DeltaSize) ? 0.f : DeltaSize;
		float MinBlockingDistance = DeltaMag;

		if (Traits::IsSweep() || DeltaMag > 0.f)
		{
			// Create filter data used to filter collisions 
			CA_SUPPRESS(6326);
			FCollisionFilterData Filter = CreateQueryFilterData(TraceChannel, Params.bTraceComplex, ResponseParams.CollisionResponse, Params, ObjectParams, Traits::SingleMultiOrTest == ESingleMultiOrTest::Multi);

			CA_SUPPRESS(6326);
			FCollisionQueryFilterCallback QueryCallback(Params, Traits::GeometryQuery == ESweepOrRay::Sweep);

			CA_SUPPRESS(6326);
			if (Traits::SingleMultiOrTest != ESingleMultiOrTest::Multi)
			{
				QueryCallback.bIgnoreTouches = true;
			}

			typename Traits::THitBuffer HitBufferSync;

			bool bBlockingHit = false;
			const FVector Dir = DeltaMag > 0.f ? (Delta / DeltaMag) : FVector(1, 0, 0);
			const FTransform StartTM = Traits::IsRay() ? FTransform(StartRef) : FTransform(*GeomInputs.GetGeometryOrientation(), StartRef);

			{
				FScopedSQHitchRepeater<decltype(HitBufferSync)> HitchRepeater(HitBufferSync, QueryCallback, FHitchDetectionInfo(StartRef, EndtRef, TraceChannel, Params));
				do
				{
					if constexpr (TAccelContainer::HasAccelerationStructureOverride())
					{
						Traits::SceneTrace(AccelContainer.GetSpatialAcceleration(), GeomInputs, Dir, DeltaMag, StartTM, HitchRepeater.GetBuffer(), Traits::GetHitFlags(), Traits::GetQueryFlags(), Filter, Params, &QueryCallback);
					}
					else
					{
						Traits::SceneTrace(PhysScene, GeomInputs, Dir, DeltaMag, StartTM, HitchRepeater.GetBuffer(), Traits::GetHitFlags(), Traits::GetQueryFlags(), Filter, Params, &QueryCallback);
					}
				} while (HitchRepeater.RepeatOnHitch());
			}


			const int32 NumHits = Traits::GetNumHits(HitBufferSync);

			if (NumHits > 0 && GetHasBlock(HitBufferSync))
			{
				bBlockingHit = true;
				MinBlockingDistance = GetDistance(Traits::GetHits(HitBufferSync)[NumHits - 1]);
			}

			if (NumHits > 0 && !Traits::IsTest())
			{
				bool bSuccess = ConvertTraceResults(bBlockingHit, World, NumHits, Traits::GetHits(HitBufferSync), DeltaMag, Filter, OutHits[i], StartRef, EndtRef, GeomInputs.GetGeometry(), StartTM, MinBlockingDistance, Params.bReturnFaceIndex, Params.bReturnPhysicalMaterial) == EConvertQueryResult::Valid;

				if (!bSuccess)
				{
					// We don't need to change bBlockingHit, that's done by ConvertTraceResults if it removed the blocking hit.
					UE_LOG(LogCollision, Error, TEXT("%s%s resulted in a NaN/INF in PHit!"), Traits::IsRay() ? TEXT("Raycast") : TEXT("Sweep"), Traits::IsMulti() ? TEXT("Multi") : (Traits::IsSingle() ? TEXT("Single") : TEXT("Test")));
	#if ENABLE_NAN_DIAGNOSTIC
					UE_LOG(LogCollision, Error, TEXT("--------TraceChannel : %d"), (int32)TraceChannel);
					UE_LOG(LogCollision, Error, TEXT("--------Start : %s"), *StartRef.ToString());
					UE_LOG(LogCollision, Error, TEXT("--------End : %s"), *EndtRef.ToString());
					if (Traits::IsSweep())
					{
						UE_LOG(LogCollision, Error, TEXT("--------GeomRotation : %s"), *(GeomInputs.GetGeometryOrientation()->ToString()));
					}
					UE_LOG(LogCollision, Error, TEXT("--------%s"), *Params.ToString());
	#endif
				}

				bHaveBlockingHit = bBlockingHit;
			}
		}
	});


	return bHaveBlockingHit;
}

template <typename Traits, typename PTTraits, typename TGeomInputs, typename TAccelContainer = FDefaultAccelContainer>
bool TSceneCastCommon(const UWorld* World, TArray<typename Traits::TOutHits> & OutHits, const TGeomInputs& GeomInputs, const TArray<FVector>& Start, const TArray<FVector>& End, ECollisionChannel TraceChannel, const struct FCollisionQueryParams& Params, const struct FCollisionResponseParams& ResponseParams, const struct FCollisionObjectQueryParams& ObjectParams, const TAccelContainer& AccelContainer = FDefaultAccelContainer{})
{
	if ((World == NULL) || (World->GetPhysicsScene() == NULL))
	{
		return false;
	}

	const EThreadQueryContext ThreadContext = GetThreadQueryContext(*World->GetPhysicsScene()->GetSolver());
	if (ThreadContext == EThreadQueryContext::GTData)
	{
		return TSceneCastCommonImp<Traits, TGeomInputs>(World, OutHits, GeomInputs, Start, End, TraceChannel, Params, ResponseParams, ObjectParams, AccelContainer);
	}
	else
	{
		return TSceneCastCommonImp<PTTraits, TGeomInputs>(World, OutHits, GeomInputs, Start, End, TraceChannel, Params, ResponseParams, ObjectParams, AccelContainer);
	}
}

//////////////////////////////////////////////////////////////////////////
// RAYCAST

bool FSodaPhysicsInterface::RaycastSingleScope(const UWorld* World, TArray<struct FHitResult>& OutHits, const TArray<FVector> & Start, const TArray<FVector> & End, ECollisionChannel TraceChannel, const struct FCollisionQueryParams& Params, const struct FCollisionResponseParams& ResponseParams, const struct FCollisionObjectQueryParams& ObjectParams)
{
	using namespace ChaosInterface;

	using TCastTraits = TSQTraits<FRaycastHit, ESweepOrRay::Raycast, ESingleMultiOrTest::Single>;
	using TPTCastTraits = TSQTraits<FPTRaycastHit, ESweepOrRay::Raycast, ESingleMultiOrTest::Single>;
	return TSceneCastCommon<TCastTraits, TPTCastTraits>(World, OutHits, FRaycastSQAdditionalInputs(), Start, End, TraceChannel, Params, ResponseParams, ObjectParams);
}



bool FSodaPhysicsInterface::RaycastMultiScope(const UWorld* World, TArray<TArray<struct FHitResult>>& OutHits, const TArray<FVector>& Start, const TArray<FVector>& End, ECollisionChannel TraceChannel, const struct FCollisionQueryParams& Params, const struct FCollisionResponseParams& ResponseParams, const struct FCollisionObjectQueryParams& ObjectParams)
{
	using namespace ChaosInterface;

	using TCastTraits = TSQTraits<FHitRaycast, ESweepOrRay::Raycast, ESingleMultiOrTest::Multi>;
	using TPTCastTraits = TSQTraits<FPTRaycastHit, ESweepOrRay::Raycast, ESingleMultiOrTest::Multi>;
	return TSceneCastCommon<TCastTraits, TPTCastTraits>(World, OutHits, FRaycastSQAdditionalInputs(), Start, End, TraceChannel, Params, ResponseParams, ObjectParams);
}

//////////////////////////////////////////////////////////////////////////
// GEOM SWEEP


bool FSodaPhysicsInterface::GeomSweepSingleScope(const UWorld* World, const struct FCollisionShape& CollisionShape, const FQuat& Rot, TArray<struct FHitResult>& OutHits, const TArray<FVector>& Start, const TArray<FVector>& End, ECollisionChannel TraceChannel, const struct FCollisionQueryParams& Params, const struct FCollisionResponseParams& ResponseParams, const struct FCollisionObjectQueryParams& ObjectParams)
{
	using namespace ChaosInterface;

	using TCastTraits = TSQTraits<FSweepHit, ESweepOrRay::Sweep, ESingleMultiOrTest::Single>;
	using TPTCastTraits = TSQTraits<FPTSweepHit, ESweepOrRay::Sweep, ESingleMultiOrTest::Single>;
	return TSceneCastCommon<TCastTraits, TPTCastTraits>(World, OutHits, FGeomSQAdditionalInputs(CollisionShape, Rot), Start, End, TraceChannel, Params, ResponseParams, ObjectParams);
}

bool FSodaPhysicsInterface::GeomSweepMultiScope(const UWorld* World, const FPhysicsGeometryCollection& InGeom, const FQuat& InGeomRot, TArray<TArray<FHitResult>>& OutHits, const TArray<FVector>& Start, const TArray<FVector>& End, ECollisionChannel TraceChannel, const FCollisionQueryParams& Params, const FCollisionResponseParams& ResponseParams, const FCollisionObjectQueryParams& ObjectParams)
{
	using namespace ChaosInterface;

	using TCastTraits = TSQTraits<FHitSweep, ESweepOrRay::Sweep, ESingleMultiOrTest::Multi>;
	using TPTCastTraits = TSQTraits<FPTSweepHit, ESweepOrRay::Sweep, ESingleMultiOrTest::Multi>;
	return TSceneCastCommon<TCastTraits, TPTCastTraits>(World, OutHits, FGeomCollectionSQAdditionalInputs(InGeom, InGeomRot), Start, End, TraceChannel, Params, ResponseParams, ObjectParams);
}


bool FSodaPhysicsInterface::GeomSweepMultiScope(const UWorld* World, const FCollisionShape& InGeom, const FQuat& InGeomRot, TArray<TArray<FHitResult>>& OutHits, const TArray<FVector>& Start, const TArray<FVector>& End, ECollisionChannel TraceChannel, const FCollisionQueryParams& Params, const FCollisionResponseParams& ResponseParams, const FCollisionObjectQueryParams& ObjectParams)
{
	using namespace ChaosInterface;

	using TCastTraits = TSQTraits<FHitSweep, ESweepOrRay::Sweep, ESingleMultiOrTest::Multi>;
	using TPTCastTraits = TSQTraits<FPTSweepHit, ESweepOrRay::Sweep, ESingleMultiOrTest::Multi>;
	return TSceneCastCommon<TCastTraits, TPTCastTraits>(World, OutHits, FGeomSQAdditionalInputs(InGeom, InGeomRot), Start, End, TraceChannel, Params, ResponseParams, ObjectParams);
}
