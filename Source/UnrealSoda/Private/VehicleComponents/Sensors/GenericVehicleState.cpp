// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/VehicleComponents/Sensors/GenericVehicleState.h"
#include "Soda/VehicleComponents/VehicleDriverComponent.h"
#include "Soda/UnrealSoda.h"
#include "DrawDebugHelpers.h"
#include "Soda/LevelState.h"
#include "Soda/Vehicles/SodaWheeledVehicle.h"

UGenericVehicleStateComponent::UGenericVehicleStateComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	GUI.ComponentNameOverride = TEXT("GenericVehicleState");
	GUI.bIsPresentInAddMenu = true;
	GUI.IcanName = TEXT("SodaIcons.GPS");
}

void UGenericVehicleStateComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	//if (!IsTickOnCurrentFrame() || !HealthIsWorkable()) return;
}

void UGenericVehicleStateComponent::PostPhysicSimulationDeferred(float DeltaTime, const FPhysBodyKinematic& VehicleKinematic, const TTimestamp & Timestamp)
{
	Super::PostPhysicSimulationDeferred(DeltaTime, VehicleKinematic, Timestamp);

	if (!Publisher.IsAdvertised())
		return;

	FTransform WorldPose;
	FVector WorldVel;
	FVector LocalAcc;
	FVector Gyro;
	VehicleKinematic.CalcIMU(GetRelativeTransform(), WorldPose, WorldVel, LocalAcc, Gyro);
	FRotator WorldRot = WorldPose.Rotator();
	FVector WorldLoc = WorldPose.GetTranslation();

	if (bImuNoiseEnabled)
	{
		FVector RotNoize = NoiseParams.Rotation.Step();
		WorldRot += FRotator(RotNoize.Y, RotNoize.Z, RotNoize.X);
		WorldLoc += NoiseParams.Location.Step() * 100;
		LocalAcc += NoiseParams.Acceleration.Step() * 100;
		WorldVel += NoiseParams.Velocity.Step() * 100;
		Gyro += NoiseParams.Gyro.Step();
	}
	FVector LocVel = WorldRot.UnrotateVector(WorldVel);

	double Lon, Lat, Alt;
	GetLevelState()->GetLLConverter().UE2LLA(WorldLoc, Lon, Lat, Alt);

	if (OutFile.is_open())
	{
		WriteCsvLog(FVector::ZeroVector, FRotator::ZeroRotator, FVector::ZeroVector, LocalAcc, Gyro, 0, 0, 0, Timestamp);
	}

	static auto ToSodaVec = [](const FVector& V) { return soda::Vector{ V.X, -V.Y, V.Z }; };
	static auto ToSodaRot = [](const FVector& V) { return soda::Vector{ -V.X, V.Y, -V.Z }; };

	soda::GenericVehicleState Msg {};

	Msg.navigation_state.timestamp = soda::RawTimestamp<std::chrono::nanoseconds>(Timestamp);
	Msg.navigation_state.longitude = Lon;
	Msg.navigation_state.latitude = Lat;
	Msg.navigation_state.altitude = Alt;
	Msg.navigation_state.rotation.roll =  -NormAngRad(WorldRot.Roll / 180.0 * M_PI);
	Msg.navigation_state.rotation.pitch =  NormAngRad(WorldRot.Pitch / 180.0 * M_PI);
	Msg.navigation_state.rotation.yaw =   -NormAngRad(WorldRot.Yaw / 180.0 * M_PI);
	Msg.navigation_state.position = ToSodaVec(WorldLoc * 0.01);
	Msg.navigation_state.world_velocity = ToSodaVec(WorldVel * 0.01);
	Msg.navigation_state.loc_velocity  = ToSodaVec(LocVel * 0.01);
	Msg.navigation_state.angular_velocity = ToSodaRot(Gyro);
	Msg.navigation_state.acceleration = ToSodaVec(LocalAcc * 0.01);
	Msg.navigation_state.gyro_biases = ToSodaRot(NoiseParams.Gyro.GetAccuracy());
	Msg.navigation_state.acc_biases = ToSodaVec(NoiseParams.Acceleration.GetAccuracy());

	if (VehicleDriver)
	{
		Msg.gear = soda::EGear(VehicleDriver->GetGear());
		Msg.mode = soda::EControlMode(VehicleDriver->GetDriveMode());
	}

	if (bIs4WDVehicle)
	{
		Msg.steer = -(WheeledVehicle->GetWheel4WD(E4WDWheelIndex::FL)->Steer + WheeledVehicle->GetWheel4WD(E4WDWheelIndex::FR)->Steer) / 2;
		for (int i = 0; i < 4; i++)
		{
			USodaVehicleWheelComponent* Wheel = WheeledVehicle->GetWheel4WD(E4WDWheelIndex(i));
			Msg.wheels_state[i].ang_vel = Wheel->AngularVelocity;
			Msg.wheels_state[i].torq = Wheel->ReqTorq;
			Msg.wheels_state[i].brake_torq = Wheel->ReqBrakeTorque;
		}
	}

	Publisher.Publish(Msg);
}

bool UGenericVehicleStateComponent::OnActivateVehicleComponent()
{
	if (!Super::OnActivateVehicleComponent())
	{
		return false;
	}

	Publisher.Advertise();

	if (!Publisher.IsAdvertised())
	{
		SetHealth(EVehicleComponentHealth::Error);
		return false;
	}

	WheeledVehicle = Cast<ASodaWheeledVehicle>(GetVehicle());
	bIs4WDVehicle = WheeledVehicle && WheeledVehicle->Is4WDVehicle();
	VehicleDriver = LinkToVehicleDriver.GetObject<UVehicleDriverComponent>(GetOwner());

	return true;
}

void UGenericVehicleStateComponent::OnDeactivateVehicleComponent()
{
	Super::OnDeactivateVehicleComponent();

	Publisher.Shutdown();
}

void UGenericVehicleStateComponent::GetRemark(FString& Info) const
{
	Info = Publisher.Address + ":" + FString::FromInt(Publisher.Port);
}
