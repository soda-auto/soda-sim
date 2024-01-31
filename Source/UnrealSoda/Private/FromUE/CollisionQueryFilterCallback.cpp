// Copyright Epic Games, Inc. All Rights Reserved.
// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#if(!IS_MONOLITHIC)

#include "Runtime/Engine/Private/PhysicsEngine/CollisionQueryFilterCallback.h"
#include "Runtime/Engine/Private/PhysicsEngine/ScopedSQHitchRepeater.h"
#include "Physics/PhysicsInterfaceUtils.h"
#include "Physics/PhysicsFiltering.h"
#include "PhysicsEngine/BodyInstance.h"
#include "Components/PrimitiveComponent.h"
#include "Collision.h"

#include "ChaosInterfaceWrapperCore.h"
#include "Physics/Experimental/ChaosInterfaceWrapper.h"
#include "Chaos/GeometryParticles.h"

ECollisionQueryHitType FCollisionQueryFilterCallback::CalcQueryHitType(const FCollisionFilterData& QueryFilter, const FCollisionFilterData& ShapeFilter, bool bPreFilter)
{
	ECollisionQuery QueryType = (ECollisionQuery)QueryFilter.Word0;
	FMaskFilter QuerierMaskFilter;
	const ECollisionChannel QuerierChannel = GetCollisionChannelAndExtraFilter(QueryFilter.Word3, QuerierMaskFilter);

	FMaskFilter ShapeMaskFilter;
	const ECollisionChannel ShapeChannel = GetCollisionChannelAndExtraFilter(ShapeFilter.Word3, ShapeMaskFilter);

	if ((QuerierMaskFilter & ShapeMaskFilter) != 0)	//If ignore mask hit something, ignore it
	{
		return ECollisionQueryHitType::None;
	}

	const uint32 ShapeBit = ECC_TO_BITFIELD(ShapeChannel);
	if (QueryType == ECollisionQuery::ObjectQuery)
	{
		const int32 MultiTrace = (int32)QuerierChannel;
		// do I belong to one of objects of interest?
		if (ShapeBit & QueryFilter.Word1)
		{
			if (bPreFilter)	//In the case of an object query we actually want to return all object types (or first in single case). So in PreFilter we have to trick physx by not blocking in the multi case, and blocking in the single case.
			{

				return MultiTrace ? ECollisionQueryHitType::Touch: ECollisionQueryHitType::Block;
			}
			else
			{
				return ECollisionQueryHitType::Block;	//In the case where an object query is being resolved for the user we just return a block because object query doesn't have the concept of overlap at all and block seems more natural
			}
		}
	}
	else
	{
		// if query channel is Touch All, then just return touch
		if (QuerierChannel == ECC_OverlapAll_Deprecated)
		{
			return ECollisionQueryHitType::Touch;
		}
		// @todo delete once we fix up object/trace APIs to work separate

		uint32 const QuerierBit = ECC_TO_BITFIELD(QuerierChannel);
		ECollisionQueryHitType QuerierHitType = ECollisionQueryHitType::None;
		ECollisionQueryHitType ShapeHitType = ECollisionQueryHitType::None;

		// check if Querier wants a hit
		if ((QuerierBit & ShapeFilter.Word1) != 0)
		{
			QuerierHitType = ECollisionQueryHitType::Block;
		}
		else if ((QuerierBit & ShapeFilter.Word2) != 0)
		{
			QuerierHitType = ECollisionQueryHitType::Touch;
		}

		if ((ShapeBit & QueryFilter.Word1) != 0)
		{
			ShapeHitType = ECollisionQueryHitType::Block;
		}
		else if ((ShapeBit & QueryFilter.Word2) != 0)
		{
			ShapeHitType = ECollisionQueryHitType::Touch;
		}

		// return minimum agreed-upon interaction
		return FMath::Min(QuerierHitType, ShapeHitType);
	}

	return ECollisionQueryHitType::None;
}

template <typename TParticle>
ECollisionQueryHitType FCollisionQueryFilterCallback::PreFilterBaseImp(const FCollisionFilterData& FilterData, const Chaos::FPerShapeData& Shape, const TParticle& Actor)
{
	//SCOPE_CYCLE_COUNTER(STAT_Collision_PreFilter);

	if (!Shape.GetQueryEnabled())
	{
		return ECollisionQueryHitType::None;
	}

	FCollisionFilterData ShapeFilter = ChaosInterface::GetQueryFilterData(Shape);

	// We usually don't have ignore components so we try to avoid the virtual getSimulationFilterData() call below. 'word2' of shape sim filter data is componentID.
	uint32 ComponentID = 0;
	if (IgnoreComponents.Num() > 0)
	{
		ComponentID = ChaosInterface::GetSimulationFilterData(Shape).Word2;
	}

	FBodyInstance* BodyInstance = nullptr;

	if constexpr (std::is_same<TParticle, Chaos::FGeometryParticle>::value)
	{
#if ENABLE_PREFILTER_LOGGING || DETECT_SQ_HITCHES
		BodyInstance = ChaosInterface::GetUserData(Actor);
#endif // ENABLE_PREFILTER_LOGGING || DETECT_SQ_HITCHES
	}

	return PreFilterImp(FilterData, ShapeFilter, ComponentID, BodyInstance);
}

ECollisionQueryHitType FCollisionQueryFilterCallback::PreFilterImp(const FCollisionFilterData& FilterData, const Chaos::FPerShapeData& Shape, const Chaos::FGeometryParticle& Actor)
{
	return PreFilterBaseImp(FilterData, Shape, Actor);
}

ECollisionQueryHitType FCollisionQueryFilterCallback::PreFilterImp(const FCollisionFilterData& FilterData, const Chaos::FPerShapeData& Shape, const Chaos::FGeometryParticleHandle& Actor)
{
	return PreFilterBaseImp(FilterData, Shape, Actor);
}

ECollisionQueryHitType FCollisionQueryFilterCallback::PreFilterImp(const FCollisionFilterData& FilterData, const FCollisionFilterData& ShapeFilter, uint32 ComponentID, const FBodyInstance* BodyInstance)
{
#if DETECT_SQ_HITCHES
	FPreFilterRecord* PreFilterRecord = nullptr;
	if (bRecordHitches && (IsInGameThread() || FSQHitchRepeaterCVars::SQHitchDetectionForceNames))
	{
		PreFilterHitchInfo.AddZeroed();
		PreFilterRecord = &PreFilterHitchInfo[PreFilterHitchInfo.Num() - 1];
		
		if(BodyInstance)
		{
			if(UPrimitiveComponent* OwnerComp = BodyInstance->OwnerComponent.Get())
			{
				PreFilterRecord->OwnerComponentReadableName = OwnerComp->GetReadableName();
			}
		}
	}
#endif

#if ENABLE_PREFILTER_LOGGING
	static bool bLoggingEnabled = false;
	if (bLoggingEnabled)
	{
		if(BodyInstance && BodyInstance->OwnerComponent.IsValid())
		{
			UE_LOG(LogCollision, Warning, TEXT("[PREFILTER] against %s[%s] : About to check "),
				(BodyInstance->OwnerComponent.Get()->GetOwner()) ? *BodyInstance->OwnerComponent.Get()->GetOwner()->GetName() : TEXT("NO OWNER"),
				*BodyInstance->OwnerComponent.Get()->GetName());
		}

		UE_LOG(LogCollision, Warning, TEXT("ShapeFilter : %x %x %x %x"), ShapeFilter.Word0, ShapeFilter.Word1, ShapeFilter.Word2, ShapeFilter.Word3);
		UE_LOG(LogCollision, Warning, TEXT("FilterData : %x %x %x %x"), FilterData.Word0, FilterData.Word1, FilterData.Word2, FilterData.Word3);
	}
#endif // ENABLE_PREFILTER_LOGGING

	// Shape : shape's Filter Data
	// Querier : filterData that owns the trace
	uint32 ShapeFlags = ShapeFilter.Word3 & 0xFFFFFF;
	uint32 QuerierFlags = FilterData.Word3 & 0xFFFFFF;
	uint32 CommonFlags = ShapeFlags & QuerierFlags;

	// First check complexity, none of them matches
	if (!(CommonFlags & EPDF_SimpleCollision) && !(CommonFlags & EPDF_ComplexCollision))
	{
		return (PreFilterReturnValue = ECollisionQueryHitType::None);
	}

	ECollisionQueryHitType Result = FCollisionQueryFilterCallback::CalcQueryHitType(FilterData, ShapeFilter, true);

	if (Result == ECollisionQueryHitType::Touch && bIgnoreTouches)
	{
		Result = ECollisionQueryHitType::None;
	}

	if (Result == ECollisionQueryHitType::Block && bIgnoreBlocks)
	{
		Result = ECollisionQueryHitType::None;
	}

	// If not already rejected, check ignore actor and component list.
	if (Result != ECollisionQueryHitType::None)
	{
		// See if we are ignoring the actor this shape belongs to (word0 of shape filterdata is actorID)
		if (IgnoreActors.Contains(ShapeFilter.Word0))
		{
			//UE_LOG(LogTemp, Log, TEXT("Ignoring Actor: %d"), ShapeFilter.word0);
			Result = ECollisionQueryHitType::None;
		}

		if (IgnoreComponents.Contains(ComponentID))
		{
			//UE_LOG(LogTemp, Log, TEXT("Ignoring Component: %d"), shape->getSimulationFilterData().word2);
			Result = ECollisionQueryHitType::None;
		}
	}

#if ENABLE_PREFILTER_LOGGING
	if (bLoggingEnabled)
	{
		ECollisionChannel QuerierChannel = GetCollisionChannel(FilterData.word3);
		UE_LOG(LogCollision, Log, TEXT("[PREFILTER] Result for Querier [CHANNEL: %d, FLAG: %x] %s [%d]"), (int32)QuerierChannel, QuerierFlags, *ComponentName, (int32)Result);
	}
#endif // ENABLE_PREFILTER_LOGGING

	if (bIsOverlapQuery && Result == ECollisionQueryHitType::Block)
	{
		Result = ECollisionQueryHitType::Touch;	//In the case of overlaps, physx only understands touches. We do this at the end to ensure all filtering logic based on block vs overlap is correct
	}

#if DETECT_SQ_HITCHES
	if (PreFilterRecord)
	{
		PreFilterRecord->Result = Result;
	}
#endif

	return  (PreFilterReturnValue = Result);
}


ECollisionQueryHitType FCollisionQueryFilterCallback::PostFilterImp(const FCollisionFilterData& FilterData, bool bIsOverlap)
{
	if (!bIsSweep)
	{
		return ECollisionQueryHitType::Block;
	}
	else if (bIsOverlap && bDiscardInitialOverlaps)
	{
		return ECollisionQueryHitType::None;
	}
	else
	{
		if (bIsOverlap && PreFilterReturnValue == ECollisionQueryHitType::Block)
		{
			// We want to keep initial blocking overlaps and continue the sweep until a non-overlapping blocking hit.
			// We will later report this hit as a blocking hit when we compute the hit type (using CalcQueryHitType).
			return ECollisionQueryHitType::Touch;
		}

		return PreFilterReturnValue;
	}
}

ECollisionQueryHitType FCollisionQueryFilterCallback::PostFilterImp(const FCollisionFilterData& FilterData, const ChaosInterface::FQueryHit& Hit)
{
	// Unused in non-sweeps
	if (!bIsSweep)
	{
		return ECollisionQueryHitType::None;
	}

	const auto& SweepHit = static_cast<const ChaosInterface::FLocationHit&>(Hit);
	const bool bIsOverlap = ChaosInterface::HadInitialOverlap(SweepHit);

	return PostFilterImp(FilterData, bIsOverlap);
}

ECollisionQueryHitType FCollisionQueryFilterCallback::PostFilterImp(const FCollisionFilterData& FilterData, const ChaosInterface::FPTQueryHit& Hit)
{
	// Unused in non-sweeps
	if (!bIsSweep)
	{
		return ECollisionQueryHitType::None;
	}

	const auto& SweepHit = static_cast<const ChaosInterface::FPTLocationHit&>(Hit);
	const bool bIsOverlap = ChaosInterface::HadInitialOverlap(SweepHit);

	return PostFilterImp(FilterData, bIsOverlap);
}

#endif // IS_MONOLITHIC