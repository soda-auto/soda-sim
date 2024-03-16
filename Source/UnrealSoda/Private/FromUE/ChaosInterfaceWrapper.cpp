// Copyright Epic Games, Inc. All Rights Reserved.
// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#if(!IS_MONOLITHIC)

#include "Physics/Experimental/ChaosInterfaceWrapper.h"
#include "Physics/Experimental/PhysScene_Chaos.h"
#include "Chaos/ParticleHandle.h"
#include "PhysxUserData.h"
#include "PBDRigidsSolver.h"


namespace ChaosInterface
{
	FBodyInstance* GetUserData(const Chaos::FGeometryParticle& Actor)
	{
		void* UserData = Actor.UserData();
		return UserData ? FChaosUserData::Get<FBodyInstance>(Actor.UserData()) : nullptr;
	}

	UPhysicalMaterial* GetUserData(const Chaos::FChaosPhysicsMaterial& Material)
	{
		void* UserData = Material.UserData;
		return UserData ? FChaosUserData::Get<UPhysicalMaterial>(UserData) : nullptr;
	}

	FScopedSceneReadLock::FScopedSceneReadLock(FPhysScene_Chaos& SceneIn)
		: Solver(SceneIn.GetSolver())
	{
		if(Solver)
		{
			Solver->GetExternalDataLock_External().ReadLock();
		}
	}

	FScopedSceneReadLock::~FScopedSceneReadLock()
	{
		if(Solver)
		{
			Solver->GetExternalDataLock_External().ReadUnlock();
		}
	}
}

#endif // IS_MONOLITHIC
