// Copyright Epic Games, Inc. All Rights Reserved.
// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#if(!IS_MONOLITHIC)

#include "Physics/Experimental/ChaosInterfaceWrapper.h"
#include "SQAccelerator.h"
#include "SQVisitor.h"

#include "PhysTestSerializer.h"

#include "PBDRigidsSolver.h"
#include "Physics/Experimental/PhysScene_Chaos.h"
#include "PhysicsEngine/ExternalSpatialAccelerationPayload.h"

int32 ForceStandardSQ = 0;
FAutoConsoleVariableRef CVarForceStandardSQ(TEXT("p.ForceStandardSQ"), ForceStandardSQ, TEXT("If enabled, we force the standard scene query even if custom SQ structure is enabled"));


#if !UE_BUILD_SHIPPING
int32 SerializeSQs = 0;
int32 SerializeSQSamples = 100;
int32 SerializeBadSQs = 0;
int32 ReplaySQs = 0;
int32 EnableRaycastSQCapture = 1;
int32 EnableOverlapSQCapture = 1;
int32 EnableSweepSQCapture = 1;

FAutoConsoleVariableRef CVarSerializeSQs(TEXT("p.SerializeSQs"), SerializeSQs, TEXT("If enabled, we create a sq capture per sq that takes more than provided value in microseconds. This can be very expensive as the entire scene is saved out"));
FAutoConsoleVariableRef CVarSerializeSQSamples(TEXT("p.SerializeSQSampleCount"), SerializeSQSamples, TEXT("If Query exceeds duration threshold, we will re-measure SQ this many times before serializing. Larger values cause hitching."));
FAutoConsoleVariableRef CVarReplaySweeps(TEXT("p.ReplaySQs"), ReplaySQs, TEXT("If enabled, we rerun the sq against chaos"));
FAutoConsoleVariableRef CVarSerializeBadSweeps(TEXT("p.SerializeBadSQs"), SerializeBadSQs, TEXT("If enabled, we create a sq capture whenever chaos and physx diverge"));
FAutoConsoleVariableRef CVarSerializeSQsRaycastEnabled(TEXT("p.SerializeSQsRaycastEnabled"), EnableRaycastSQCapture, TEXT("If disabled, p.SerializeSQs will not consider raycasts"));
FAutoConsoleVariableRef CVarSerializeSQsOverlapEnabled(TEXT("p.SerializeSQsOverlapEnabled"), EnableOverlapSQCapture, TEXT("If disabled, p.SerializeSQs will not consider overlaps"));
FAutoConsoleVariableRef CVarSerializeSQsSweepEnabled(TEXT("p.SerializeSQsSweepEnabled"), EnableSweepSQCapture, TEXT("If disabled, p.SerializeSQs will not consider sweeps"));
#else
constexpr int32 SerializeSQs = 0;
constexpr int32 ReplaySQs = 0;
constexpr int32 SerializeSQSamples = 0;
constexpr int32 EnableRaycastSQCapture = 0;
constexpr int32 EnableOverlapSQCapture = 0;
constexpr int32 EnableSweepSQCapture = 0;
#endif

namespace
{
	void FinalizeCapture(FPhysTestSerializer& Serializer)
	{
#if !UE_BUILD_SHIPPING
		if (SerializeSQs)
		{
			Serializer.Serialize(TEXT("SQCapture"));
		}
#if 0
		if (ReplaySQs)
		{
			const bool bReplaySuccess = SQComparisonHelper(Serializer);
			if (!bReplaySuccess)
			{
				UE_LOG(LogPhysicsCore, Warning, TEXT("Chaos SQ does not match physx"));
				if (SerializeBadSQs && !SerializeSQs)
				{
					Serializer.Serialize(TEXT("BadSQCapture"));
				}
			}
		}
#endif
#endif
	}

	/**
	 * This is meant as a replacement for FChaosSQAccelerator. There doesn't seem to be a need to expose this publicly since it'll just be used internally to allow
	 * us to use more than one type of acceleration structure payload.
	 */
	template<typename TPayload>
	class FGenericChaosSQAccelerator
	{
	public:
		explicit FGenericChaosSQAccelerator(const Chaos::ISpatialAcceleration<TPayload, Chaos::FReal, 3>& InSpatialAcceleration)
			: SpatialAcceleration(InSpatialAcceleration)
		{}

		template<typename THit>
		void Raycast(const FVector& Start, const FVector& Dir, const float DeltaMagnitude, ChaosInterface::FSQHitBuffer<THit>& HitBuffer, EHitFlags OutputFlags, const ChaosInterface::FQueryFilterData& QueryFilterData, ICollisionQueryFilterCallbackBase& QueryCallback, const ChaosInterface::FQueryDebugParams& DebugParams = {}) const
		{
			using namespace Chaos;
			using namespace ChaosInterface;

			TSQVisitor<TSphere<FReal, 3>, TPayload, THit, std::is_same_v<THit, FRaycastHit>> RaycastVisitor(Start, Dir, HitBuffer, OutputFlags, QueryFilterData, QueryCallback, DebugParams);
			HitBuffer.IncFlushCount();
			SpatialAcceleration.Raycast(Start, Dir, DeltaMagnitude, RaycastVisitor);
			HitBuffer.DecFlushCount();
		}

		template<typename THit>
		void Sweep(const Chaos::FImplicitObject& QueryGeom, const FTransform& StartTM, const FVector& Dir, const float DeltaMagnitude, ChaosInterface::FSQHitBuffer<THit>& HitBuffer, EHitFlags OutputFlags, const ChaosInterface::FQueryFilterData& QueryFilterData, ICollisionQueryFilterCallbackBase& QueryCallback, const ChaosInterface::FQueryDebugParams& DebugParams = {}) const
		{
			return Chaos::Utilities::CastHelper(QueryGeom, StartTM, [&](const auto& Downcast, const FTransform& StartFullTM)
				{
					return SweepHelper(Downcast, SpatialAcceleration, StartFullTM, Dir, DeltaMagnitude, HitBuffer, OutputFlags, QueryFilterData, QueryCallback, DebugParams);
				});
		}

		template<typename THit>
		void Overlap(const Chaos::FImplicitObject& QueryGeom, const FTransform& GeomPose, ChaosInterface::FSQHitBuffer<THit>& HitBuffer, const ChaosInterface::FQueryFilterData& QueryFilterData, ICollisionQueryFilterCallbackBase& QueryCallback, const ChaosInterface::FQueryDebugParams& DebugParams = {}) const
		{
			return Chaos::Utilities::CastHelper(QueryGeom, GeomPose, [&](const auto& Downcast, const FTransform& GeomFullPose)
				{
					return OverlapHelper(Downcast, SpatialAcceleration, GeomFullPose, HitBuffer, QueryFilterData, QueryCallback, DebugParams);
				});
		}

	private:
		const Chaos::ISpatialAcceleration<TPayload, Chaos::FReal, 3>& SpatialAcceleration;
	};

	template<typename TPayload>
	void SweepSQCaptureHelper(float QueryDurationSeconds, const FGenericChaosSQAccelerator<TPayload>& SQAccelerator, const FPhysScene& Scene, const FPhysicsGeometry& QueryGeom, const FTransform& StartTM, const FVector& Dir, float DeltaMag, const FPhysicsHitCallback<FHitSweep>& HitBuffer, EHitFlags OutputFlags, FQueryFlags QueryFlags, const FCollisionFilterData& Filter, const ChaosInterface::FQueryFilterData& QueryFilterData, ICollisionQueryFilterCallbackBase* QueryCallback, const ChaosInterface::FQueryDebugParams& DebugParams)
	{
#if !UE_BUILD_SHIPPING
		float QueryDurationMicro = QueryDurationSeconds * 1000.0 * 1000.0;
		if (((SerializeSQs && QueryDurationMicro > SerializeSQs)) && IsInGameThread())
		{
			// Measure average time of query over multiple samples to reduce fluke from context switches or that kind of thing.
			uint32 Cycles = 0.0;
			const uint32 SampleCount = SerializeSQSamples;
			for (uint32 Samples = 0; Samples < SampleCount; ++Samples)
			{
				// Reset output to not skew times with large buffer
				FPhysicsHitCallback<FHitSweep> ScratchHitBuffer = FPhysicsHitCallback<FHitSweep>(HitBuffer.WantsSingleResult());

				uint32 StartTime = FPlatformTime::Cycles();
				SQAccelerator.Sweep(QueryGeom, StartTM, Dir, DeltaMag, ScratchHitBuffer, OutputFlags, QueryFilterData, *QueryCallback, DebugParams);
				Cycles += FPlatformTime::Cycles() - StartTime;
			}

			float Milliseconds = FPlatformTime::ToMilliseconds(Cycles);
			float AvgMicroseconds = (Milliseconds * 1000) / SampleCount;

			if (AvgMicroseconds > SerializeSQs)
			{
				FPhysTestSerializer Serializer;
				Serializer.SetPhysicsData(*Scene.GetSolver()->GetEvolution());
				FSQCapture& SweepCapture = Serializer.CaptureSQ();
				SweepCapture.StartCaptureChaosSweep(*Scene.GetSolver()->GetEvolution(), QueryGeom, StartTM, Dir, DeltaMag, OutputFlags, QueryFilterData, Filter, *QueryCallback);
				SweepCapture.EndCaptureChaosSweep(HitBuffer);

				FinalizeCapture(Serializer);
			}
		}
#endif
	}

	template<typename TPayload>
	void RaycastSQCaptureHelper(float QueryDurationSeconds, const FGenericChaosSQAccelerator<TPayload>& SQAccelerator, const FPhysScene& Scene, const FVector& Start, const FVector& Dir, float DeltaMag, const FPhysicsHitCallback<FHitRaycast>& HitBuffer, EHitFlags OutputFlags, FQueryFlags QueryFlags, const FCollisionFilterData& Filter, const ChaosInterface::FQueryFilterData& QueryFilterData, ICollisionQueryFilterCallbackBase* QueryCallback, const ChaosInterface::FQueryDebugParams& DebugParams)
	{
#if !UE_BUILD_SHIPPING
		float QueryDurationMicro = QueryDurationSeconds * 1000.0 * 1000.0;
		if (((!!SerializeSQs && QueryDurationMicro > SerializeSQs)) && IsInGameThread())
		{
			// Measure average time of query over multiple samples to reduce fluke from context switches or that kind of thing.
			uint32 Cycles = 0.0;
			const uint32 SampleCount = SerializeSQSamples;
			for (uint32 Samples = 0; Samples < SampleCount; ++Samples)
			{
				// Reset output to not skew times with large buffer
				FPhysicsHitCallback<FHitRaycast> ScratchHitBuffer = FPhysicsHitCallback<FHitRaycast>(HitBuffer.WantsSingleResult());

				uint32 StartTime = FPlatformTime::Cycles();
				SQAccelerator.Raycast(Start, Dir, DeltaMag, ScratchHitBuffer, OutputFlags, QueryFilterData, *QueryCallback, DebugParams);
				Cycles += FPlatformTime::Cycles() - StartTime;
			}

			float Milliseconds = FPlatformTime::ToMilliseconds(Cycles);
			float AvgMicroseconds = (Milliseconds * 1000) / SampleCount;

			if (AvgMicroseconds > SerializeSQs)
			{
				FPhysTestSerializer Serializer;
				Serializer.SetPhysicsData(*Scene.GetSolver()->GetEvolution());
				FSQCapture& RaycastCapture = Serializer.CaptureSQ();
				RaycastCapture.StartCaptureChaosRaycast(*Scene.GetSolver()->GetEvolution(), Start, Dir, DeltaMag, OutputFlags, QueryFilterData, Filter, *QueryCallback);
				RaycastCapture.EndCaptureChaosRaycast(HitBuffer);

				FinalizeCapture(Serializer);
			}
		}
#endif
	}

	template<typename TPayload>
	void OverlapSQCaptureHelper(float QueryDurationSeconds, const FGenericChaosSQAccelerator<TPayload>& SQAccelerator, const FPhysScene& Scene, const FPhysicsGeometry& QueryGeom, const FTransform& GeomPose, const FPhysicsHitCallback<FHitOverlap>& HitBuffer, FQueryFlags QueryFlags, const FCollisionFilterData& Filter, const ChaosInterface::FQueryFilterData& QueryFilterData, ICollisionQueryFilterCallbackBase* QueryCallback, const ChaosInterface::FQueryDebugParams& DebugParams)
	{
#if !UE_BUILD_SHIPPING
		float QueryDurationMicro = QueryDurationSeconds * 1000.0 * 1000.0;
		if (((!!SerializeSQs && QueryDurationMicro > SerializeSQs)) && IsInGameThread())
		{
			// Measure average time of query over multiple samples to reduce fluke from context switches or that kind of thing.
			uint32 Cycles = 0.0;
			const uint32 SampleCount = SerializeSQSamples;
			for (uint32 Samples = 0; Samples < SampleCount; ++Samples)
			{
				// Reset output to not skew times with large buffer
				FPhysicsHitCallback<FHitOverlap> ScratchHitBuffer = FPhysicsHitCallback<FHitOverlap>(HitBuffer.WantsSingleResult());

				uint32 StartTime = FPlatformTime::Cycles();
				SQAccelerator.Overlap(QueryGeom, GeomPose, ScratchHitBuffer, QueryFilterData, *QueryCallback);
				Cycles += FPlatformTime::Cycles() - StartTime;
			}

			float Milliseconds = FPlatformTime::ToMilliseconds(Cycles);
			float AvgMicroseconds = (Milliseconds * 1000) / SampleCount;

			if (AvgMicroseconds > SerializeSQs)
			{
				FPhysTestSerializer Serializer;
				Serializer.SetPhysicsData(*Scene.GetSolver()->GetEvolution());
				FSQCapture& OverlapCapture = Serializer.CaptureSQ();
				OverlapCapture.StartCaptureChaosOverlap(*Scene.GetSolver()->GetEvolution(), QueryGeom, GeomPose, QueryFilterData, Filter, *QueryCallback);
				OverlapCapture.EndCaptureChaosOverlap(HitBuffer);

				FinalizeCapture(Serializer);
			}
		}
#endif
	}

	template<typename TAccelContainer, bool bGTData>
	struct FAccelerationContainerTraits
	{
		using TAccelStructure = std::conditional_t<std::is_same_v<TAccelContainer, FPhysScene>, Chaos::IDefaultChaosSpatialAcceleration, TAccelContainer>;

		static const TAccelStructure* GetSpatialAccelerationFromContainer(const TAccelContainer& Container)
		{
			if constexpr (IsPhysScene())
			{
				if constexpr (bGTData)
				{
					return Container.GetSpacialAcceleration();
				}
				else
				{
					return Container.GetSolver()->GetInternalAccelerationStructure_Internal();
				}
			}
			else
			{
				return &Container;
			}

			return nullptr;
		}

		static constexpr bool IsPhysScene()
		{
			return std::is_same_v<TAccelContainer, FPhysScene>;
		}
	};
				}

#define DEFINE_LOW_LEVEL_RAYCAST_ACCEL_HIT(TACCEL, THIT) template void LowLevelRaycast<TACCEL, THIT>(const TACCEL& Container, const FVector& Start, const FVector& Dir, float DeltaMag, FPhysicsHitCallback<THIT>& HitBuffer, EHitFlags OutputFlags, FQueryFlags QueryFlags, const FCollisionFilterData& Filter, const ChaosInterface::FQueryFilterData& QueryFilterData, ICollisionQueryFilterCallbackBase* QueryCallback, const ChaosInterface::FQueryDebugParams& DebugParams);
#define DEFINE_LOW_LEVEL_RAYCAST_ACCEL(TACCEL) \
DEFINE_LOW_LEVEL_RAYCAST_ACCEL_HIT(TACCEL, FHitRaycast); \
DEFINE_LOW_LEVEL_RAYCAST_ACCEL_HIT(TACCEL, ChaosInterface::FPTRaycastHit);

template <typename TAccelContainer, typename THitRaycast>
void LowLevelRaycast(const TAccelContainer& Container, const FVector& Start, const FVector& Dir, float DeltaMag, FPhysicsHitCallback<THitRaycast>& HitBuffer, EHitFlags OutputFlags, FQueryFlags QueryFlags, const FCollisionFilterData& Filter, const ChaosInterface::FQueryFilterData& QueryFilterData, ICollisionQueryFilterCallbackBase* QueryCallback, const ChaosInterface::FQueryDebugParams& DebugParams)
{
	constexpr bool bGTData = std::is_same_v<THitRaycast, FHitRaycast>;
	using TTraits = FAccelerationContainerTraits<TAccelContainer, bGTData>;
	if (const typename TTraits::TAccelStructure* SolverAccelerationStructure = TTraits::GetSpatialAccelerationFromContainer(Container))
	{
		FGenericChaosSQAccelerator<typename TTraits::TAccelStructure::TPayload> SQAccelerator(*SolverAccelerationStructure);
		double Time = 0.0;
		{
			FScopedDurationTimer Timer(Time);
			SQAccelerator.Raycast(Start, Dir, DeltaMag, HitBuffer, OutputFlags, QueryFilterData, *QueryCallback, DebugParams);
		}

		if constexpr (bGTData && TTraits::IsPhysScene())
		{
			if (!!SerializeSQs && !!EnableRaycastSQCapture)
			{
				RaycastSQCaptureHelper(Time, SQAccelerator, Container, Start, Dir, DeltaMag, HitBuffer, OutputFlags, QueryFlags, Filter, QueryFilterData, QueryCallback, DebugParams);
			}
		}
	}
}

DEFINE_LOW_LEVEL_RAYCAST_ACCEL(Chaos::IDefaultChaosSpatialAcceleration)
DEFINE_LOW_LEVEL_RAYCAST_ACCEL(IExternalSpatialAcceleration)
DEFINE_LOW_LEVEL_RAYCAST_ACCEL(FPhysScene)

#define DEFINE_LOW_LEVEL_SWEEP_ACCEL_HIT(TACCEL, THIT) template void LowLevelSweep<TACCEL, THIT>(const TACCEL& Container, const FPhysicsGeometry& QueryGeom, const FTransform& StartTM, const FVector& Dir, float DeltaMag, FPhysicsHitCallback<THIT>& HitBuffer, EHitFlags OutputFlags, FQueryFlags QueryFlags, const FCollisionFilterData& Filter, const ChaosInterface::FQueryFilterData& QueryFilterData, ICollisionQueryFilterCallbackBase* QueryCallback, const ChaosInterface::FQueryDebugParams& DebugParams)
#define DEFINE_LOW_LEVEL_SWEEP_ACCEL(TACCEL) \
DEFINE_LOW_LEVEL_SWEEP_ACCEL_HIT(TACCEL, FHitSweep); \
DEFINE_LOW_LEVEL_SWEEP_ACCEL_HIT(TACCEL, ChaosInterface::FPTSweepHit);

template <typename TAccelContainer, typename THitSweep>
void LowLevelSweep(const TAccelContainer& Container, const FPhysicsGeometry& QueryGeom, const FTransform& StartTM, const FVector& Dir, float DeltaMag, FPhysicsHitCallback<THitSweep>& HitBuffer, EHitFlags OutputFlags, FQueryFlags QueryFlags, const FCollisionFilterData& Filter, const ChaosInterface::FQueryFilterData& QueryFilterData, ICollisionQueryFilterCallbackBase* QueryCallback, const ChaosInterface::FQueryDebugParams& DebugParams)
{
	constexpr bool bGTData = std::is_same<THitSweep, FHitSweep>::value;
	using TTraits = FAccelerationContainerTraits<TAccelContainer, bGTData>;
	if (const typename TTraits::TAccelStructure* SolverAccelerationStructure = TTraits::GetSpatialAccelerationFromContainer(Container))
	{
		FGenericChaosSQAccelerator<typename TTraits::TAccelStructure::TPayload> SQAccelerator(*SolverAccelerationStructure);
		{
			double Time = 0.0;
			{
				FScopedDurationTimer Timer(Time);
				SQAccelerator.Sweep(QueryGeom, StartTM, Dir, DeltaMag, HitBuffer, OutputFlags, QueryFilterData, *QueryCallback, DebugParams);
			}

			if constexpr (bGTData && FAccelerationContainerTraits<TAccelContainer, bGTData>::IsPhysScene())
			{
				if (!!SerializeSQs && !!EnableSweepSQCapture)
				{
					SweepSQCaptureHelper(Time, SQAccelerator, Container, QueryGeom, StartTM, Dir, DeltaMag, HitBuffer, OutputFlags, QueryFlags, Filter, QueryFilterData, QueryCallback, DebugParams);
				}
			}
		}
	}
}

DEFINE_LOW_LEVEL_SWEEP_ACCEL(Chaos::IDefaultChaosSpatialAcceleration)
DEFINE_LOW_LEVEL_SWEEP_ACCEL(IExternalSpatialAcceleration)
DEFINE_LOW_LEVEL_SWEEP_ACCEL(FPhysScene)

#define DEFINE_LOW_LEVEL_OVERLAP_ACCEL_HIT(TACCEL, THIT) template void LowLevelOverlap<TACCEL, THIT>(const TACCEL& Container, const FPhysicsGeometry& QueryGeom, const FTransform& GeomPose, FPhysicsHitCallback<THIT>& HitBuffer, FQueryFlags QueryFlags, const FCollisionFilterData& Filter, const ChaosInterface::FQueryFilterData& QueryFilterData, ICollisionQueryFilterCallbackBase* QueryCallback, const ChaosInterface::FQueryDebugParams& DebugParams)
#define DEFINE_LOW_LEVEL_OVERLAP_ACCEL(TACCEL) \
DEFINE_LOW_LEVEL_OVERLAP_ACCEL_HIT(TACCEL, FHitOverlap); \
DEFINE_LOW_LEVEL_OVERLAP_ACCEL_HIT(TACCEL, ChaosInterface::FPTOverlapHit);

template <typename TAccelContainer, typename THitOverlap>
void LowLevelOverlap(const TAccelContainer& Container, const FPhysicsGeometry& QueryGeom, const FTransform& GeomPose, FPhysicsHitCallback<THitOverlap>& HitBuffer, FQueryFlags QueryFlags, const FCollisionFilterData& Filter, const ChaosInterface::FQueryFilterData& QueryFilterData, ICollisionQueryFilterCallbackBase* QueryCallback, const ChaosInterface::FQueryDebugParams& DebugParams)
{
	constexpr bool bGTData = std::is_same<THitOverlap, FHitOverlap>::value;
	using TTraits = FAccelerationContainerTraits<TAccelContainer, bGTData>;
	if (const typename TTraits::TAccelStructure* SolverAccelerationStructure = TTraits::GetSpatialAccelerationFromContainer(Container))
	{
		FGenericChaosSQAccelerator<typename TTraits::TAccelStructure::TPayload> SQAccelerator(*SolverAccelerationStructure);
		double Time = 0.0;
		{
			FScopedDurationTimer Timer(Time);
			SQAccelerator.Overlap(QueryGeom, GeomPose, HitBuffer, QueryFilterData, *QueryCallback, DebugParams);
		}

		if constexpr (bGTData && FAccelerationContainerTraits<TAccelContainer, bGTData>::IsPhysScene())
		{
			if (!!SerializeSQs && !!EnableOverlapSQCapture)
			{
				OverlapSQCaptureHelper(Time, SQAccelerator, Container, QueryGeom, GeomPose, HitBuffer, QueryFlags, Filter, QueryFilterData, QueryCallback, DebugParams);
			}
		}
	}
}

DEFINE_LOW_LEVEL_OVERLAP_ACCEL(Chaos::IDefaultChaosSpatialAcceleration)
DEFINE_LOW_LEVEL_OVERLAP_ACCEL(IExternalSpatialAcceleration)
DEFINE_LOW_LEVEL_OVERLAP_ACCEL(FPhysScene)

#endif // IS_MONOLITHIC