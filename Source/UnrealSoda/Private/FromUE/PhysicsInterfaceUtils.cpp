// Copyright Epic Games, Inc. All Rights Reserved.
// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#if(!IS_MONOLITHIC)

#include "Physics/PhysicsInterfaceUtils.h"
#include "PhysXPublic.h"
#include "WorldCollision.h"
#include "Physics/PhysicsFiltering.h"
#include "Physics/PhysicsInterfaceTypes.h"

FCollisionFilterData CreateObjectQueryFilterData(const bool bTraceComplex, const int32 MultiTrace/*=1 if multi. 0 otherwise*/, const struct FCollisionObjectQueryParams& ObjectParam)
{
	/**
	* Format for QueryData :
	*		word0 (meta data - ECollisionQuery. Extendable)
	*
	*		For object queries
	*
	*		word1 (object type queries)
	*		word2 (unused)
	*		word3 (Multi (1) or single (0) (top 8) + Flags (lower 24))
	*/

	FCollisionFilterData NewData;

	NewData.Word0 = (uint32)ECollisionQuery::ObjectQuery;

	if (bTraceComplex)
	{
		NewData.Word3 |= EPDF_ComplexCollision;
	}
	else
	{
		NewData.Word3 |= EPDF_SimpleCollision;
	}

	// get object param bits
	NewData.Word1 = ObjectParam.GetQueryBitfield();

	// if 'nothing', then set no bits
	NewData.Word3 |= CreateChannelAndFilter((ECollisionChannel)MultiTrace, ObjectParam.IgnoreMask);

	return NewData;
}

FCollisionFilterData CreateTraceQueryFilterData(const uint8 MyChannel, const bool bTraceComplex, const FCollisionResponseContainer& InCollisionResponseContainer, const FCollisionQueryParams& Params)
{
	/**
	* Format for QueryData :
	*		word0 (meta data - ECollisionQuery. Extendable)
	*
	*		For trace queries
	*
	*		word1 (blocking channels)
	*		word2 (touching channels)
	*		word3 (MyChannel (top 8) as ECollisionChannel + Flags (lower 24))
	*/

	FCollisionFilterData NewData;

	NewData.Word0 = (uint32)ECollisionQuery::TraceQuery;

	if (bTraceComplex)
	{
		NewData.Word3 |= EPDF_ComplexCollision;
	}
	else
	{
		NewData.Word3 |= EPDF_SimpleCollision;
	}

	// word1 encodes 'what i block', word2 encodes 'what i touch'
	for (int32 i = 0; i < UE_ARRAY_COUNT(InCollisionResponseContainer.EnumArray); i++)
	{
		if (InCollisionResponseContainer.EnumArray[i] == ECR_Block)
		{
			// if i block, set that in word1
			NewData.Word1 |= CRC_TO_BITFIELD(i);
		}
		else if (InCollisionResponseContainer.EnumArray[i] == ECR_Overlap)
		{
			// if i touch, set that in word2
			NewData.Word2 |= CRC_TO_BITFIELD(i);
		}
	}

	// if 'nothing', then set no bits
	NewData.Word3 |= CreateChannelAndFilter((ECollisionChannel)MyChannel, Params.IgnoreMask);

	return NewData;
}

#define TRACE_MULTI		1
#define TRACE_SINGLE	0

/** Utility for creating a PhysX PxFilterData for performing a query (trace) against the scene */
FCollisionFilterData CreateQueryFilterData(const uint8 MyChannel, const bool bTraceComplex, const FCollisionResponseContainer& InCollisionResponseContainer, const struct FCollisionQueryParams& QueryParam, const struct FCollisionObjectQueryParams& ObjectParam, const bool bMultitrace)
{
	//#TODO implement chaos
	if (ObjectParam.IsValid())
	{
		return CreateObjectQueryFilterData(bTraceComplex, (bMultitrace ? TRACE_MULTI : TRACE_SINGLE), ObjectParam);
	}
	else
	{
		return CreateTraceQueryFilterData(MyChannel, bTraceComplex, InCollisionResponseContainer, QueryParam);
	}
}

#endif // IS_MONOLITHIC