// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "Math/Transform.h"
#include "Soda/Misc/Utils.h"

struct UNREALSODA_API FPhysBodyKinematic
{
	struct FState
	{
		FVector GlobalVelocityOfCenterMass = FVector(0.f);	// [cm/s]
		FVector GlobalAcceleration = FVector(0.f);			// [cm/c2]
		FVector AngularVelocity = FVector(0.f);				// [rad/s]
		FTransform GlobalPose;								// Center of rear axis [cm]
		FVector CenterOfMassLocal = FVector(0.f);			// Offset from rear axis [cm]

		inline FVector GetGlobalVelocityAtLocalPoint(const FVector& Point) const
		{
			return GlobalVelocityOfCenterMass + GlobalPose.Rotator().RotateVector((AngularVelocity ^ (Point - CenterOfMassLocal)));
		}

		inline FVector GetGlobaVelocityAtGlobalPoint(const FVector& Point) const
		{
			return GlobalVelocityOfCenterMass + (AngularVelocity ^ (Point - GlobalPose.TransformPosition(CenterOfMassLocal)));
		}

		inline FVector GetLocalVelocity() const
		{
			return GlobalPose.Rotator().UnrotateVector(GlobalVelocityOfCenterMass);
		}

		inline FVector GetLocalAcceleration() const
		{
			return GlobalPose.Rotator().UnrotateVector(GlobalAcceleration);
		}

		inline FVector GetLocalVelocityAtLocalPose(const FTransform& LocalPose) const
		{
			FTransform WorldPose = LocalPose * GlobalPose;
			return  WorldPose.Rotator().UnrotateVector(GetGlobalVelocityAtLocalPoint(LocalPose.GetTranslation()));
		}

		inline FVector GetLocalVelocityAtLocalPoint(const FVector& Point) const
		{
			return  GlobalPose.Rotator().UnrotateVector(GetGlobalVelocityAtLocalPoint(Point));
		}

		inline void EstimatePoseInternal(float InDeltatime, FTransform& WorldPose) const
		{
			FVector NewLocationOfCenterMass =
				GlobalPose.TransformPosition(CenterOfMassLocal) +
				GlobalVelocityOfCenterMass * InDeltatime + GlobalAcceleration * InDeltatime * InDeltatime / 2;

			//TODO: Use: NewRotatation = GlobalPose.GetRotation() * QExp(AngularVelocity * DeltaTime); NewRotatation.Normalize();
			FRotator NewRotatation =
				GlobalPose.Rotator() +
				FRotator(AngularVelocity.Y * InDeltatime / M_PI * 180.0, AngularVelocity.Z * InDeltatime / M_PI * 180.0, AngularVelocity.X * InDeltatime / M_PI * 180.0);

			WorldPose = FTransform(
				NewRotatation,
				NewLocationOfCenterMass - NewRotatation.RotateVector(CenterOfMassLocal),
				FVector(1.0));
		}
	};

	inline void Push(float InDeltatime)
	{
		Deltatime = InDeltatime;
		Prev = Curr;
	}

	inline void Push(float InDeltatime, const FState& NewState)
	{
		Deltatime = InDeltatime;
		Prev = Curr;
		Curr = NewState;
	}

	inline void ComputeGlobalVelocityOfCenterMass()
	{
		Curr.GlobalVelocityOfCenterMass = (Curr.GlobalPose.TransformPosition(Curr.CenterOfMassLocal) - Prev.GlobalPose.TransformPosition(Prev.CenterOfMassLocal)) / Deltatime;
	}

	inline void ComputeGlobalAcceleration()
	{
		Curr.GlobalAcceleration = (Curr.GlobalVelocityOfCenterMass - Prev.GlobalVelocityOfCenterMass) / Deltatime;
	}

	inline void ComputeAngularVelocity()
	{
		Curr.AngularVelocity = QLog((Prev.GlobalPose.GetRotation().Inverse() * Curr.GlobalPose.GetRotation()).GetNormalized()) / Deltatime;
	}

	inline FVector GetLocalAcc(float Gravity = -980.f) const
	{
		FVector LocalVel = Curr.GetLocalVelocity();
		return Curr.GlobalPose.Rotator().UnrotateVector(FVector(0.f, 0.f, Gravity)) +
			(Curr.AngularVelocity ^ LocalVel) +
			(Prev.GetLocalVelocity() - LocalVel) / Deltatime;
	}

	inline float GetForwardSpeed() const
	{
		return Curr.GetLocalVelocity().X;
	}

	inline void CalcIMU(const FTransform& LocalPose, FTransform& WorldPose, FVector& WorldVel, FVector& LocalAcc, FVector& Gyro, float Gravity = -980.f) const
	{
		WorldPose = LocalPose * Curr.GlobalPose;
		WorldVel = Curr.GetGlobalVelocityAtLocalPoint(LocalPose.GetTranslation());
		Gyro = LocalPose.Rotator().RotateVector(Curr.AngularVelocity);

	// Two methods of computing of the LocalAcc. Which is batter?
#if 1
		FVector LocalVel = WorldPose.Rotator().UnrotateVector(WorldVel);
		FVector PreLocalVel = Prev.GetLocalVelocityAtLocalPose(LocalPose);
		FVector dVdt = (PreLocalVel - LocalVel) / Deltatime;
		LocalAcc = WorldPose.Rotator().UnrotateVector(FVector(0.f, 0.f, Gravity)) - (Curr.AngularVelocity ^ LocalVel) + dVdt;
#else
		FVector PreWorldVel = Prev.GetGlobalVelocityAtLocalPoint(LocalPose.GetTranslation());
		LocalAcc = WorldPose.Rotator().UnrotateVector(FVector(0.f, 0.f, Gravity) + (PreWorldVel - WorldVel) / Deltatime);
#endif
	}

	/** Simple pose estimation based on Curr pose, velocity and acceleration*/
	inline virtual void EstimatePose(float InDeltatime, FTransform& WorldPose) const
	{
		Curr.EstimatePoseInternal(InDeltatime, WorldPose);
	}
public:

	FState Curr, Prev;
	float Deltatime = 1.f;
};

struct UNREALSODA_API FPhysBodyKinematicLerpEstm : public FPhysBodyKinematic
{
	inline void Push(float Deltatime_, const FState& NewState)
	{
		FTransform Pose;
		EstimatePose(Deltatime_, Pose);
		Estm = NewState;
		Estm.GlobalPose = Pose;	

		FPhysBodyKinematic::Push(Deltatime_, NewState);
	}

	/** Pose estimation based on interpolation from prev estimated pose to new estimated pose. Provides smooth motion without teleportations on push. Call Push with CalcEstmPose=true*/
	inline virtual void EstimatePose(float InDeltatime, FTransform& WorldPose) const
	{
		if (InDeltatime < Deltatime)
		{
			float Alpha = InDeltatime / Deltatime;
			FTransform PoseByEstm, PoseByCurr;
			Estm.EstimatePoseInternal(InDeltatime, PoseByEstm);
			Curr.EstimatePoseInternal(InDeltatime, PoseByCurr);
			FTransform LerpPose(
				FMath::Lerp(PoseByEstm.GetRotation(), PoseByCurr.GetRotation(), Alpha),
				FMath::Lerp(
					PoseByEstm.GetTranslation(),
					PoseByCurr.GetTranslation(),
					Alpha),
				FVector(1.0));
			WorldPose = LerpPose;
		}
		else
		{
			Curr.EstimatePoseInternal(InDeltatime, WorldPose);
		}
	}

	FState Estm;
};
