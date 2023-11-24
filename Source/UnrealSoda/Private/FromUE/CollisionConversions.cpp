// Copyright Epic Games, Inc. All Rights Reserved.
// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#if(!IS_MONOLITHIC)

#include "Runtime/Engine/Private/Collision/CollisionConversions.h"
#include "Engine/World.h"
#include "Components/PrimitiveComponent.h"

//#include "Collision/CollisionDebugDrawing.h"
#include "Components/LineBatchComponent.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "PhysicalMaterials/PhysicalMaterialMask.h"
#include "PhysicsEngine/PhysicsSettings.h"
#include "PhysicsEngine/BodySetup.h"
#include "Physics/PhysicsInterfaceUtils.h"

#include "Physics/Experimental/ChaosInterfaceWrapper.h"
#include "Physics/Experimental/PhysInterface_Chaos.h"
#include "Chaos/ParticleHandle.h"

#include "Runtime/Engine/Private/PhysicsEngine/CollisionQueryFilterCallback.h"
#include "GameFramework/LightWeightInstanceManager.h"
#include "PhysicsProxy/SingleParticlePhysicsProxy.h"
#include "PhysicsProxy/GeometryCollectionPhysicsProxy.h"

#include "ChaosInterfaceWrapperCore.h"



ECollisionShapeType GetGeometryType(const Chaos::FPerShapeData& Shape)
{
	return ChaosInterface::GetType(*Shape.GetGeometry());
}


// Used to place overlaps into a TMap when deduplicating them
struct FOverlapKey
{
	UPrimitiveComponent* Component;
	int32 ComponentIndex;

	FOverlapKey(UPrimitiveComponent* InComponent, int32 InComponentIndex)
		: Component(InComponent)
		, ComponentIndex(InComponentIndex)
	{
	}

	friend bool operator==(const FOverlapKey& X, const FOverlapKey& Y)
	{
		return (X.ComponentIndex == Y.ComponentIndex) && (X.Component == Y.Component);
	}
};

uint32 GetTypeHash(const FOverlapKey& Key)
{
	return GetTypeHash(Key.Component) ^ GetTypeHash(Key.ComponentIndex);
}


//extern int32 CVarShowInitialOverlaps;

// Forward declare, I don't want to move the entire function right now or we lose change history.
template <typename THitLocation>
static bool ConvertOverlappedShapeToImpactHit(const UWorld* World, const THitLocation& Hit, const FVector& StartLoc, const FVector& EndLoc, FHitResult& OutResult, const FPhysicsGeometry& Geom, const FTransform& QueryTM, const FCollisionFilterData& QueryFilter, bool bReturnPhysMat);

DECLARE_CYCLE_STAT(TEXT("ConvertQueryHit"), STAT_ConvertQueryImpactHit, STATGROUP_Collision);
DECLARE_CYCLE_STAT(TEXT("ConvertOverlapToHit"), STAT_CollisionConvertOverlapToHit, STATGROUP_Collision);
DECLARE_CYCLE_STAT(TEXT("ConvertOverlap"), STAT_CollisionConvertOverlap, STATGROUP_Collision);
DECLARE_CYCLE_STAT(TEXT("SetHitResultFromShapeAndFaceIndex"), STAT_CollisionSetHitResultFromShapeAndFaceIndex, STATGROUP_Collision);

#define ENABLE_CHECK_HIT_NORMAL  (!(UE_BUILD_SHIPPING || UE_BUILD_TEST))

#if ENABLE_CHECK_HIT_NORMAL
/* Validate Normal of OutResult. We're on hunt for invalid normal */
static void CheckHitResultNormal(const FHitResult& OutResult, const TCHAR* Message, const FVector& Start = FVector::ZeroVector, const FVector& End = FVector::ZeroVector, const FPhysicsGeometry* Geom = nullptr, const FVector& NormalPreSafeNormalize = FVector::ZeroVector)
{
	using namespace ChaosInterface;

	if (!OutResult.bStartPenetrating && !OutResult.Normal.IsNormalized())
	{
		UE_LOG(LogPhysics, Warning, TEXT("(%s) Non-normalized OutResult.Normal from hit conversion: %s (Component- %s)"), Message, *OutResult.Normal.ToString(), *OutResult.Component->GetFullName());
		UE_LOG(LogPhysics, Warning, TEXT("Start Loc(%s), End Loc(%s), Hit Loc(%s), ImpactNormal(%s) NormalPreSafeNormalize(%s)"), *Start.ToString(), *End.ToString(), *OutResult.Location.ToString(), *OutResult.ImpactNormal.ToString(), *NormalPreSafeNormalize.ToString());
		if (Geom)
		{
			if (GetType(*Geom) == ECollisionShapeType::Capsule)
			{
				const FPhysicsCapsuleGeometry& Capsule = (FPhysicsCapsuleGeometry&)*Geom;
				UE_LOG(LogPhysics, Warning, TEXT("Capsule radius (%f), Capsule Halfheight (%f)"), GetRadius(Capsule), GetHalfHeight(Capsule));
			}
		}
		ensure(OutResult.Normal.IsNormalized());
	}
}
#endif // ENABLE_CHECK_HIT_NORMAL




static FVector FindSimpleOpposingNormal(const FHitLocation& Hit, const FVector& TraceDirectionDenorm, const FVector InNormal)
{
	// We don't compute anything special
	return InNormal;
}


/**
 * Util to find the normal of the face that we hit. Will use faceIndex from the hit if possible.
 * @param PHit - incoming hit from Physics
 * @param TraceDirectionDenorm - direction of sweep test (not normalized)
 * @param InNormal - default value in case no new normal is computed.
 * @return New normal we compute for geometry.
 */
template <typename THitLocation>
static FVector FindGeomOpposingNormal(ECollisionShapeType QueryGeomType, const THitLocation& Hit, const FVector& TraceDirectionDenorm, const FVector InNormal)
{
	// TODO: can we support other shapes here as well?
	if (QueryGeomType == ECollisionShapeType::Capsule || QueryGeomType == ECollisionShapeType::Sphere)
	{
		if (const FPhysicsShape* Shape = GetShape(Hit))
		{
			const Chaos::EImplicitObjectType ShapeType = Shape->GetGeometry()->GetNestedType();
			if (ShapeType == Chaos::ImplicitObjectType::Union && !Hit.FaceNormal.IsNearlyZero())
			{
				return Hit.FaceNormal;
			}
			else
			{
				const FTransform ActorTM(Hit.Actor->R(), Hit.Actor->X());
				const FVector LocalInNormal = ActorTM.InverseTransformVectorNoScale(InNormal);
				const FVector LocalTraceDirectionDenorm = ActorTM.InverseTransformVectorNoScale(TraceDirectionDenorm);
				const FVector LocalNormal = Shape->GetGeometry()->FindGeometryOpposingNormal(LocalTraceDirectionDenorm, Hit.FaceIndex, LocalInNormal);
				return ActorTM.TransformVectorNoScale(LocalNormal);
			}
		}
	}

	return InNormal;
}

/** Set info in the HitResult (Actor, Component, PhysMaterial, BoneName, Item) based on the supplied shape and face index */
static void SetHitResultFromShapeAndFaceIndex(const FPhysicsShape& Shape, const FPhysicsActor& Actor, const uint32 FaceIndex, const FVector& HitLocation, FHitResult& OutResult, bool bReturnPhysMat)
{
	using namespace ChaosInterface;

	SCOPE_CYCLE_COUNTER(STAT_CollisionSetHitResultFromShapeAndFaceIndex);

	const int32 ShapeIndex = Shape.GetShapeIndex();
	CHAOS_CHECK(ShapeIndex < (int32)TNumericLimits<uint8>::Max()); // I could just write < 256, but this makes it more clear *why*
	OutResult.ElementIndex = (uint8)ShapeIndex;

	UPrimitiveComponent* OwningComponent = nullptr;
	if (const FBodyInstance* BodyInst = GetUserData(Actor))
	{
		BodyInst = FPhysicsInterface::ShapeToOriginalBodyInstance(BodyInst, &Shape);

		//Normal case where we hit a body
		OutResult.Item = BodyInst->InstanceBodyIndex;
		const UBodySetupCore* BodySetup = BodyInst->BodySetup.Get();	//this data should be immutable at runtime so ok to check from worker thread.
		if (BodySetup)
		{
			OutResult.BoneName = BodySetup->BoneName;
		}

		OwningComponent = BodyInst->OwnerComponent.Get();
	}
	else
	{
		// Currently geom collections are registered with a primitive component user data, but maybe custom should be adapted
		// to be more general so we can support leaf identification #BGTODO
		void* UserData = Actor.UserData();
		UPrimitiveComponent* PossibleOwner = FChaosUserData::Get<UPrimitiveComponent>(UserData);

		if (PossibleOwner)
		{
			OwningComponent = PossibleOwner;
			OutResult.Item = INDEX_NONE;
			OutResult.BoneName = NAME_None;

			// if we have a geometry component let's extract the index of the active piece we have a hit for
			if (const IPhysicsProxyBase* ActorProxy = Actor.GetProxy())
			{
				if (ActorProxy->GetType() == EPhysicsProxyType::GeometryCollectionType)
				{
					const FGeometryCollectionPhysicsProxy* ConcreteProxy = static_cast<const FGeometryCollectionPhysicsProxy*>(ActorProxy);
					const FGeometryCollectionItemIndex ItemIndex = ConcreteProxy->GetItemIndexFromGTParticle_External(Actor.CastToRigidParticle());
					OutResult.Item = ItemIndex.GetItemIndex();
					OutResult.BoneName = ConcreteProxy->GetTransformName_External(ItemIndex);
				}
			}
		}
		else
		{
			ensureMsgf(false, TEXT("SetHitResultFromShapeAndFaceIndex hit shape with invalid userData"));
		}
	}

	OutResult.PhysMaterial = nullptr;

	// Grab actor/component
	if (OwningComponent)
	{
		OutResult.Component = OwningComponent;
		AActor* Owner = OwningComponent->GetOwner();
		if (ALightWeightInstanceManager* LWIManager = Cast<ALightWeightInstanceManager>(Owner))
		{
			OutResult.HitObjectHandle = FActorInstanceHandle(LWIManager, OutResult.Item);
		}
		else
		{
			OutResult.HitObjectHandle = FActorInstanceHandle(OwningComponent->GetOwner());
		}

		/*
		if (bReturnPhysMat)
		{
			if (const FPhysicsMaterial* PhysicsMaterial = GetMaterialFromInternalFaceIndexAndHitLocation(Shape, Actor, FaceIndex, HitLocation))
			{
				OutResult.PhysMaterial = GetUserData(*PhysicsMaterial);
			}
		}
		*/
	}

	OutResult.FaceIndex = INDEX_NONE;
}

const FPhysicsActor* GetGTActor(const FPhysicsActor* GTActor) { return GTActor; }

template <bool bGT>
const FPhysicsShape* GetGTShape(const FPhysicsShape* GTShape, const FPhysicsActor* GTActor) { return GTShape; }

template <>
const FPhysicsShape* GetGTShape<false>(const FPhysicsShape* PTShape, const FPhysicsActor* GTActor)
{
	//This assumes that the geometry hasn't changed.
	//If it has you will either get null, or the wrong GT shape (note this gt shape is valid and safe to access, user will just get incorrect result)
	//This is expected because you are accessing GT timeline data from the PT timeline
	if (GTActor)
	{
		const Chaos::FShapesArray& Shapes = GTActor->ShapesArray();
		const int32 Idx = PTShape->GetShapeIndex();
		if (Idx < Shapes.Num())
		{
			return Shapes[Idx].Get();
		}
	}

	return nullptr;
}

const FPhysicsActor* GetGTActor(const Chaos::FGeometryParticleHandle* PTActor)
{
	//TODO: need to pass in context so that in PT we always return null
	//In frozen GT this is ok because object can't be unregistered while we're holding on to this
	//On PT this is only true if we acquire a gt lock which is not great
	auto Proxy = static_cast<const Chaos::FSingleParticlePhysicsProxy*>(PTActor->PhysicsProxy());
	return Proxy->GetParticle_LowLevel();
}

template <typename THitLocation>
EConvertQueryResult ConvertQueryImpactHitImp(const UWorld* World, const THitLocation& Hit, FHitResult& OutResult, float CheckLength, const FCollisionFilterData& QueryFilter, const FVector& StartLoc, const FVector& EndLoc, const FPhysicsGeometry* Geom, const FTransform& QueryTM, bool bReturnFaceIndex, bool bReturnPhysMat)
{
	using namespace ChaosInterface;

	SCOPE_CYCLE_COUNTER(STAT_ConvertQueryImpactHit);

#if WITH_EDITOR
	if (bReturnFaceIndex && World->IsGameWorld())
	{
		if (UPhysicsSettings::Get()->bSuppressFaceRemapTable)
		{
			bReturnFaceIndex = false;	//The editor uses the remap table, so we modify this to get the same results as you would in a cooked build
		}
	}
#endif

	FHitFlags Flags = GetFlags(Hit);
	checkSlow(Flags & EHitFlags::Distance);

	const FPhysicsShape* pHitShape = GetShape(Hit);
	const auto pHitActor = GetActor(Hit);

	const uint32 InternalFaceIndex = GetInternalFaceIndex(Hit);
	const bool bInitialOverlap = HadInitialOverlap(Hit);

	if (bInitialOverlap && Geom)
	{
		ConvertOverlappedShapeToImpactHit(World, Hit, StartLoc, EndLoc, OutResult, *Geom, QueryTM, QueryFilter, bReturnPhysMat);

		if (pHitShape)
		{
			const bool bTriMesh = GetGeometryType(*pHitShape) == ECollisionShapeType::Trimesh;
			const bool bValidInternalFace = InternalFaceIndex != GetInvalidPhysicsFaceIndex();
			if (bReturnFaceIndex && bTriMesh && bValidInternalFace)
			{
				//OutResult.FaceIndex = GetTriangleMeshExternalFaceIndex(*pHitShape, InternalFaceIndex);
			}
		}

		return EConvertQueryResult::Valid;
	}

	if ((pHitShape == nullptr) || (pHitActor == nullptr))
	{
		OutResult.Reset();
		return EConvertQueryResult::Invalid;
	}

	const FPhysicsShape& HitShape = *pHitShape;
	const auto& HitActor = *pHitActor;

	//Get the GT actor/shape for accessing GT specific data, should not be used for things like hit location
	const FPhysicsActor* GTActor = GetGTActor(pHitActor);
	//Note that a non-gt query could still happen on gt if we're in a fixed tick
	constexpr bool bIsGTQuery = std::is_same<THitLocation, FHitLocation>::value;
	const FPhysicsShape* GTShape = GetGTShape<bIsGTQuery>(pHitShape, GTActor);

	// See if this is a 'blocking' hit
	const FCollisionFilterData ShapeFilter = GetQueryFilterData(HitShape);
	const ECollisionQueryHitType HitType = FCollisionQueryFilterCallback::CalcQueryHitType(QueryFilter, ShapeFilter);
	OutResult.bBlockingHit = (HitType == ECollisionQueryHitType::Block);
	OutResult.bStartPenetrating = bInitialOverlap;

	// calculate the hit time
	const float HitTime = GetDistance(Hit) / CheckLength;
	OutResult.Time = HitTime;
	OutResult.Distance = GetDistance(Hit);

	// figure out where the the "safe" location for this shape is by moving from the startLoc toward the ImpactPoint
	const FVector TraceStartToEnd = EndLoc - StartLoc;
	const FVector SafeLocationToFitShape = StartLoc + (HitTime * TraceStartToEnd);
	OutResult.Location = SafeLocationToFitShape;

	const bool bUseReturnedPoint = ((Flags & EHitFlags::Position) && !bInitialOverlap);
	FVector Position = StartLoc;
	if (bUseReturnedPoint)
	{
		Position = GetPosition(Hit);
		if (Position.ContainsNaN() && GTActor && GTShape)
		{
#if ENABLE_NAN_DIAGNOSTIC
			SetHitResultFromShapeAndFaceIndex(*GTShape, *GTActor, InternalFaceIndex, OutResult.ImpactPoint, OutResult, bReturnPhysMat);
			UE_LOG(LogCore, Error, TEXT("ConvertQueryImpactHit() NaN details:\n>> Actor:%s (%s)\n>> Component:%s\n>> Item:%d\n>> BoneName:%s\n>> Time:%f\n>> Distance:%f\n>> Location:%s\n>> bIsBlocking:%d\n>> bStartPenetrating:%d"),
				*OutResult.GetHitObjectHandle().GetName(), OutResult.HasValidHitObjectHandle() ? *OutResult.GetHitObjectHandle().FetchActor()->GetPathName() : TEXT("no path"),
				*GetNameSafe(OutResult.GetComponent()), OutResult.Item, *OutResult.BoneName.ToString(),
				OutResult.Time, OutResult.Distance, *OutResult.Location.ToString(), OutResult.bBlockingHit ? 1 : 0, OutResult.bStartPenetrating ? 1 : 0);
#endif // ENABLE_NAN_DIAGNOSTIC

			OutResult.Reset();
			logOrEnsureNanError(TEXT("ConvertQueryImpactHit() received NaN/Inf for position: %s"), *Position.ToString());
			return EConvertQueryResult::Invalid;
		}
	}
	OutResult.ImpactPoint = Position;

	// Caution: we may still have an initial overlap, but with null Geom. This is the case for RayCast results.
	const bool bUseReturnedNormal = ((Flags & EHitFlags::Normal) && !bInitialOverlap);
	const FVector HitNormal = GetNormal(Hit);
	if (bUseReturnedNormal && HitNormal.ContainsNaN() && GTActor && GTShape)
	{
#if ENABLE_NAN_DIAGNOSTIC
		SetHitResultFromShapeAndFaceIndex(*GTShape, *GTActor, InternalFaceIndex, OutResult.ImpactPoint, OutResult, bReturnPhysMat);
		UE_LOG(LogCore, Error, TEXT("ConvertQueryImpactHit() NaN details:\n>> Actor:%s (%s)\n>> Component:%s\n>> Item:%d\n>> BoneName:%s\n>> Time:%f\n>> Distance:%f\n>> Location:%s\n>> bIsBlocking:%d\n>> bStartPenetrating:%d"),
			*OutResult.GetHitObjectHandle().GetName(), OutResult.HasValidHitObjectHandle() ? *OutResult.GetHitObjectHandle().FetchActor()->GetPathName() : TEXT("no path"),
			*GetNameSafe(OutResult.GetComponent()), OutResult.Item, *OutResult.BoneName.ToString(),
			OutResult.Time, OutResult.Distance, *OutResult.Location.ToString(), OutResult.bBlockingHit ? 1 : 0, OutResult.bStartPenetrating ? 1 : 0);
#endif // ENABLE_NAN_DIAGNOSTIC

		OutResult.Reset();
		logOrEnsureNanError(TEXT("ConvertQueryImpactHit() received NaN/Inf for normal: %s"), *HitNormal.ToString());
		return EConvertQueryResult::Invalid;
	}

	FVector OriginalNormal = bUseReturnedNormal ? HitNormal : -TraceStartToEnd;
	FVector Normal = OriginalNormal.GetSafeNormal();
	OutResult.Normal = Normal;
	OutResult.ImpactNormal = Normal;

	OutResult.TraceStart = StartLoc;
	OutResult.TraceEnd = EndLoc;

	// Fill in Actor, Component, material, etc.
	if (GTActor && GTShape)
	{
		SetHitResultFromShapeAndFaceIndex(*GTShape, *GTActor, InternalFaceIndex, OutResult.ImpactPoint, OutResult, bReturnPhysMat);
	}

#if ENABLE_CHECK_HIT_NORMAL
	CheckHitResultNormal(OutResult, TEXT("Invalid Normal from ConvertQueryImpactHit"), StartLoc, EndLoc, Geom, OriginalNormal);
#endif // ENABLE_CHECK_HIT_NORMAL

	if (bUseReturnedNormal && !Normal.IsNormalized())
	{
		// TraceStartToEnd should never be zero, because of the length restriction in the raycast and sweep tests.
		Normal = -TraceStartToEnd.GetSafeNormal();
		OutResult.Normal = Normal;
		OutResult.ImpactNormal = Normal;
	}

	const ECollisionShapeType SweptGeometryType = Geom ? GetType(*Geom) : ECollisionShapeType::None;
	OutResult.ImpactNormal = FindGeomOpposingNormal(SweptGeometryType, Hit, TraceStartToEnd, Normal);

	ECollisionShapeType GeomType = GetGeometryType(HitShape);

	if (GeomType == ECollisionShapeType::Heightfield)
	{
		// Lookup physical material for heightfields
		if (bReturnPhysMat && InternalFaceIndex != GetInvalidPhysicsFaceIndex() && GTActor && GTShape)
		{
			/*
			if (const FPhysicsMaterial* Material = GetMaterialFromInternalFaceIndex(*GTShape, *GTActor, InternalFaceIndex))
			{
				OutResult.PhysMaterial = GetUserData(*Material);
			}
			*/
		}
	}
	else if (bReturnFaceIndex && GeomType == ECollisionShapeType::Trimesh && InternalFaceIndex != GetInvalidPhysicsFaceIndex() && GTShape)
	{
		//OutResult.FaceIndex = GetTriangleMeshExternalFaceIndex(*GTShape, InternalFaceIndex);
	}

	return EConvertQueryResult::Valid;
}

EConvertQueryResult ConvertQueryImpactHit(const UWorld* World, const FHitLocation& Hit, FHitResult& OutResult, float CheckLength, const FCollisionFilterData& QueryFilter, const FVector& StartLoc, const FVector& EndLoc, const FPhysicsGeometry* Geom, const FTransform& QueryTM, bool bReturnFaceIndex, bool bReturnPhysMat)
{
	return ConvertQueryImpactHitImp(World, Hit, OutResult, CheckLength, QueryFilter, StartLoc, EndLoc, Geom, QueryTM, bReturnFaceIndex, bReturnPhysMat);
}
EConvertQueryResult ConvertQueryImpactHit(const UWorld* World, const ChaosInterface::FPTLocationHit& Hit, FHitResult& OutResult, float CheckLength, const FCollisionFilterData& QueryFilter, const FVector& StartLoc, const FVector& EndLoc, const FPhysicsGeometry* Geom, const FTransform& QueryTM, bool bReturnFaceIndex, bool bReturnPhysMat)
{
	return ConvertQueryImpactHitImp(World, Hit, OutResult, CheckLength, QueryFilter, StartLoc, EndLoc, Geom, QueryTM, bReturnFaceIndex, bReturnPhysMat);
}

template <typename HitType>
EConvertQueryResult ConvertTraceResults(bool& OutHasValidBlockingHit, const UWorld* World, int32 NumHits, HitType* Hits, float CheckLength, const FCollisionFilterData& QueryFilter, TArray<FHitResult>& OutHits, const FVector& StartLoc, const FVector& EndLoc, const FPhysicsGeometry* Geom, const FTransform& QueryTM, float MaxDistance, bool bReturnFaceIndex, bool bReturnPhysMat)
{
	OutHits.Reserve(OutHits.Num() + NumHits);
	EConvertQueryResult ConvertResult = EConvertQueryResult::Valid;
	bool bHadBlockingHit = false;
	const FVector Dir = (EndLoc - StartLoc).GetSafeNormal();

	for (int32 i = 0; i < NumHits; i++)
	{
		HitType& Hit = Hits[i];
		if (GetDistance(Hit) <= MaxDistance)
		{
			if constexpr (std::is_same_v<HitType, FHitSweep>)
			{
				if (!HadInitialOverlap(Hit))
				{
					SetInternalFaceIndex(Hit, FindFaceIndex(Hit, Dir));
				}
			}

			FHitResult& NewResult = OutHits[OutHits.AddDefaulted()];
			if (ConvertQueryImpactHit(World, Hit, NewResult, CheckLength, QueryFilter, StartLoc, EndLoc, Geom, QueryTM, bReturnFaceIndex, bReturnPhysMat) == EConvertQueryResult::Valid)
			{
				bHadBlockingHit |= NewResult.bBlockingHit;
			}
			else
			{
				// Reject invalid result (this should be rare). Remove from the results.
				OutHits.Pop(/*bAllowShrinking=*/ false);
				ConvertResult = EConvertQueryResult::Invalid;
			}

		}
	}

	// Sort results from first to last hit
	OutHits.Sort(FCompareFHitResultTime());
	OutHasValidBlockingHit = bHadBlockingHit;
	return ConvertResult;
}

template <typename Hit>
EConvertQueryResult ConvertTraceResults(bool& OutHasValidBlockingHit, const UWorld* World, int32 NumHits, Hit* Hits, float CheckLength, const FCollisionFilterData& QueryFilter, FHitResult& OutHit, const FVector& StartLoc, const FVector& EndLoc, const FPhysicsGeometry* Geom, const FTransform& QueryTM, float MaxDistance, bool bReturnFaceIndex, bool bReturnPhysMat)
{
	const FVector Dir = (EndLoc - StartLoc).GetSafeNormal();
	if constexpr (std::is_same_v<Hit, FHitSweep>)
	{
		if (!HadInitialOverlap(Hits[0]))
		{
			SetInternalFaceIndex(Hits[0], FindFaceIndex(Hits[0], Dir));
		}
	}
	EConvertQueryResult Result = ConvertQueryImpactHit(World, Hits[0], OutHit, CheckLength, QueryFilter, StartLoc, EndLoc, Geom, QueryTM, bReturnFaceIndex, bReturnPhysMat);
	OutHasValidBlockingHit = Result == EConvertQueryResult::Valid;
	return Result;
}

template EConvertQueryResult ConvertTraceResults<FHitSweep>(bool& OutHasValidBlockingHit, const UWorld* World, int32 NumHits, FHitSweep* Hits, float CheckLength, const FCollisionFilterData& QueryFilter, TArray<FHitResult>& OutHits, const FVector& StartLoc, const FVector& EndLoc, const FPhysicsGeometry* Geom, const FTransform& QueryTM, float MaxDistance, bool bReturnFaceIndex, bool bReturnPhysMat);
template EConvertQueryResult ConvertTraceResults<FHitSweep>(bool& OutHasValidBlockingHit, const UWorld* World, int32 NumHits, FHitSweep* Hits, float CheckLength, const FCollisionFilterData& QueryFilter, FHitResult& OutHit, const FVector& StartLoc, const FVector& EndLoc, const FPhysicsGeometry* Geom, const FTransform& QueryTM, float MaxDistance, bool bReturnFaceIndex, bool bReturnPhysMat);
template EConvertQueryResult ConvertTraceResults<ChaosInterface::FPTSweepHit>(bool& OutHasValidBlockingHit, const UWorld* World, int32 NumHits, ChaosInterface::FPTSweepHit* Hits, float CheckLength, const FCollisionFilterData& QueryFilter, TArray<FHitResult>& OutHits, const FVector& StartLoc, const FVector& EndLoc, const FPhysicsGeometry* Geom, const FTransform& QueryTM, float MaxDistance, bool bReturnFaceIndex, bool bReturnPhysMat);
template EConvertQueryResult ConvertTraceResults<ChaosInterface::FPTSweepHit>(bool& OutHasValidBlockingHit, const UWorld* World, int32 NumHits, ChaosInterface::FPTSweepHit* Hits, float CheckLength, const FCollisionFilterData& QueryFilter, FHitResult& OutHit, const FVector& StartLoc, const FVector& EndLoc, const FPhysicsGeometry* Geom, const FTransform& QueryTM, float MaxDistance, bool bReturnFaceIndex, bool bReturnPhysMat);
template EConvertQueryResult ConvertTraceResults<FHitRaycast>(bool& OutHasValidBlockingHit, const UWorld* World, int32 NumHits, FHitRaycast* Hits, float CheckLength, const FCollisionFilterData& QueryFilter, TArray<FHitResult>& OutHits, const FVector& StartLoc, const FVector& EndLoc, const FPhysicsGeometry* Geom, const FTransform& QueryTM, float MaxDistance, bool bReturnFaceIndex, bool bReturnPhysMat);
template EConvertQueryResult ConvertTraceResults<FHitRaycast>(bool& OutHasValidBlockingHit, const UWorld* World, int32 NumHits, FHitRaycast* Hits, float CheckLength, const FCollisionFilterData& QueryFilter, FHitResult& OutHit, const FVector& StartLoc, const FVector& EndLoc, const FPhysicsGeometry* Geom, const FTransform& QueryTM, float MaxDistance, bool bReturnFaceIndex, bool bReturnPhysMat);
template EConvertQueryResult ConvertTraceResults<ChaosInterface::FPTRaycastHit>(bool& OutHasValidBlockingHit, const UWorld* World, int32 NumHits, ChaosInterface::FPTRaycastHit* Hits, float CheckLength, const FCollisionFilterData& QueryFilter, TArray<FHitResult>& OutHits, const FVector& StartLoc, const FVector& EndLoc, const FPhysicsGeometry* Geom, const FTransform& QueryTM, float MaxDistance, bool bReturnFaceIndex, bool bReturnPhysMat);
template EConvertQueryResult ConvertTraceResults<ChaosInterface::FPTRaycastHit>(bool& OutHasValidBlockingHit, const UWorld* World, int32 NumHits, ChaosInterface::FPTRaycastHit* Hits, float CheckLength, const FCollisionFilterData& QueryFilter, FHitResult& OutHit, const FVector& StartLoc, const FVector& EndLoc, const FPhysicsGeometry* Geom, const FTransform& QueryTM, float MaxDistance, bool bReturnFaceIndex, bool bReturnPhysMat);

/** Util to convert an overlapped shape into a sweep hit result, returns whether it was a blocking hit. */
template <typename THitLocation>
static bool ConvertOverlappedShapeToImpactHit(const UWorld* World, const THitLocation& Hit, const FVector& StartLoc, const FVector& EndLoc, FHitResult& OutResult, const FPhysicsGeometry& Geom, const FTransform& QueryTM, const FCollisionFilterData& QueryFilter, bool bReturnPhysMat)
{
	using namespace ChaosInterface;

	SCOPE_CYCLE_COUNTER(STAT_CollisionConvertOverlapToHit);

	const FPhysicsShape* pHitShape = GetShape(Hit);
	const auto pHitActor = GetActor(Hit);
	if ((pHitShape == nullptr) || (pHitActor == nullptr))
	{
		OutResult.Reset();
		return false;
	}

	const FPhysicsShape& HitShape = *pHitShape;
	const auto& HitActor = *pHitActor;

	// See if this is a 'blocking' hit
	FCollisionFilterData ShapeFilter = GetQueryFilterData(HitShape);
	const ECollisionQueryHitType HitType = FCollisionQueryFilterCallback::CalcQueryHitType(QueryFilter, ShapeFilter);
	const bool bBlockingHit = (HitType == ECollisionQueryHitType::Block);
	OutResult.bBlockingHit = bBlockingHit;

	// Time of zero because initially overlapping
	OutResult.bStartPenetrating = true;
	OutResult.Time = 0.f;
	OutResult.Distance = 0.f;

	// Return start location as 'safe location'
	OutResult.Location = QueryTM.GetLocation();

	const bool bValidPosition = !!(GetFlags(Hit) & EHitFlags::Position);
	if (bValidPosition)
	{
		const FVector HitPosition = GetPosition(Hit);
		const bool bFinitePosition = !HitPosition.ContainsNaN();
		if (bFinitePosition)
		{
			OutResult.ImpactPoint = HitPosition;
		}
		else
		{
			OutResult.ImpactPoint = StartLoc;
			UE_LOG(LogPhysics, Verbose, TEXT("Warning: ConvertOverlappedShapeToImpactHit: MTD returned NaN :( position: %s"), *HitPosition.ToString());
		}
	}
	else
	{
		OutResult.ImpactPoint = StartLoc;
	}
	OutResult.TraceStart = StartLoc;
	OutResult.TraceEnd = EndLoc;

	const FVector HitNormal = GetNormal(Hit);
	const bool bFiniteNormal = !HitNormal.ContainsNaN();
	const bool bValidNormal = (GetFlags(Hit) & EHitFlags::Normal) && bFiniteNormal;

	// Use MTD result if possible. We interpret the MTD vector as both the direction to move and the opposing normal.
	if (bValidNormal)
	{
		OutResult.ImpactNormal = HitNormal;
		OutResult.PenetrationDepth = FMath::Abs(GetDistance(Hit));
	}
	else
	{
		// Fallback normal if we can't find it with MTD or otherwise.
		OutResult.ImpactNormal = FVector::UpVector;
		OutResult.PenetrationDepth = 0.f;
		if (!bFiniteNormal)
		{
			UE_LOG(LogPhysics, Verbose, TEXT("Warning: ConvertOverlappedShapeToImpactHit: MTD returned NaN :( normal: %s"), *HitNormal.ToString());
		}
	}
/*
#if DRAW_OVERLAPPING_TRIS
	if (CVarShowInitialOverlaps != 0 && World && World->IsGameWorld())
	{
		DrawOverlappingTris(World, Hit, Geom, QueryTM);
	}
#endif
*/

	if (bBlockingHit)
	{
		// Zero-distance hits are often valid hits and we can extract the hit normal.
		// For invalid normals we can try other methods as well (get overlapping triangles).
		if (GetDistance(Hit) == 0.f || !bValidNormal)	//todo(ocohen): isn't hit distance always zero in this function? should this be if(!bValidNormal) ?
		{
			ComputeZeroDistanceImpactNormalAndPenetration(World, Hit, Geom, QueryTM, OutResult);
		}
	}
	else
	{
		// non blocking hit (overlap).
		if (!bValidNormal)
		{
			OutResult.ImpactNormal = (StartLoc - EndLoc).GetSafeNormal();
			ensure(OutResult.ImpactNormal.IsNormalized());
		}
	}

	OutResult.Normal = OutResult.ImpactNormal;

	const FPhysicsActor* GTHitActor = GetGTActor(&HitActor);

	//Note that a non-gt query could still happen on gt if we're in a fixed tick
	constexpr bool bIsGTQuery = std::is_same<THitLocation, FHitLocation>::value;
	const FPhysicsShape* GTHitShape = GetGTShape<bIsGTQuery>(&HitShape, GTHitActor);
	if (GTHitActor && GTHitShape)
	{
		SetHitResultFromShapeAndFaceIndex(*GTHitShape, *GTHitActor, GetInternalFaceIndex(Hit), OutResult.ImpactPoint, OutResult, bReturnPhysMat);
	}

	return bBlockingHit;
}


void ConvertQueryOverlap(const FPhysicsShape& Shape, const FPhysicsActor& Actor, FOverlapResult& OutOverlap, const FCollisionFilterData& QueryFilter)
{
	using namespace ChaosInterface;

	// Grab actor/component

	// Try body instance
	if (const FBodyInstance* BodyInst = GetUserData(Actor))
	{
		BodyInst = FPhysicsInterface::ShapeToOriginalBodyInstance(BodyInst, &Shape);
		if (const UPrimitiveComponent* OwnerComponent = BodyInst->OwnerComponent.Get())
		{
			if (ALightWeightInstanceManager* LWIManager = Cast<ALightWeightInstanceManager>(OwnerComponent->GetOwner()))
			{
				OutOverlap.OverlapObjectHandle = FActorInstanceHandle(LWIManager, BodyInst->InstanceBodyIndex);
			}
			else
			{
				OutOverlap.OverlapObjectHandle = FActorInstanceHandle(OwnerComponent->GetOwner());
			}
			OutOverlap.Component = BodyInst->OwnerComponent; // Copying weak pointer is faster than assigning raw pointer.
			OutOverlap.ItemIndex = OwnerComponent->bMultiBodyOverlap ? BodyInst->InstanceBodyIndex : INDEX_NONE;
		}
	}
	else
	{
		// Currently geom collections are registered with a primitive component user data, but maybe custom should be adapted
		// to be more general so we can support leaf identification #BGTODO
		void* UserData = Actor.UserData();
		UPrimitiveComponent* PossibleOwner = FChaosUserData::Get<UPrimitiveComponent>(UserData);

		if (PossibleOwner)
		{
			OutOverlap.Component = PossibleOwner;
			OutOverlap.OverlapObjectHandle = FActorInstanceHandle(OutOverlap.Component->GetOwner());
			OutOverlap.ItemIndex = INDEX_NONE;
		}
		else
		{
			ensureMsgf(false, TEXT("ConvertQueryOverlap called with bad payload type"));
		}
	}
}

/** Util to add NewOverlap to OutOverlaps if it is not already there */
static void AddUniqueOverlap(TArray<FOverlapResult>& OutOverlaps, const FOverlapResult& NewOverlap)
{
	// Look to see if we already have this overlap (based on component)
	for (int32 TestIdx = 0; TestIdx < OutOverlaps.Num(); TestIdx++)
	{
		FOverlapResult& Overlap = OutOverlaps[TestIdx];

		if (Overlap.ItemIndex == NewOverlap.ItemIndex && Overlap.Component == NewOverlap.Component)
		{
			// These should be the same if the component matches!
			checkSlow(Overlap.OverlapObjectHandle == NewOverlap.OverlapObjectHandle);

			// If we had a non-blocking overlap with this component, but now we have a blocking one, use that one instead!
			if (!Overlap.bBlockingHit && NewOverlap.bBlockingHit)
			{
				Overlap = NewOverlap;
			}

			return;
		}
	}

	// Not found, so add it 
	OutOverlaps.Add(NewOverlap);
}

bool IsBlocking(const FPhysicsShape& Shape, const FCollisionFilterData& QueryFilter)
{
	using namespace ChaosInterface;

	// See if this is a 'blocking' hit
	const FCollisionFilterData ShapeFilter = GetQueryFilterData(Shape);
	const ECollisionQueryHitType HitType = FCollisionQueryFilterCallback::CalcQueryHitType(QueryFilter, ShapeFilter);
	const bool bBlock = (HitType == ECollisionQueryHitType::Block);
	return bBlock;
}

/** Min number of overlaps required to start using a TMap for deduplication */
int32 GNumOverlapsRequiredForTMap = 3;

static FAutoConsoleVariableRef GTestOverlapSpeed(
	TEXT("Engine.MinNumOverlapsToUseTMap"),
	GNumOverlapsRequiredForTMap,
	TEXT("Min number of overlaps required before using a TMap for deduplication")
);

template <typename THitOverlap>
bool ConvertOverlapResultsImp(int32 NumOverlaps, THitOverlap* OverlapResults, const FCollisionFilterData& QueryFilter, TArray<FOverlapResult>& OutOverlaps)
{
	SCOPE_CYCLE_COUNTER(STAT_CollisionConvertOverlap);

	const int32 ExpectedSize = OutOverlaps.Num() + NumOverlaps;
	OutOverlaps.Reserve(ExpectedSize);
	bool bBlockingFound = false;
	constexpr bool bIsGTQuery = std::is_same<THitOverlap, FHitOverlap>::value;

	if (ExpectedSize >= GNumOverlapsRequiredForTMap)
	{
		// Map from an overlap to the position in the result array (the index has one added to it so 0 can be a sentinel)
		TMap<FOverlapKey, int32, TInlineSetAllocator<64>> OverlapMap;
		OverlapMap.Reserve(ExpectedSize);

		// Fill in the map with existing hits
		for (int32 ExistingIndex = 0; ExistingIndex < OutOverlaps.Num(); ++ExistingIndex)
		{
			const FOverlapResult& ExistingOverlap = OutOverlaps[ExistingIndex];
			OverlapMap.Add(FOverlapKey(ExistingOverlap.Component.Get(), ExistingOverlap.ItemIndex), ExistingIndex + 1);
		}

		for (int32 PResultIndex = 0; PResultIndex < NumOverlaps; ++PResultIndex)
		{
			FOverlapResult NewOverlap;
			const auto Actor = GetActor(OverlapResults[PResultIndex]);
			const FPhysicsShape* Shape = GetShape(OverlapResults[PResultIndex]);

			const bool bBlock = IsBlocking(*Shape, QueryFilter);
			NewOverlap.bBlockingHit = bBlock;
			bBlockingFound |= bBlock;

			const FPhysicsActor* GTActor = GetGTActor(Actor);
			const FPhysicsShape* GTShape = GetGTShape<bIsGTQuery>(Shape, GTActor);
			if (GTActor && GTShape)
			{
				ConvertQueryOverlap(*GTShape, *GTActor, NewOverlap, QueryFilter);
			}

			// Look for it in the map, newly added elements will start with 0, so we know we need to add it to the results array then (the index is stored as +1)
			//TODO: this doesn't de-duplicate if FOverlapResult has no component - like in PT for example. If we care about de-duplication we should use the PT handle if needed
			//Question: why do we de-duplicate? This seems expensive and is not consistent with traces. The user could handle this at their level anyway (at the cost of a bit of transient memory here)
			//TODO: make sure it doesn't de-duplicate null results
			int32& DestinationIndex = OverlapMap.FindOrAdd(FOverlapKey(NewOverlap.Component.Get(), NewOverlap.ItemIndex));
			if (DestinationIndex == 0)
			{
				DestinationIndex = OutOverlaps.Add(NewOverlap) + 1;
			}
			else
			{
				FOverlapResult& ExistingOverlap = OutOverlaps[DestinationIndex - 1];

				// If we had a non-blocking overlap with this component, but now we have a blocking one, use that one instead!
				if (!ExistingOverlap.bBlockingHit && NewOverlap.bBlockingHit)
				{
					ExistingOverlap = NewOverlap;
				}
			}
		}
	}
	else
	{
		// N^2 approach, no maps
		for (int32 i = 0; i < NumOverlaps; i++)
		{
			FOverlapResult NewOverlap;
			const auto Actor = GetActor(OverlapResults[i]);
			const FPhysicsShape* Shape = GetShape(OverlapResults[i]);

			const bool bBlock = IsBlocking(*Shape, QueryFilter);
			NewOverlap.bBlockingHit = bBlock;
			bBlockingFound |= bBlock;

			const FPhysicsActor* GTActor = GetGTActor(Actor);
			const FPhysicsShape* GTShape = GetGTShape<bIsGTQuery>(Shape, GTActor);
			if (GTActor && GTShape)
			{
				ConvertQueryOverlap(*GTShape, *GTActor, NewOverlap, QueryFilter);
			}

			AddUniqueOverlap(OutOverlaps, NewOverlap);
		}
	}

	return bBlockingFound;
}

bool ConvertOverlapResults(int32 NumOverlaps, FHitOverlap* OverlapResults, const FCollisionFilterData& QueryFilter, TArray<FOverlapResult>& OutOverlaps)
{
	return ConvertOverlapResultsImp(NumOverlaps, OverlapResults, QueryFilter, OutOverlaps);
}

bool ConvertOverlapResults(int32 NumOverlaps, ChaosInterface::FPTOverlapHit* OverlapResults, const FCollisionFilterData& QueryFilter, TArray<FOverlapResult>& OutOverlaps)
{
	return ConvertOverlapResultsImp(NumOverlaps, OverlapResults, QueryFilter, OutOverlaps);
}

FHitResult ConvertOverlapToHitResult(const FOverlapResult& Overlap)
{
	FHitResult Hit;
	Hit.bBlockingHit = Overlap.bBlockingHit;
	Hit.Item = Overlap.ItemIndex;
	Hit.Component = Overlap.Component;
	Hit.HitObjectHandle = Overlap.OverlapObjectHandle;
	return Hit;
}
#endif // IS_MONOLITHIC