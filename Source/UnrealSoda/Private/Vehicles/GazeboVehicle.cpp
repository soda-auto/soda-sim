// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/Vehicles/GazeboVehicle.h"
#include "Soda/UnrealSoda.h"
#include "DrawDebugHelpers.h"
#include "Components/PrimitiveComponent.h"

const FVector V2V(const soda::Vector3D& V)
{
	return FVector(V.x * 100, -V.y * 100, V.z * 100);
}

const soda::Vector3D V2V(const FVector& V)
{
	return soda::Vector3D(V.X / 100, -V.Y / 100, V.Z / 100);
}

void FSecondaryTickFunction::ExecuteTick(
	float DeltaTime,
	ELevelTick TickType,
	ENamedThreads::Type CurrentThread,
	const FGraphEventRef& MyCompletionGraphEvent)
{

	if (Target && !Target->IsPendingKillOrUnreachable())
	{
		if (TickType != LEVELTICK_ViewportsOnly || Target->ShouldTickIfViewportsOnly())
		{
			FScopeCycleCounterUObject ActorScope(Target);
			Target->TickActor2(DeltaTime * Target->CustomTimeDilation, TickType, *this);
		}
	}
}

FString FSecondaryTickFunction::DiagnosticMessage()
{
	return Target->GetFullName() + TEXT("[TickActor2]");
}

AGazeboPawn::AGazeboPawn()
{
	/*
	Ctx = new zmq::context_t();
	SockSub = new zmq::socket_t(*Ctx, ZMQ_SUB);
	SockPub = new zmq::socket_t(*Ctx, ZMQ_PUB);

	SockSub->setsockopt(ZMQ_SUBSCRIBE, 0, 0);
	SockPub->setsockopt(ZMQ_SNDHWM, 1);
	*/

	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bTickEvenWhenPaused = false;
	PrimaryActorTick.TickGroup = TG_PrePhysics;
	PrimaryActorTick.bStartWithTickEnabled = true;

	//PrimaryActorTick.bAllowTickOnDedicatedServer = true;

	SecondaryActorTick.bCanEverTick = true;
	SecondaryActorTick.bTickEvenWhenPaused = false;
	SecondaryActorTick.TickGroup = TG_PostPhysics;
	SecondaryActorTick.bStartWithTickEnabled = true;
}

AGazeboPawn::~AGazeboPawn()
{
	/*
	delete SockSub;
	delete SockPub;
	delete Ctx;
	*/
}

void AGazeboPawn::PostInitProperties()
{
	Super::PostInitProperties();

	if (!IsTemplate() && SecondaryActorTick.bCanEverTick)
	{
		SecondaryActorTick.Target = this;
		SecondaryActorTick.SetTickFunctionEnable(SecondaryActorTick.bStartWithTickEnabled || SecondaryActorTick.IsTickFunctionEnabled());
		SecondaryActorTick.RegisterTickFunction(GetLevel());
	}
}

void AGazeboPawn::BeginPlay()
{
	Super::BeginPlay();

	//int Confl = 1;

	/*
	try
	{
		//SockSub->setsockopt(ZMQ_CONFLATE, &Confl, sizeof(confl)); // Keep only last message
		SockSub->connect(std::string(TCHAR_TO_UTF8(*ModelStateSubAddr)));
	}
	catch (...)
	{
		UE_LOG(LogSoda, Error, TEXT("sock_sub->connect('%s') failed"), *ModelStateSubAddr);
	}

	try
	{
		//SockPub->setsockopt(ZMQ_CONFLATE, &Confl, sizeof(confl)); // Keep only last message
		SockPub->bind(std::string(TCHAR_TO_UTF8(*CollisionPublisherAddr)));
	}
	catch (...)
	{
		UE_LOG(LogSoda, Error, TEXT("sock_pub->bind('%s')"), *CollisionPublisherAddr);
	}
	*/

	CollisionMsg.msg_id = 0;
	StateMsg.msg_id = 0;
}

void AGazeboPawn::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	/*

	try
	{
		SockSub->disconnect(TCHAR_TO_UTF8(*ModelStateSubAddr));
	}
	catch (...)
	{
		UE_LOG(LogSoda, Error, TEXT("sock_sub->disconnect('%s')"), *ModelStateSubAddr);
	}

	try
	{
		SockPub->unbind(TCHAR_TO_UTF8(*CollisionPublisherAddr));
	}
	catch (...)
	{
		UE_LOG(LogSoda, Error, TEXT("sock_pub->unbind('%s')"), *CollisionPublisherAddr);
	}
	*/
}

void AGazeboPawn::TickActor(float DeltaTime, enum ELevelTick TickType, FActorTickFunction& ThisTickFunction)
{
	Super::TickActor(DeltaTime, TickType, ThisTickFunction);

	zmq::message_t Msg;
	int Pkgs = 0;

	/*
	while (SockSub->recv(&Msg, ZMQ_NOBLOCK))
	{
		StateMsg.Deserialize((unsigned char*)Msg.data(), Msg.size());
		Pkgs++;
	}
	*/

	/*
	UE_LOG(LogSoda, Warning, TEXT("*** PrePhysics; pkgs: %i; state_msg.last_collision_msg: %i; last_collision_msg: %i; acc_z: %f"), 
		pkgs, 
		state_msg.last_collision_msg, 
		collision_msg.msg_id,
		state_msg.liner_acceleration.z);
	*/

	if (!Model || !Pkgs)
		return;

	/*
	UE_LOG(LogSoda, Warning, TEXT("*** pos (%f %f %f); yaw: %f; pitch: %f; roll: %f; V (%f %f %f); A (%f %f %f);"), 
		state.x, state.y, state.z, 
		state.yaw, state.pitch, state.roll, 
		state.vx, state.vy, state.vz,
		state.ax, state.ay, state.az
	);*/

	Model->AddForce(FVector(0, 0, (StateMsg.liner_acceleration.z - StateMsg.add_acc_z) * 1.5 * 100));

	Model->SetWorldTransform(FTransform(
								 FRotator(-StateMsg.pitch / PI * 180, -StateMsg.yaw / PI * 180, StateMsg.roll / PI * 180).Quaternion(),
								 V2V(StateMsg.position)),
							 true);

	Model->SetPhysicsLinearVelocity(V2V(StateMsg.liner_velocity));
	Model->SetPhysicsAngularVelocityInRadians(FVector(-StateMsg.angular_velocity.x, StateMsg.angular_velocity.y, -StateMsg.angular_velocity.z));

	if (GazeboJoints.Num() != StateMsg.chanels.size())
		GazeboJoints.SetNum(StateMsg.chanels.size());

	for (size_t i = 0; i < GazeboJoints.Num(); i++)
		GazeboJoints[i] = StateMsg.chanels[i];

	/*
    FVector v = Model->GetPhysicsLinearVelocity();
	UE_LOG(LogSoda, Warning, TEXT("*** PrePhysics; V: (%f %f %f);"), 
		v.X, v.Y, v.Z
	);
*/
}

void AGazeboPawn::TickActor2(float DeltaTime, enum ELevelTick TickType, FSecondaryTickFunction& ThisTickFunction)
{
	/*
    const bool bShouldTick = ((TickType != LEVELTICK_ViewportsOnly) || ShouldTickIfViewportsOnly());
    if (bShouldTick)
    {
      	if (!IsPendingKill() && GetWorld())
       	{
			if (GetWorldSettings()!=NULL &&
				(bAllowReceiveTickEventOnDedicatedServer||!IsRunningDedicatedServer()))
				{
					//My cool post physics tick stuff
				}
       	}
    }
*/

	/*
	FVector v = Model->GetPhysicsLinearVelocity();
	UE_LOG(LogSoda, Warning, TEXT("*** PostPhysics; V: (%f %f %f);"), 
		v.X, v.Y, v.Z
	);
*/
}

void AGazeboPawn::NotifyHit(class UPrimitiveComponent* MyComp, class AActor* Other, class UPrimitiveComponent* OtherComp, bool bSelfMoved, FVector HitLocation,
							FVector HitNormal, FVector NormalImpulse, const FHitResult& Hit)
{
	if (!Model)
		return;

	/*
	UE_LOG(LogSoda, Warning, TEXT("Hit location (%f %f %f), normal (%f %f %f);"), 
		HitLocation.X, HitLocation.Y, HitLocation.Z, 
		NormalImpulse.X, NormalImpulse.Y, NormalImpulse.Z);
*/
	FVector Pos = Model->GetComponentLocation();
	FRotator Rot = Model->GetComponentRotation();
	FVector CMass = Model->GetCenterOfMass();
	FVector V = Model->GetPhysicsLinearVelocity();
	FVector W = Model->GetPhysicsAngularVelocityInRadians();

	//FTransform Trans = Model->GetComponentTransform().Inverse();
	//FVector HitPos = trans.TransformPosition(HitLocation);
	//FVector NormalImpulseTrans =  trans.TransformVector(NormalImpulse);

	CollisionMsg.position = V2V(Pos);
	CollisionMsg.yaw = -Rot.Yaw / 180.0 * PI;
	CollisionMsg.pitch = -Rot.Pitch / 180.0 * PI;
	CollisionMsg.roll = Rot.Roll / 180.0 * PI;
	CollisionMsg.liner_velocity = V2V(V);
	CollisionMsg.angular_velocity = soda::Vector3D(-W.X, W.Y, -W.Z);
	CollisionMsg.hit_location = V2V(HitLocation);
	CollisionMsg.hit_normal = V2V(HitNormal);
	CollisionMsg.hit_normal_impulse = V2V(NormalImpulse);
	CollisionMsg.msg_id++;
	CollisionMsg.last_state_msg = StateMsg.msg_id;

	CollisionMsg.Serialize(SendBuf);

	//SockPub->send(&SendBuf[0], SendBuf.size(), ZMQ_NOBLOCK);

	if (DebugOut)
	{
		DrawDebugLine(
			GetWorld(),
			HitLocation,
			HitLocation + NormalImpulse * 30,
			FColor(0, 255, 0), false, 2.0, 0, 12.333);
		/*
		UE_LOG(LogSoda, Warning, TEXT("*** NotifyHit; V: (%f %f %f); id: %i"),
			   V.X, V.Y, V.Z, CollisionMsg.msg_id);
		*/
	}
}

void AGazeboPawn::PostInitializeComponents()
{
	Super::PostInitializeComponents();
}