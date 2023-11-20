// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/VehicleComponents/Others/V2V.h"
#include "Soda/UnrealSoda.h"
#include "Soda/SodaApp.h"
#include "DrawDebugHelpers.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Soda/SodaStatics.h"
#include "Sockets.h"
#include "SocketSubsystem.h"
#include "Soda/LevelState.h"

int UV2VTransmitterComponent::IDsCounter = 10000;

UV2VTransmitterComponent::UV2VTransmitterComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	GUI.Category = TEXT("Other");
	GUI.ComponentNameOverride = TEXT("V2V Transmitter");
	GUI.IcanName = TEXT("SodaIcons.Modem");
	GUI.bIsPresentInAddMenu = true;
}

void UV2VTransmitterComponent::BeginPlay()
{
	Super::BeginPlay();

	if (bAutoCalculateBoundingBox)
	{
		CalculateBoundingBox();
	}

	if (bAssignUniqueIdAtStartup)
	{
		ID = IDsCounter;
	}

	++IDsCounter;
}

void UV2VTransmitterComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	//if (!IsTickOnCurrentFrame() || !HealthIsWorkable()) return;
}

bool UV2VTransmitterComponent::OnActivateVehicleComponent()
{
	return Super::OnActivateVehicleComponent();
}

void UV2VTransmitterComponent::OnDeactivateVehicleComponent()
{
	Super::OnDeactivateVehicleComponent();
}

void UV2VTransmitterComponent::GetRemark(FString& Info) const
{
	Info = "ID: " + FString::FromInt(ID);
}

bool UV2VTransmitterComponent::FillV2VMsg(soda::V2VSimulatedObject& V2V_data, bool RecalculateBoundingBox, bool DrawDebug)
{
	if (RecalculateBoundingBox)
	{
		CalculateBoundingBox();
	}
	UPrimitiveComponent* BasePrimComp = Cast< UPrimitiveComponent >(GetAttachParent());
	if (BasePrimComp)
	{
		FVector WorldLoc = GetComponentLocation();
		FRotator WorldRot = GetComponentRotation();
		FTransform Trans = GetComponentTransform();
		FVector WorldVel = BasePrimComp->GetPhysicsLinearVelocityAtPoint(GetComponentLocation());
		FVector AngVel = BasePrimComp->GetPhysicsAngularVelocityInRadians();
		FVector LocalVel = Trans.Rotator().UnrotateVector(WorldVel);

		V2V_data.id = ID;

		V2V_data.bounding_box0_X = BoundingBox0.X;
		V2V_data.bounding_box0_Y = BoundingBox0.Y;
		V2V_data.bounding_box1_X = BoundingBox1.X;
		V2V_data.bounding_box1_Y = BoundingBox1.Y;
		V2V_data.bounding_box2_X = BoundingBox2.X;
		V2V_data.bounding_box2_Y = BoundingBox2.Y;
		V2V_data.bounding_box3_X = BoundingBox3.X;
		V2V_data.bounding_box3_Y = BoundingBox3.Y;

		V2V_data.bounding_3d_box0_X = BoundingBox0.X;
		V2V_data.bounding_3d_box0_Y = BoundingBox0.Y;
		V2V_data.bounding_3d_box0_Z = BoundingBox0.Z;
		V2V_data.bounding_3d_box1_X = BoundingBox1.X;
		V2V_data.bounding_3d_box1_Y = BoundingBox1.Y;
		V2V_data.bounding_3d_box1_Z = BoundingBox1.Z;
		V2V_data.bounding_3d_box2_X = BoundingBox2.X;
		V2V_data.bounding_3d_box2_Y = BoundingBox2.Y;
		V2V_data.bounding_3d_box2_Z = BoundingBox2.Z;
		V2V_data.bounding_3d_box3_X = BoundingBox3.X;
		V2V_data.bounding_3d_box3_Y = BoundingBox3.Y;
		V2V_data.bounding_3d_box3_Z = BoundingBox3.Z;
		V2V_data.bounding_3d_box4_X = BoundingBox4.X;
		V2V_data.bounding_3d_box4_Y = BoundingBox4.Y;
		V2V_data.bounding_3d_box4_Z = BoundingBox4.Z;
		V2V_data.bounding_3d_box5_X = BoundingBox5.X;
		V2V_data.bounding_3d_box5_Y = BoundingBox5.Y;
		V2V_data.bounding_3d_box5_Z = BoundingBox5.Z;
		V2V_data.bounding_3d_box6_X = BoundingBox6.X;
		V2V_data.bounding_3d_box6_Y = BoundingBox6.Y;
		V2V_data.bounding_3d_box6_Z = BoundingBox6.Z;
		V2V_data.bounding_3d_box7_X = BoundingBox7.X;
		V2V_data.bounding_3d_box7_Y = BoundingBox7.Y;
		V2V_data.bounding_3d_box7_Z = BoundingBox7.Z;

		FVector Desp(100.f * (BoundingBox0.X + BoundingBox2.X) / 2, 100.f * (BoundingBox0.Y + BoundingBox2.Y) / 2, 100.f * (BoundingBox0.Z + BoundingBox6.Z) / 2);
		FVector Extent(100.f * (BoundingBox0.X - BoundingBox2.X), 100.f * (BoundingBox0.Y - BoundingBox2.Y), 100.f * (BoundingBox6.Z - BoundingBox0.Z));

		double Lon, Lat, Alt;
		GetLevelState()->GetLLConverter().UE2LLA(WorldLoc, Lon, Lat, Alt);

		V2V_data.timestamp = soda::RawTimestamp<std::chrono::milliseconds>(SodaApp.GetSimulationTimestamp());
		V2V_data.longitude = Lon / 180.0 * M_PI;
		V2V_data.latitude = Lat / 180.0 * M_PI;
		V2V_data.altitude = Alt;
		V2V_data.roll = WorldRot.Roll / 180.0 * M_PI;
		V2V_data.pitch = WorldRot.Pitch / 180.0 * M_PI;
		V2V_data.yaw = NormAngRad(-WorldRot.Yaw / 180 * M_PI + M_PI);
		V2V_data.vel_forward = LocalVel.X / 100.0;
		V2V_data.vel_lateral = -LocalVel.Y / 100.0;
		V2V_data.vel_up = LocalVel.Z / 100.0;
		V2V_data.ang_vel_roll = -AngVel.X;
		V2V_data.ang_vel_pitch = AngVel.Y;
		V2V_data.ang_vel_yaw = AngVel.Z;

		if (DrawDebug)
		{
#if ENABLE_DRAW_DEBUG
			DrawDebugBox(GetWorld(), WorldLoc + WorldRot.RotateVector(Desp), Extent * 0.5f, FQuat(WorldRot), FColor::Green, false, -1.f, 0.0f, 5.f);
			DrawDebugString(GetWorld(), WorldLoc, *FString::Printf(TEXT("object_id=%d, yaw=%f"), V2V_data.id, V2V_data.yaw), NULL, FColor::Green, 0.0f, false);
#endif
		}

		return true;
	}

	return false;
}

bool UV2VTransmitterComponent::FillV2VMsg(soda::V2XRenderObject& V2V_data, bool RecalculateBoundingBox, bool DrawDebug)
{
	if (RecalculateBoundingBox)
	{
		CalculateBoundingBox();
	}
	UPrimitiveComponent* BasePrimComp = Cast< UPrimitiveComponent >(GetAttachParent());

	if (BasePrimComp)
	{
		FVector WorldLoc = GetComponentLocation();
		FRotator WorldRot = GetComponentRotation();
		FTransform Trans = GetComponentTransform();
		FVector WorldVel = BasePrimComp->GetPhysicsLinearVelocityAtPoint(GetComponentLocation());
		FVector AngVel = BasePrimComp->GetPhysicsAngularVelocityInRadians();
		FVector LocalVel = Trans.Rotator().UnrotateVector(WorldVel);

		FVector Desp = FVector((BoundingBox0.X + BoundingBox2.X) / 2, (BoundingBox0.Y + BoundingBox2.Y) / 2, (BoundingBox0.Z + BoundingBox6.Z) / 2) * 100;
		FVector Extent = FVector((BoundingBox0.X - BoundingBox2.X), (BoundingBox0.Y - BoundingBox2.Y), (BoundingBox6.Z - BoundingBox0.Z)) * 100;
		FVector BoxWorldCenter = WorldLoc + WorldRot.RotateVector(Desp);

		V2V_data.reserved = 0;
		V2V_data.object_type = (uint32_t)soda::V2XRenderObjectType::VEHICLE;
		V2V_data.object_id = ID;
		V2V_data.timestamp = soda::RawTimestamp<std::chrono::duration<double>>(SodaApp.GetSimulationTimestamp());
		V2V_data.east = -BoxWorldCenter.X / 100.0;
		V2V_data.north = BoxWorldCenter.Y / 100.0;
		V2V_data.up = BoxWorldCenter.Z / 100.0;
		V2V_data.heading = NormAngRad(-WorldVel.ToOrientationRotator().Yaw / 180.0 * M_PI + M_PI);
		V2V_data.velocity = WorldVel.Size() / 100.0;
		V2V_data.status = 0;
		V2V_data.position_accuracy = 0.001;
		V2V_data.heading_accuracy = 0.001;
		V2V_data.velocity_accuracy = 0.001;
		V2V_data.vertical_velocity = WorldVel.Z / 100.0;
		V2V_data.length = Extent.X / 100;
		V2V_data.width = Extent.Y / 100;
		V2V_data.height = Extent.Z / 100;
		V2V_data.yaw_angle = NormAngRad(-WorldRot.Yaw / 180 * M_PI + M_PI);
		V2V_data.pitch_angle = WorldRot.Pitch / 180 * M_PI;
		V2V_data.roll_angle = -WorldRot.Roll / 180 * M_PI;
		V2V_data.orientation_accuracy = 0.001;

		if (DrawDebug)
		{
#if ENABLE_DRAW_DEBUG
			DrawDebugBox(GetWorld(), BoxWorldCenter, Extent * 0.5f, FQuat(WorldRot), FColor::Green, false, -1.f, 0.0f, 5.f);
			DrawDebugString(GetWorld(), BoxWorldCenter, *FString::Printf(TEXT("object_id=%d, heading=%f, velocity=%f, yaw=%f, east=%f, north=%f"), V2V_data.object_id, V2V_data.heading, V2V_data.velocity, V2V_data.yaw_angle, V2V_data.east, V2V_data.north), NULL, FColor::Green, 0.0f, false);
#endif
		}

		return true;
	}

	return false;
}

void UV2VTransmitterComponent::CalculateBoundingBox()
{
	if (AActor* ParentActor = GetOwner())
	{
		FBox LocalBounds;
		USodaStatics::GetActorLocalBounds(ParentActor, LocalBounds);
// Todo: move wheels bounding logic to SodaWheeledVehicleMovement classes
/*		if (UArrivalPhisXWheeledVehicleMovementComponent* WheeledVehicleMovementComponent = ParentActor->FindComponentByClass<UArrivalPhisXWheeledVehicleMovementComponent>())
		{
			for (UStaticMeshComponent* Wheel : WheeledVehicleMovementComponent->WheelMesheComponents)
			{
				if (UStaticMesh* StaticMesh = Wheel->GetStaticMesh())
				{
					FBox WheelBox = StaticMesh->GetBoundingBox();

					FVector WorldLoc = Wheel->GetComponentLocation();
					FRotator WorldRot = Wheel->GetComponentRotation();
					DrawDebugBox(GetWorld(), WorldLoc + WorldRot.RotateVector(WheelBox.GetCenter()), WheelBox.GetExtent() * 0.5f, FQuat(WorldRot), FColor::Green, false, -1.f, 0.0f, 5.f);

				}
			}
		}*/
		BoundingBox0.X = LocalBounds.Max.X / 100;
		BoundingBox0.Y = LocalBounds.Max.Y / 100;
		BoundingBox0.Z = LocalBounds.Min.Z / 100;
		BoundingBox1.X = LocalBounds.Min.X / 100;
		BoundingBox1.Y = LocalBounds.Max.Y / 100;
		BoundingBox1.Z = LocalBounds.Min.Z / 100;
		BoundingBox2.X = LocalBounds.Min.X / 100;
		BoundingBox2.Y = LocalBounds.Min.Y / 100;
		BoundingBox2.Z = LocalBounds.Min.Z / 100;
		BoundingBox3.X = LocalBounds.Max.X / 100;
		BoundingBox3.Y = LocalBounds.Min.Y / 100;
		BoundingBox3.Z = LocalBounds.Min.Z / 100;
		BoundingBox4.X = LocalBounds.Max.X / 100;
		BoundingBox4.Y = LocalBounds.Max.Y / 100;
		BoundingBox4.Z = LocalBounds.Max.Z / 100;
		BoundingBox5.X = LocalBounds.Min.X / 100;
		BoundingBox5.Y = LocalBounds.Max.Y / 100;
		BoundingBox5.Z = LocalBounds.Max.Z / 100;
		BoundingBox6.X = LocalBounds.Min.X / 100;
		BoundingBox6.Y = LocalBounds.Min.Y / 100;
		BoundingBox6.Z = LocalBounds.Max.Z / 100;
		BoundingBox7.X = LocalBounds.Max.X / 100;
		BoundingBox7.Y = LocalBounds.Min.Y / 100;
		BoundingBox7.Z = LocalBounds.Max.Z / 100;
	}
}

/*********************************************************************************************************************/
UV2VReceiverComponent::UV2VReceiverComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	GUI.Category = TEXT("Other");
	GUI.ComponentNameOverride = TEXT("V2V Receiver");
	GUI.IcanName = TEXT("SodaIcons.Modem");
	GUI.bIsPresentInAddMenu = true;

	PrimaryComponentTick.bCanEverTick = true;
}

void UV2VReceiverComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (!IsTickOnCurrentFrame() || !HealthIsWorkable()) return;

	soda::V2VSimulatedObject V2VDataOld;
	soda::V2XRenderObject V2VDataNew;

	int SentMessages = 0;
	FString VehiclesIds;

	auto World = GetWorld();
	for (TObjectIterator< UV2VTransmitterComponent > V2VItr; V2VItr; ++V2VItr)
	{
		if (V2VItr->GetWorld() == World)
		{
			if (((*V2VItr)->GetComponentLocation() - GetComponentLocation()).Size() < Radius * 100)
			{
				if (bUseNewProtocol)
				{
					if ((*V2VItr)->FillV2VMsg(V2VDataNew, bRecalculateBoundingBoxEveryTick, Common.bDrawDebugCanvas))
					{
						Publisher.Publish(V2VDataNew);
					}
					if (bLogMessages)
					{
						SentMessages++;
						VehiclesIds.Appendf(TEXT("%d, "), V2VDataNew.object_id);
					}
				}
				else
				{
					if ((*V2VItr)->FillV2VMsg(V2VDataOld, bRecalculateBoundingBoxEveryTick, Common.bDrawDebugCanvas))
					{
						Publisher.Publish(V2VDataOld);
					}
					if (bLogMessages)
					{
						SentMessages++;
						VehiclesIds.Appendf(TEXT("%d, "), V2VDataOld.id);
					}
				}
			}
		}
	}
	if (bLogMessages)
	{
		UE_LOG(LogSoda, Log, TEXT("UV2VReceiverComponent() sent %d message(s), v2v ids:{ %s }"), SentMessages, *VehiclesIds);
	}
}

bool UV2VReceiverComponent::OnActivateVehicleComponent()
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

	return true;
}

void UV2VReceiverComponent::OnDeactivateVehicleComponent()
{
	Super::OnDeactivateVehicleComponent();
	Publisher.Shutdown();
}

void UV2VReceiverComponent::GetRemark(FString & Info) const
{
	Info = "Port: " +  FString::FromInt(Publisher.Port);
}