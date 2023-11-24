// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/VehicleComponents/WheeledVehicleMovements/Soda2DWheeledVehicleMovement.h"
#include "VehicleComponents/WheeledVehicleMovements/Bicycle/dyncar_bicycle.h"
#include "Soda/UnrealSoda.h"
#include "Soda/SodaApp.h"
#include "Engine/Engine.h"
#include "Engine/Canvas.h"
#include "Engine/CollisionProfile.h"
#include "Soda/Misc/MathUtils.hpp"
#include "Soda/SodaApp.h"
#include "Soda/SodaUserSettings.h"

DECLARE_CYCLE_STAT(TEXT("BicycleCompute"), STAT_BicycleCompute, STATGROUP_SodaVehicle);
DECLARE_CYCLE_STAT(TEXT("SimulationTick"), STAT_UpdateSimulationTick, STATGROUP_SodaVehicle);

static inline float CalcWheelSpeed(float Vt, float Slip, float R)
{
	return (Vt / (1.0 - Slip) - 0.001 * Slip / (1 - Slip)) / R;
}


/*
float Trapezoid(float dy0, float dy1, float dx)
{
	return (dy0 + dy1) * dx / 2.0;
}
*/


USoda2DWheeledVehicleMovementComponent::USoda2DWheeledVehicleMovementComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	GUI.ComponentNameOverride = TEXT("Soda 2WD Vehicle");

	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PostPhysics;
	bWantsInitializeComponent = true;

	TelemetryGrid.InitGrid(4, 4, 120, 120, 500);
	for(int i = 0; i < 4; ++i)
	{
		TelemetryGrid.InitCell(0, i, -5, 5, "", FString::Printf(TEXT("ToGround[%i]"), i));
		TelemetryGrid.InitCell(1, i , -1, 1, "", FString::Printf(TEXT("Tension[%i]"), i));
		TelemetryGrid.InitCell(2, i, -1, 1, "", FString::Printf(TEXT("ZVel[%i]"),     i));
		TelemetryGrid.InitCell(3, i , -1, 1, "", FString::Printf(TEXT("Damping[%i]"), i));
	}
}

void USoda2DWheeledVehicleMovementComponent::InitializeComponent()
{
	Super::InitializeComponent();
}

void USoda2DWheeledVehicleMovementComponent::UninitializeComponent()
{
	Super::UninitializeComponent();
}

void USoda2DWheeledVehicleMovementComponent::BeginPlay()
{
	Super::BeginPlay();
}

void USoda2DWheeledVehicleMovementComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
}

void USoda2DWheeledVehicleMovementComponent::OnPreActivateVehicleComponent()
{
	Super::OnPreActivateVehicleComponent();

	GetWheeledVehicle()->GetWheel4WD(E4WDWheelIndex::FL)->Radius = FrontWheelRadius * 100;
	GetWheeledVehicle()->GetWheel4WD(E4WDWheelIndex::FR)->Radius = FrontWheelRadius * 100;
	GetWheeledVehicle()->GetWheel4WD(E4WDWheelIndex::RL)->Radius = RearWheelRadius * 100;
	GetWheeledVehicle()->GetWheel4WD(E4WDWheelIndex::RR)->Radius = RearWheelRadius * 100;


	GroundScaner = NewObject< UGroundScaner>(this);

	OnSetActiveMovement();
}


bool USoda2DWheeledVehicleMovementComponent::OnActivateVehicleComponent()
{
	if (!Super::OnActivateVehicleComponent())
	{
		return false;
	}

	check(GetWheeledVehicle());

	if (!GetWheeledVehicle()->Is4WDVehicle())
	{
		SetHealth(EVehicleComponentHealth::Error, TEXT("Support only 4WD vehicles"));
		return false;
	}

	USkinnedMeshComponent* Mesh = Cast< USkinnedMeshComponent >(UpdatedPrimitive);
	if (!Mesh)
	{
		SetHealth(EVehicleComponentHealth::Error);
		UE_LOG(LogSoda, Error, TEXT("Forgot to set Mesh to USoda2DWheeledVehicleMovementComponent?"));
		return false;
	}

	if (WheelSetups.Num() != 4)
	{
		SetHealth(EVehicleComponentHealth::Error);
		UE_LOG(LogSoda, Error, TEXT("WheelSetups.Num() != 4 in USoda2DWheeledVehicleMovementComponent"));
		return false;
	}

	Mesh->SetEnableGravity(false);

	if (bDisableCollisions)
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
	DynCar->car_par.a = A;
	DynCar->car_par.b = B;
	DynCar->car_par.L = A + B;
	DynCar->car_par.mass = Mass;
	DynCar->car_par.moment_inertia = MomentOfInertia.Z;
	DynCar->car_par.CGvert = CGvert;
	DynCar->car_par.mu = Friction;
	DynCar->car_par.Cy_r = RearWheelLatStiff;
	DynCar->car_par.Cy_f = ForwardWheelLatStiff;
	DynCar->car_par.Cx_r = RearWheelLonStiff;
	DynCar->car_par.Cx_f = ForwardWheelLonStiff;
	DynCar->car_par.rearWheelRad = RearWheelRadius;
	DynCar->car_par.frontWheelRad = FrontWheelRadius;
	DynCar->car_par.rollingResistance = DragRollingResistance;
	DynCar->car_par.rollingResistanceSpeedDepended = DragRollingResistanceSpeedDepended;
	DynCar->car_par.Cxx = DragCxx;
	DynCar->car_par.fWIn = MomentOfInertiaFrontTrain;
	DynCar->car_par.rWIn = MomentOfInertiaRearTrain;
	
	FRotator WorldRot = UpdatedPrimitive->GetComponentRotation();
	FTransform Transform = UpdatedPrimitive->GetComponentTransform();
	FVector WorldLoc = Transform.TransformPosition(FVector(B * 100.0, 0.0, 0.0));
	VehicleSimData.VehicleKinematic.Curr.GlobalPose = FTransform(WorldRot, WorldLoc, FVector(1.0, 1.0, 1.0));
	DynCar->init_nav(WorldLoc.X / 100.0, -WorldLoc.Y / 100.0, -WorldRot.Yaw / 180.0 * M_PI, 0.0);
	GroundScaner->WheelBase = GetWheelBaseWidth();
	GroundScaner->TrackWidth = GetTrackWidth();
	GroundScaner->CollisionQueryParams = FCollisionQueryParams(NAME_None, false, GetOwner());
	bSynchronousMode = SodaApp.IsSynchronousMode();
	PrecisionTimer.TimerDelegate.BindLambda([this](const std::chrono::nanoseconds& Deltatime, const std::chrono::nanoseconds& Elapsed)
	{
		UpdateSimulation(Deltatime, Elapsed);
	});	

	return true;
}

void USoda2DWheeledVehicleMovementComponent::OnDeactivateVehicleComponent()
{
	Super::OnDeactivateVehicleComponent();

	PrecisionTimer.TimerStop();
	DynCar.Reset();
}

void USoda2DWheeledVehicleMovementComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (!HealthIsWorkable()) return;

	if (!bSynchronousMode)
	{
		if (!PrecisionTimer.IsJoinable())
		{
			PrecisionTimer.TimerStart(std::chrono::milliseconds(TimeStep), std::chrono::duration<float>(ModelStartUpDelay));
			return;
		}
	}
	else
	{
		const std::chrono::nanoseconds dt = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::duration<float>(DeltaTime));
		UpdateSimulation(dt, std::chrono::nanoseconds::zero());
	}

	VehicleSimData.RenderStep = VehicleSimData.SimulatedStep;
	VehicleSimData.RenderTimestamp = VehicleSimData.SimulatedTimestamp;

	FVehicleSimData VehicleSimDataCopy = VehicleSimData;

	FVector NewLoc = VehicleSimDataCopy.VehicleKinematic.Curr.GlobalPose.GetTranslation();
	FRotator NewRot = VehicleSimDataCopy.VehicleKinematic.Curr.GlobalPose.Rotator();
	float Steer = GetWheeledVehicle()->GetWheel4WD(E4WDWheelIndex::FL)->Steer / M_PI * 180;

	if (bLogPhysStemp)
	{
		UE_LOG(LogSoda, Warning, TEXT("2WDVehicle, RenderStep: %i, RenderTimestamp: %s , Pos: %s"),
			VehicleSimData.RenderStep,
			*soda::ToString(VehicleSimData.RenderTimestamp),
			*NewLoc.ToString());
	}

	if (UpdatedPrimitive)
	{
		FScopeLock ScopeLock(&WheeledVehicle->PhysicMutex);

		FVector AngVel0 = UpdatedPrimitive->GetPhysicsAngularVelocityInRadians();
		FVector LineerVel0 = UpdatedPrimitive->GetPhysicsLinearVelocity();

		FHitResult Hit;
		UpdatedPrimitive->SetPhysicsLinearVelocity(VehicleSimDataCopy.VehicleKinematic.Curr.GlobalVelocityOfCenterMass);
		UpdatedPrimitive->SetPhysicsAngularVelocityInRadians(VehicleSimDataCopy.VehicleKinematic.Curr.AngularVelocity);
		UpdatedPrimitive->SetWorldLocationAndRotation(NewLoc, NewRot, true, &Hit, ETeleportType::TeleportPhysics);

		FVector NewLoc1 = UpdatedPrimitive->GetComponentToWorld().GetLocation();
		FQuat NewRot1 = UpdatedPrimitive->GetComponentToWorld().GetRotation();

		FVector DiffLoc = NewLoc - NewLoc1;
		FQuat DiffRot = NewRot.Quaternion() - NewRot1;
		FVector LocalVel = NewRot1.UnrotateVector(LineerVel0);

		if (DiffLoc.Size() > 1.0 || Hit.bBlockingHit)
		{
			UE_LOG(LogSoda, Warning, TEXT("Bicycle hit detected"));

			FTransform Transform = UpdatedPrimitive->GetComponentTransform();
			FVector WorldLoc = Transform.TransformPosition(FVector(B * 100.0, 0.0, 0.0));

			DynCar->car_state.x = WorldLoc.X / 100.0;
			DynCar->car_state.y = -WorldLoc.Y / 100.0;
			DynCar->car_state.psi = -NewRot1.Rotator().Yaw / 180.0 * M_PI;

			DynCar->car_state.Vx = LocalVel.X / 100;
			DynCar->car_state.Vy = -LocalVel.Y / 100;
			DynCar->car_state.Om = -AngVel0.Z;

			VehicleSimData.VehicleKinematic.Curr.AngularVelocity = AngVel0;
			VehicleSimData.VehicleKinematic.Curr.GlobalVelocityOfCenterMass = LineerVel0;
			VehicleSimData.VehicleKinematic.Curr.GlobalPose = UpdatedPrimitive->GetComponentToWorld();

			ZDotDot = 0;
			PitchDotDot = 0;
			RollDotDot = 0;

			ZDot = LineerVel0.Z;
			PitchDot = AngVel0.Y;
			RollDot = AngVel0.X;
			
		}
	}



	if(bUseGroundScaner)
		GroundScaner->Scan(VehicleSimDataCopy.VehicleKinematic, DeltaTime);
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
		const float Steer = -(GetWheeledVehicle()->GetWheel4WD(E4WDWheelIndex::FL)->ReqSteer + GetWheeledVehicle()->GetWheel4WD(E4WDWheelIndex::FR)->ReqSteer) / 2;
		const float FrontTorq = GetWheeledVehicle()->GetWheel4WD(E4WDWheelIndex::FL)->ReqTorq + GetWheeledVehicle()->GetWheel4WD(E4WDWheelIndex::FR)->ReqTorq;
		const float RearTorq = GetWheeledVehicle()->GetWheel4WD(E4WDWheelIndex::RL)->ReqTorq + GetWheeledVehicle()->GetWheel4WD(E4WDWheelIndex::RR)->ReqTorq;
		const float BrakeFrontTorq = GetWheeledVehicle()->GetWheel4WD(E4WDWheelIndex::FL)->ReqBrakeTorque + GetWheeledVehicle()->GetWheel4WD(E4WDWheelIndex::FR)->ReqBrakeTorque;
		const float BrakeRearTorq = GetWheeledVehicle()->GetWheel4WD(E4WDWheelIndex::RL)->ReqBrakeTorque + GetWheeledVehicle()->GetWheel4WD(E4WDWheelIndex::RR)->ReqBrakeTorque;

		// UE_LOG(LogSoda, Warning, TEXT("USoda2DWheeledVehicleMovementComponent::UpdateSimulation(). %.2f; %.2f"), FrontTorq, RearTorq);

		SCOPE_CYCLE_COUNTER(STAT_BicycleCompute);
		DynCar->dynamicNav_ev_Base(DeltaTime, Steer, FrontTorq, RearTorq, BrakeFrontTorq, BrakeRearTorq);
	}

	FVector2D V_FL(DynCar->car_state.Vx - TrackWidth / 2 * DynCar->car_state.Om, DynCar->car_state.Vy + A * DynCar->car_state.Om);
	FVector2D V_RL(DynCar->car_state.Vx - TrackWidth / 2 * DynCar->car_state.Om, DynCar->car_state.Vy - B * DynCar->car_state.Om);
	FVector2D V_RR(DynCar->car_state.Vx + TrackWidth / 2 * DynCar->car_state.Om, DynCar->car_state.Vy - B * DynCar->car_state.Om);
	FVector2D V_FR(DynCar->car_state.Vx + TrackWidth / 2 * DynCar->car_state.Om, DynCar->car_state.Vy + A * DynCar->car_state.Om);
	float Vt_FL = V_FL.GetRotated(-DynCar->car_state.steer_alpha / M_PI * 180.0).X;
	float Vt_FR = V_FR.GetRotated(-DynCar->car_state.steer_alpha / M_PI * 180.0).X;

	GetWheeledVehicle()->GetWheel4WD(E4WDWheelIndex::FL)->AngularVelocity = CalcWheelSpeed(Vt_FL, DynCar->car_state.long_slip_f, FrontWheelRadius );
	GetWheeledVehicle()->GetWheel4WD(E4WDWheelIndex::FR)->AngularVelocity = CalcWheelSpeed(Vt_FR, DynCar->car_state.long_slip_f, FrontWheelRadius );
	GetWheeledVehicle()->GetWheel4WD(E4WDWheelIndex::RL)->AngularVelocity = CalcWheelSpeed(V_RL.X, DynCar->car_state.long_slip_r, RearWheelRadius );
	GetWheeledVehicle()->GetWheel4WD(E4WDWheelIndex::RR)->AngularVelocity = CalcWheelSpeed(V_RR.X, DynCar->car_state.long_slip_r, RearWheelRadius );

	for(int i = 0; i < 4; i++)
	{
		USodaVehicleWheelComponent * Wheel = GetWheeledVehicle()->GetWheel4WD(E4WDWheelIndex(i));
		Wheel->Pitch = NormAngRad(Wheel->Pitch - Wheel->AngularVelocity * DeltaTime);
	}

	FVector Pos = VehicleSimData.VehicleKinematic.Curr.GlobalPose.GetTranslation();
	FRotator Rot = VehicleSimData.VehicleKinematic.Curr.GlobalPose.Rotator();
	FQuat PreQuat = VehicleSimData.VehicleKinematic.Curr.GlobalPose.GetRotation();

	Pos.X = DynCar->car_state.xc * 100;
	Pos.Y = -DynCar->car_state.yc * 100;
	Rot.Yaw = -DynCar->car_state.psi / M_PI * 180.f;

	const float DZ = ZDot * DeltaTime;
	const float DPitch = (PitchDot * DeltaTime / M_PI * 180.f);
	const float DRoll = (RollDot * DeltaTime / M_PI * 180.f);

	const FVector CoF = FVector(B * 100.0, 0.0, 0.0);

	if(bUseGroundScaner)
	{
		FTransform GlobalPose = FTransform(Rot, Pos, FVector(1.0, 1.0, 1.0));

		float R =-Mass * Gravity;
		FVector M(0);

		for(int i = 0; i < 4; i++)
		{
			USodaVehicleWheelComponent * SodaWheel = GetWheeledVehicle()->GetWheel4WD(E4WDWheelIndex(i));

			FVector Pt = GlobalPose.TransformPosition(SodaWheel->RestingLocation);
			
			float GroundLevel;
			if(!GroundScaner->GetHeight(Pt.X, Pt.Y, GroundLevel))  GroundLevel = -FLT_MAX;

			float PrevToGround = WheelsData[i].ToGround;
			WheelsData[i].ToGround = SodaWheel->Radius - (Pt.Z - GroundLevel);
			SodaWheel->SuspensionOffset = FMath::Clamp(WheelsData[i].ToGround, -WheelSetups[i].SuspensionRise, WheelSetups[i].SuspensionFall);
			WheelsData[i].ToGroundVelocity = FMath::Clamp( float((PrevToGround - WheelsData[i].ToGround ) / DeltaTime), -300.f, 300.f);
			
			float TensionFactor = 0;
			float DampingFactor = 0;

			if (WheelsData[i].ToGround < SodaWheel->Radius  + WheelSetups[i].SuspensionRise )
			{
				TensionFactor = FMath::Clamp(float((WheelsData[i].ToGround - WheelSetups[i].SuspensionRise) / (WheelSetups[i].SuspensionFall + WheelSetups[i].SuspensionRise) + 1.f) * 2.f, 0.f, 2.f) * WheelSetups[i].SuspensionDist;
				if (WheelsData[i].ToGroundVelocity >= 0)
				{
					if (WheelsData[i].ToGround > -WheelSetups[i].SuspensionFall && WheelsData[i].ToGround < WheelSetups[i].SuspensionRise)
					{
						DampingFactor = -WheelSetups[i].DampingFallFactor;
					}
				}
				else
				{
					if (WheelsData[i].ToGround > -WheelSetups[i].SuspensionFall)
					{
						DampingFactor = WheelSetups[i].DampingRiseFactor;
					}
					/*
					if (Wheels[i].ToGround > WheelSetups[i].SuspensionRise)
					{
						DampingFactor += 0.05 * (Wheels[i].ToGround -  WheelSetups[i].SuspensionRise);
						DampingFactor = FMath::Clamp(DampingFactor, 0.f, 0.5f);
					}*/
				}
			}

			WheelsData[i].SpringForce = Mass * Gravity * TensionFactor;
			WheelsData[i].DampingForce = std::abs(WheelsData[i].ToGroundVelocity ) * Mass / DeltaTime * DampingFactor;

			float FullRorce = WheelsData[i].SpringForce + WheelsData[i].DampingForce;

			FVector r = SodaWheel->RestingLocation - CoF;

			R += FullRorce;
			M += (r ^ FVector(0.0f, 0.0, FullRorce));

			//UE_LOG(LogSoda, Error, TEXT("ind: %i; M: %s R: %f: FullRorce: %f"), i, *M.ToString(), R, FullRorce);

			if(bDrawTelemetry)
			{
				TelemetryGrid.AddPoint(0, i, WheelsData[i].ToGround);
				TelemetryGrid.AddPoint(1, i, TensionFactor);
				TelemetryGrid.AddPoint(2, i, WheelsData[i].ToGroundVelocity  / 100);
				TelemetryGrid.AddPoint(3, i, DampingFactor);
			}
		}

		ZDotDot = ZDotDot * GlobalDampingFactor  +  R/Mass * (1.f - GlobalDampingFactor);
		ZDot += (ZDotDot * DeltaTime);

		PitchDotDot =  PitchDotDot * GlobalDampingFactor - M.Y / MomentOfInertia.Y / 100000 * (1.f - GlobalDampingFactor);
		PitchDot += (PitchDotDot * DeltaTime);

		RollDotDot =  RollDotDot * GlobalDampingFactor - M.X / MomentOfInertia.X / 100000 * (1.f - GlobalDampingFactor);
		RollDot += (RollDotDot * DeltaTime);
	}

	GetWheeledVehicle()->GetWheel4WD(E4WDWheelIndex::FL)->Steer = -DynCar->car_state.steer_alpha;
	GetWheeledVehicle()->GetWheel4WD(E4WDWheelIndex::FR)->Steer = -DynCar->car_state.steer_alpha;

	GetWheeledVehicle()->GetWheel4WD(E4WDWheelIndex::FL)->Slip = GetWheeledVehicle()->GetWheel4WD(E4WDWheelIndex::FR)->Slip = FVector2D(DynCar->car_state.long_slip_f, DynCar->car_state.slipAng_f);
	GetWheeledVehicle()->GetWheel4WD(E4WDWheelIndex::RL)->Slip = GetWheeledVehicle()->GetWheel4WD(E4WDWheelIndex::RR)->Slip = FVector2D(DynCar->car_state.long_slip_r, DynCar->car_state.slipAng_r);

	Pos.Z += DZ;
	Rot.Pitch += DPitch;
	Rot.Roll += DRoll;

	VehicleSimData.VehicleKinematic.Push(DeltaTime);
	VehicleSimData.VehicleKinematic.Curr.GlobalPose = FTransform(Rot, Pos - FRotator(DPitch, 0, DRoll).RotateVector(CoF) + CoF, FVector(1.0, 1.0, 1.0));
	VehicleSimData.VehicleKinematic.Curr.GlobalVelocityOfCenterMass = FVector(DynCar->car_state.Vx * 100, -DynCar->car_state.Vy * 100, ZDot).RotateAngleAxis(
		-DynCar->car_state.psi / M_PI * 180.0, FVector(0.f, 0.f, 1.f));
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
		YPos += Canvas->DrawText(RenderFont, TEXT("Bicycle Vehicle:"), 4, YPos + 4);

		Canvas->SetDrawColor(FColor::White);
		YPos += Canvas->DrawText(RenderFont, FString::Printf(TEXT("Vx (km/h): %.1f"), DynCar->car_state.Vx * 3.6), 16, YPos);
		YPos += Canvas->DrawText(RenderFont, FString::Printf(TEXT("Vy (km/h): %.1f"), DynCar->car_state.Vy * 3.6), 16, YPos);
		YPos += Canvas->DrawText(RenderFont, FString::Printf(TEXT("Acc x (m/s^2): %.1f"), DynCar->car_state.dVxdt), 16, YPos);
		YPos += Canvas->DrawText(RenderFont, FString::Printf(TEXT("Acc y (m/s^2): %.1f"), DynCar->car_state.dVydt), 16, YPos);

		if (bDrawTelemetry)
		{
			float XPos = SodaApp.GetSodaUserSettings()->VehicleDebugAreaWidth + 10;
			TelemetryGrid.Draw(Canvas, XPos, 10);
		}
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
