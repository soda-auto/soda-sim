// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/VehicleComponents/WheeledVehicleMovements/Soda2DWheeledVehicleMovement.h"
#include "VehicleComponents/WheeledVehicleMovements/Bicycle/dyncar_bicycle.h"
#include "Soda/UnrealSoda.h"
#include "Soda/SodaApp.h"
#include "Engine/Engine.h"
#include "Engine/Canvas.h"
#include "Engine/CollisionProfile.h"
#include "Soda/Misc/Utils.h"
#include "Soda/SodaApp.h"
#include "Soda/SodaUserSettings.h"
#include "Components/SkeletalMeshComponent.h"

DECLARE_CYCLE_STAT(TEXT("BicycleCompute"), STAT_BicycleCompute, STATGROUP_SodaVehicle);
DECLARE_CYCLE_STAT(TEXT("SimulationTick"), STAT_UpdateSimulationTick, STATGROUP_SodaVehicle);

static inline float CalcWheelSpeed(float Vt, float Slip, float R)
{
	return (Vt / (1.0 - Slip) - 0.001 * Slip / (1 - Slip)) / R;
}

USoda2DWheeledVehicleMovementComponent::USoda2DWheeledVehicleMovementComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	GUI.ComponentNameOverride = TEXT("Soda 2D Vehicle");

	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PostPhysics;
	bWantsInitializeComponent = true;
}

void USoda2DWheeledVehicleMovementComponent::OnPreActivateVehicleComponent()
{
	Super::OnPreActivateVehicleComponent();

	if (!GetWheeledVehicle()->IsXWDVehicle(4))
	{
		GetWheeledVehicle()->GetWheelByIndex(EWheelIndex::FL)->Radius = FrontWheelRadius;
		GetWheeledVehicle()->GetWheelByIndex(EWheelIndex::FR)->Radius = FrontWheelRadius;
		GetWheeledVehicle()->GetWheelByIndex(EWheelIndex::RL)->Radius = RearWheelRadius;
		GetWheeledVehicle()->GetWheelByIndex(EWheelIndex::RR)->Radius = RearWheelRadius;
	}

	OnSetActiveMovement();
}


bool USoda2DWheeledVehicleMovementComponent::OnActivateVehicleComponent()
{
	if (!Super::OnActivateVehicleComponent())
	{
		return false;
	}

	check(GetWheeledVehicle());

	if (!GetWheeledVehicle()->IsXWDVehicle(4))
	{
		SetHealth(EVehicleComponentHealth::Error, TEXT("Support only 4WD vehicles"));
		return false;
	}

	USkinnedMeshComponent* Mesh = Cast< USkinnedMeshComponent >(UpdatedPrimitive);
	if (!Mesh)
	{
		SetHealth(EVehicleComponentHealth::Error);
		UE_LOG(LogSoda, Error, TEXT("USoda2DWheeledVehicleMovementComponent::OnActivateVehicleComponent(). Forgot to set Mesh to USoda2DWheeledVehicleMovementComponent?"));
		return false;
	}

	Mesh->SetEnableGravity(false);

	if (!bEnableCollisions)
	{
		Mesh->SetSimulatePhysics(false);
		Mesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Overlap);
		Mesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	}

	check(!DynCar);
	DynCar = MakeShared<DynamicCar>(Eigen::Vector3d::Zero(3));
	DynamicPar dynpar = DynamicPar();

	DynCar->culc_par = dynpar.culc_par;
	DynCar->car_par = dynpar.car_par;
	DynCar->culc_par.fl_drag_taking = FlDragTaking;
	DynCar->culc_par.fl_Long_Circ = FlLongCirc;
	DynCar->culc_par.fl_static_load = FlStaticLoad;
	DynCar->culc_par.fl_RunKut = FlRunKut;
	DynCar->culc_par.nsteps = NSteps;
	DynCar->culc_par.fl_act_low = FlActLow;
	DynCar->culc_par.implicit_solver = ImplicitSolver;
	DynCar->culc_par.num_impl_nonlin_it = NumImpLonlinIt;
	DynCar->culc_par.corr_impl_step = CorrImplStep;
	DynCar->car_par.a = CoGToForwardWheel / 100.0;
	DynCar->car_par.b = CoGToRearWheel / 100.0;
	DynCar->car_par.L = (CoGToForwardWheel + CoGToRearWheel) / 100.0;
	DynCar->car_par.mass = Mass;
	DynCar->car_par.moment_inertia = MomentOfInertia;
	DynCar->car_par.CGvert = CoGVert / 100;
	DynCar->car_par.mu = Friction;
	DynCar->car_par.Cy_r = RearWheelLatStiff;
	DynCar->car_par.Cy_f = ForwardWheelLatStiff;
	DynCar->car_par.Cx_r = RearWheelLonStiff;
	DynCar->car_par.Cx_f = ForwardWheelLonStiff;
	DynCar->car_par.rearWheelRad = RearWheelRadius / 100;
	DynCar->car_par.frontWheelRad = FrontWheelRadius / 100;
	DynCar->car_par.rollingResistance = DragRollingResistance;
	DynCar->car_par.rollingResistanceSpeedDepended = DragRollingResistanceSpeedDepended;
	DynCar->car_par.Cxx = DragCxx;
	DynCar->car_par.fWIn = MomentOfInertiaFrontTrain;
	DynCar->car_par.rWIn = MomentOfInertiaRearTrain;

	
	
	const FTransform Transform = UpdatedPrimitive->GetComponentTransform();
	const FVector CoFWorld = Transform.TransformPosition(CoF);

	CoF = RearWheelOffset + FVector(CoGToRearWheel, 0.0, CoGVert);
	ZOffset = Transform.GetTranslation().Z;
	VehicleSimData.VehicleKinematic.Curr.GlobalPose = Transform; 

	DynCar->init_nav(CoFWorld.X / 100.0, -CoFWorld.Y / 100.0, -Transform.GetRotation().Rotator().Yaw / 180.0 * M_PI, 0.0);

	bSynchronousMode = SodaApp.IsSynchronousMode();
	PrecisionTimer.TimerDelegate.BindLambda([this](const std::chrono::nanoseconds& InDeltatime, const std::chrono::nanoseconds& Elapsed)
	{
		UpdateSimulation(std::chrono::nanoseconds(IntegrationTimeStep * 1000000), Elapsed);
	});	

	if (USkeletalMeshComponent* SkeletalMesh = Cast<USkeletalMeshComponent>(UpdatedComponent))
	{
		SkeletalMesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;
		SkeletalMesh->InitAnim(true);
	}

	return true;
}

void USoda2DWheeledVehicleMovementComponent::OnDeactivateVehicleComponent()
{
	Super::OnDeactivateVehicleComponent();
	DynCar.Reset();
}

void USoda2DWheeledVehicleMovementComponent::OnPreDeactivateVehicleComponent()
{
	Super::OnPreDeactivateVehicleComponent();
	PrecisionTimer.TimerStop();
}

void USoda2DWheeledVehicleMovementComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (!HealthIsWorkable()) return;

	if (!bSynchronousMode)
	{
		if (!PrecisionTimer.IsJoinable())
		{
			PrecisionTimer.TimerStart(std::chrono::duration<double>(double(IntegrationTimeStep) / 1000.0 / SpeedFactor), std::chrono::duration<float>(ModelStartUpDelay));
			return;
		}
	}
	else
	{
		const std::chrono::nanoseconds dt = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::duration<float>(DeltaTime));
		UpdateSimulation(dt, std::chrono::nanoseconds::zero());
	}

	FScopeLock ScopeLock(&WheeledVehicle->PhysicMutex);

	VehicleSimData.RenderStep = VehicleSimData.SimulatedStep;
	VehicleSimData.RenderTimestamp = VehicleSimData.SimulatedTimestamp;

	if (bPullToGround)
	{
		const FVector RefPoint = VehicleSimData.VehicleKinematic.Curr.GlobalPose.TransformPosition(RearWheelOffset + FVector(CoGToRearWheel, 0.0, 0.0));

		FHitResult Hit;
		GetWorld()->LineTraceSingleByChannel(
			Hit, 
			RefPoint + FVector(0, 0, 50), RefPoint + FVector(0, 0, -100),
			ECollisionChannel::ECC_WorldDynamic,
			FCollisionQueryParams(NAME_None, false, GetOwner()));

		if (Hit.bBlockingHit)
		{
			ZOffset = Hit.Location.Z - RefPoint.Z + PullToGroundOffset;
			VehicleSimData.VehicleKinematic.Curr.GlobalPose.AddToTranslation(FVector(0, 0, ZOffset));
		}
	}

	if (UpdatedPrimitive)
	{
		const FVector NewLoc = VehicleSimData.VehicleKinematic.Curr.GlobalPose.GetTranslation();
		const FRotator NewRot = VehicleSimData.VehicleKinematic.Curr.GlobalPose.Rotator();

		const FVector AngVel0 = UpdatedPrimitive->GetPhysicsAngularVelocityInRadians();
		const FVector LineerVel0 = UpdatedPrimitive->GetPhysicsLinearVelocity();

		FHitResult Hit;
		UpdatedPrimitive->SetPhysicsLinearVelocity(VehicleSimData.VehicleKinematic.Curr.GlobalVelocityOfCenterMass);
		UpdatedPrimitive->SetPhysicsAngularVelocityInRadians(VehicleSimData.VehicleKinematic.Curr.AngularVelocity);
		UpdatedPrimitive->SetWorldLocationAndRotation(NewLoc, NewRot, false, &Hit, ETeleportType::TeleportPhysics);

		if (bEnableCollisions)
		{
			const FVector NewLoc1 = UpdatedPrimitive->GetComponentToWorld().GetLocation();
			const FQuat NewRot1 = UpdatedPrimitive->GetComponentToWorld().GetRotation();

			const FVector DiffLoc = NewLoc - NewLoc1;
			const FQuat DiffRot = NewRot.Quaternion() - NewRot1;
			const FVector LocalVel = NewRot1.UnrotateVector(LineerVel0);

			if (DiffLoc.Size() > 1.0 || Hit.bBlockingHit)
			{
				UE_LOG(LogSoda, Warning, TEXT("Bicycle hit detected"));

				FTransform Transform = UpdatedPrimitive->GetComponentTransform();
				FVector WorldLoc = Transform.TransformPosition(FVector(CoGToRearWheel, 0.0, 0.0));

				DynCar->car_state.x = WorldLoc.X / 100.0;
				DynCar->car_state.y = -WorldLoc.Y / 100.0;
				DynCar->car_state.psi = -NewRot1.Rotator().Yaw / 180.0 * M_PI;

				DynCar->car_state.Vx = LocalVel.X / 100;
				DynCar->car_state.Vy = -LocalVel.Y / 100;
				DynCar->car_state.Om = -AngVel0.Z;

				VehicleSimData.VehicleKinematic.Curr.AngularVelocity = AngVel0;
				VehicleSimData.VehicleKinematic.Curr.GlobalVelocityOfCenterMass = LineerVel0;
				VehicleSimData.VehicleKinematic.Curr.GlobalPose = UpdatedPrimitive->GetComponentToWorld();
			}
		}
	}
}

void USoda2DWheeledVehicleMovementComponent::UpdateSimulation(const std::chrono::nanoseconds& InDeltatime, const std::chrono::nanoseconds& Elapsed)
{
	//Check Deltatime
	/*
	static auto PrevTime = std::chrono::steady_clock::now();
	auto CurrTime = std::chrono::steady_clock::now();
	UE_LOG(LogSoda, Error, TEXT("***** %i"), std::chrono::duration_cast<std::chrono::microseconds>(CurrTime - PrevTime).count());
	PrevTime = CurrTime;
	*/

	if (Elapsed > std::chrono::nanoseconds::zero())
	{
		UE_LOG(LogSoda, Warning, TEXT("USoda2DWheeledVehicleMovementComponent::UpdateSimulation().Realtime expired by %i micro sec"), std::chrono::duration_cast<std::chrono::microseconds>(Elapsed).count());
	}

	//FScopeLock ScopeLock(&WheeledVehicle->PhysicMutex);
	while (!WheeledVehicle->PhysicMutex.TryLock() && PrecisionTimer.IsTimerWorking())
	{
		FPlatformProcess::Yield();
	}

	float DeltaTime = (std::chrono::duration_cast<std::chrono::duration<float>>(InDeltatime)).count();

	PrePhysicSimulation(DeltaTime, VehicleSimData.VehicleKinematic, VehicleSimData.SimulatedTimestamp);

    {
		const float Steer = -(GetWheeledVehicle()->GetWheelByIndex(EWheelIndex::FL)->ReqSteer + GetWheeledVehicle()->GetWheelByIndex(EWheelIndex::FR)->ReqSteer) / 2;
		const float FrontTorq = GetWheeledVehicle()->GetWheelByIndex(EWheelIndex::FL)->ReqTorq + GetWheeledVehicle()->GetWheelByIndex(EWheelIndex::FR)->ReqTorq;
		const float RearTorq = GetWheeledVehicle()->GetWheelByIndex(EWheelIndex::RL)->ReqTorq + GetWheeledVehicle()->GetWheelByIndex(EWheelIndex::RR)->ReqTorq;
		const float BrakeFrontTorq = GetWheeledVehicle()->GetWheelByIndex(EWheelIndex::FL)->ReqBrakeTorque + GetWheeledVehicle()->GetWheelByIndex(EWheelIndex::FR)->ReqBrakeTorque;
		const float BrakeRearTorq = GetWheeledVehicle()->GetWheelByIndex(EWheelIndex::RL)->ReqBrakeTorque + GetWheeledVehicle()->GetWheelByIndex(EWheelIndex::RR)->ReqBrakeTorque;

		// UE_LOG(LogSoda, Warning, TEXT("USoda2DWheeledVehicleMovementComponent::UpdateSimulation(). %.2f; %.2f"), FrontTorq, RearTorq);

		SCOPE_CYCLE_COUNTER(STAT_BicycleCompute);
		DynCar->dynamicNav_ev_Base(DeltaTime, Steer, FrontTorq, RearTorq, BrakeFrontTorq, BrakeRearTorq);
	}

	const FVector2D V_FL(DynCar->car_state.Vx - TrackWidth / 100.0 / 2.0 * DynCar->car_state.Om, DynCar->car_state.Vy + CoGToForwardWheel * DynCar->car_state.Om / 100.0);
	const FVector2D V_RL(DynCar->car_state.Vx - TrackWidth / 100.0 / 2.0 * DynCar->car_state.Om, DynCar->car_state.Vy - CoGToRearWheel * DynCar->car_state.Om / 100.0);
	const FVector2D V_RR(DynCar->car_state.Vx + TrackWidth / 100.0 / 2.0 * DynCar->car_state.Om, DynCar->car_state.Vy - CoGToRearWheel * DynCar->car_state.Om / 100.0);
	const FVector2D V_FR(DynCar->car_state.Vx + TrackWidth / 100.0 / 2.0 * DynCar->car_state.Om, DynCar->car_state.Vy + CoGToForwardWheel * DynCar->car_state.Om / 100.0);
	const float Vt_FL = V_FL.GetRotated(-DynCar->car_state.steer_alpha / M_PI * 180.0).X;
	const float Vt_FR = V_FR.GetRotated(-DynCar->car_state.steer_alpha / M_PI * 180.0).X;

	GetWheeledVehicle()->GetWheelByIndex(EWheelIndex::FL)->AngularVelocity = CalcWheelSpeed(Vt_FL, DynCar->car_state.long_slip_f, FrontWheelRadius / 100.0);
	GetWheeledVehicle()->GetWheelByIndex(EWheelIndex::FR)->AngularVelocity = CalcWheelSpeed(Vt_FR, DynCar->car_state.long_slip_f, FrontWheelRadius / 100.0);
	GetWheeledVehicle()->GetWheelByIndex(EWheelIndex::RL)->AngularVelocity = CalcWheelSpeed(V_RL.X, DynCar->car_state.long_slip_r, RearWheelRadius / 100.0);
	GetWheeledVehicle()->GetWheelByIndex(EWheelIndex::RR)->AngularVelocity = CalcWheelSpeed(V_RR.X, DynCar->car_state.long_slip_r, RearWheelRadius / 100.0);

	for(auto & Wheel: GetWheeledVehicle()->GetWheelsSorted())
	{
		Wheel->Pitch = NormAngRad(Wheel->Pitch - Wheel->AngularVelocity * DeltaTime);
		Wheel->SuspensionOffset2 = FVector::ZeroVector;
	}
	//FQuat PreQuat = VehicleSimData.VehicleKinematic.Curr.GlobalPose.GetRotation();

	FRotator Rot = VehicleSimData.VehicleKinematic.Curr.GlobalPose.Rotator();
	Rot.Yaw = -DynCar->car_state.psi / M_PI * 180.f;

	const FVector Pos = FVector( DynCar->car_state.xc * 100, -DynCar->car_state.yc * 100, ZOffset) + FRotator(0, Rot.Yaw, 0).RotateVector(FVector(CoF.X, CoF.Y, 0));
	
	GetWheeledVehicle()->GetWheelByIndex(EWheelIndex::FL)->Steer = -DynCar->car_state.steer_alpha;
	GetWheeledVehicle()->GetWheelByIndex(EWheelIndex::FR)->Steer = -DynCar->car_state.steer_alpha;

	GetWheeledVehicle()->GetWheelByIndex(EWheelIndex::FL)->Slip = GetWheeledVehicle()->GetWheelByIndex(EWheelIndex::FR)->Slip = FVector2D(DynCar->car_state.long_slip_f, DynCar->car_state.slipAng_f);
	GetWheeledVehicle()->GetWheelByIndex(EWheelIndex::RL)->Slip = GetWheeledVehicle()->GetWheelByIndex(EWheelIndex::RR)->Slip = FVector2D(DynCar->car_state.long_slip_r, DynCar->car_state.slipAng_r);

	VehicleSimData.VehicleKinematic.Push(DeltaTime);
	VehicleSimData.VehicleKinematic.Curr.GlobalPose = FTransform(Rot, Pos, FVector::ZeroVector);
	VehicleSimData.VehicleKinematic.Curr.GlobalVelocityOfCenterMass = FVector(DynCar->car_state.Vx * 100, -DynCar->car_state.Vy * 100, 0).RotateAngleAxis(-DynCar->car_state.psi / M_PI * 180.0, FVector(0.f, 0.f, 1.f));
	VehicleSimData.VehicleKinematic.Curr.CenterOfMassLocal = CoF;
	//VehicleKinematic.Curr.AngularVelocity = QLog((PreQuat.Inverse() * Rot.Quaternion()).GetNormalized()) / DeltaTime;
	VehicleSimData.VehicleKinematic.ComputeAngularVelocity();
	//VehicleKinematic.ComputeGlobalVelocityOfCenterMass(); 
	VehicleSimData.VehicleKinematic.ComputeGlobalAcceleration();
	++VehicleSimData.SimulatedStep;
	VehicleSimData.SimulatedTimestamp = bSynchronousMode ? SodaApp.GetSimulationTimestamp() : soda::Now();
	if(bLogPhysStemp)
	{
		UE_LOG(LogSoda, Warning, TEXT("2WDVehicle, SimulatedStep: %i, SimulatedTimestamp: %s, Pos: %s"), 
			VehicleSimData.SimulatedStep,
			*soda::ToString(VehicleSimData.SimulatedTimestamp),
			*Pos.ToString());
	}

	PostPhysicSimulation(DeltaTime, VehicleSimData.VehicleKinematic, VehicleSimData.SimulatedTimestamp);
	PostPhysicSimulationDeferred(DeltaTime, VehicleSimData.VehicleKinematic, VehicleSimData.SimulatedTimestamp);

	WheeledVehicle->PhysicMutex.Unlock();
}

void USoda2DWheeledVehicleMovementComponent::DrawDebug(UCanvas* Canvas, float& YL, float& YPos)
{
	Super::DrawDebug(Canvas, YL, YPos);

	if (Common.bDrawDebugCanvas && GetHealth() != EVehicleComponentHealth::Disabled)
	{
		UFont* RenderFont = GEngine->GetSmallFont();
		// draw drive data

		Canvas->SetDrawColor(FColor::Green);
		YPos += Canvas->DrawText(RenderFont, TEXT("2D Vehicle:"), 4, YPos + 4);

		Canvas->SetDrawColor(FColor::White);
		YPos += Canvas->DrawText(RenderFont, FString::Printf(TEXT("Vx (km/h): %.1f"), DynCar->car_state.Vx * 3.6), 16, YPos);
		YPos += Canvas->DrawText(RenderFont, FString::Printf(TEXT("Vy (km/h): %.1f"), DynCar->car_state.Vy * 3.6), 16, YPos);
		YPos += Canvas->DrawText(RenderFont, FString::Printf(TEXT("Acc x (m/s^2): %.1f"), DynCar->car_state.dVxdt), 16, YPos);
		YPos += Canvas->DrawText(RenderFont, FString::Printf(TEXT("Acc y (m/s^2): %.1f"), DynCar->car_state.dVydt), 16, YPos);

	}
}

bool USoda2DWheeledVehicleMovementComponent::SetVehiclePosition(const FVector& NewLocation, const FRotator& NewRotation)
{
	if (DynCar != nullptr)
	{
		VehicleSimData.VehicleKinematic.Curr.GlobalPose = FTransform(NewRotation, NewLocation, FVector(1.0, 1.0, 1.0));
		DynCar->init_nav(NewLocation.X / 100.0, -NewLocation.Y / 100.0, -NewRotation.Yaw / 180.0 * M_PI, 0.0);
		return true;
	}
	return false;
}

bool USoda2DWheeledVehicleMovementComponent::SetVehicleVelocity(float InVelocity)
{
	if (DynCar != nullptr)
	{
		DynCar->init_nav(DynCar->car_state.x, DynCar->car_state.y, DynCar->car_state.psi, InVelocity / 100.0);
		return true;
	}
	return false;
}
