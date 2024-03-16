// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.


#include "Soda/Actors/GhostVehicle/GhostVehicleAnimationInstance.h"
#include "Soda/Actors/GhostVehicle/GhostVehicle.h"
#include "AnimationRuntime.h"

UGhostVehicleAnimationInstance::UGhostVehicleAnimationInstance(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}


void UGhostVehicleAnimationInstance::NativeInitializeAnimation()
{
	// Find a wheeled movement component
	if (AGhostVehicle* Actor = Cast<AGhostVehicle>(GetOwningActor()))
	{
		SetVehicle(Actor);
	}
}

FAnimInstanceProxy* UGhostVehicleAnimationInstance::CreateAnimInstanceProxy()
{
	return &AnimInstanceProxy;
}

void UGhostVehicleAnimationInstance::DestroyAnimInstanceProxy(FAnimInstanceProxy* InProxy)
{
}

const AGhostVehicle* UGhostVehicleAnimationInstance::GetVehicle() const
{
	return GhostVehicle.Get(); 
}

//---------------------------------------------------------------------------------------------------------------------------------

void FGhostVehicleAnimationInstanceProxy::SetVehicle(const AGhostVehicle* GhostVehicle)
{
	check(GhostVehicle);

	const int32 NumOfwheels = GhostVehicle->Wheels.Num();
	WheelInstances.Empty(NumOfwheels);
	if (NumOfwheels > 0)
	{
		WheelInstances.AddZeroed(NumOfwheels);
		// now add wheel data
		for (int32 WheelIndex = 0; WheelIndex < WheelInstances.Num(); ++WheelIndex)
		{
			FGhostWheelAnimationData& WheelInstance = WheelInstances[WheelIndex];
			const FGhostVehicleWheel& Wheel = GhostVehicle->Wheels[WheelIndex];

			// set data
			WheelInstance.BoneName = Wheel.BoneName;
			WheelInstance.LocOffset = FVector::ZeroVector;
			WheelInstance.RotOffset = FRotator::ZeroRotator;
		}
	}
}

void FGhostVehicleAnimationInstanceProxy::PreUpdate(UAnimInstance* InAnimInstance, float DeltaSeconds)
{
	Super::PreUpdate(InAnimInstance, DeltaSeconds);

	const UGhostVehicleAnimationInstance* VehicleAnimInstance = CastChecked<UGhostVehicleAnimationInstance>(InAnimInstance);
	if (const AGhostVehicle* Vehicle= VehicleAnimInstance->GetVehicle())
	{
		for (int32 WheelIndex = 0; WheelIndex < WheelInstances.Num(); ++WheelIndex)
		{
			FGhostWheelAnimationData& WheelInstance = WheelInstances[WheelIndex];
			if (Vehicle->Wheels.IsValidIndex(WheelIndex))
			{
				const FGhostVehicleWheel& VehicleWheel = Vehicle->Wheels[WheelIndex];
				if (WheelSpokeCount > 0 && ShutterSpeed > 0 && MaxAngularVelocity > SMALL_NUMBER) // employ stagecoach effect
				{
					// normalized spoke transition value
					float AngularVelocity = VehicleWheel.GetRotationAngularVelocity();
					float DegreesPerFrame = AngularVelocity / ShutterSpeed;
					float DegreesPerSpoke = 360.f / WheelSpokeCount;

					float IntegerPart = 0;
					float SpokeTransition = FMath::Modf(DegreesPerFrame / DegreesPerSpoke, &IntegerPart);
					float StagecoachEffectVelocity = FMath::Sin(SpokeTransition * TWO_PI) * MaxAngularVelocity;

					// blend
					float OffsetVelocity = FMath::Abs(AngularVelocity) - MaxAngularVelocity;
					if (OffsetVelocity < 0.f)
					{
						OffsetVelocity = 0.f;
					}

					float BlendAlpha = FMath::Clamp(OffsetVelocity / MaxAngularVelocity, 0.f, 1.f);

					float CorrectedAngularVelocity = FMath::Lerp(AngularVelocity, StagecoachEffectVelocity, BlendAlpha);
					CorrectedAngularVelocity = FMath::Clamp(CorrectedAngularVelocity, -MaxAngularVelocity, MaxAngularVelocity);

					// integrate to angular position
					float RotationDelta = CorrectedAngularVelocity * DeltaSeconds;
					WheelInstance.RotOffset.Pitch += RotationDelta;

					int ExcessRotations = (int)(WheelInstance.RotOffset.Pitch / 360.0f);
					if (FMath::Abs(ExcessRotations) > 1)
					{
						WheelInstance.RotOffset.Pitch -= ExcessRotations * 360.0f;
					}
				}
				else
				{
					WheelInstance.RotOffset.Pitch = VehicleWheel.GetRotationAngle();
				}
				WheelInstance.RotOffset.Yaw = VehicleWheel.GetSteerAngle();
				WheelInstance.RotOffset.Roll = 0.f;
				WheelInstance.LocOffset = -FVector(0, 0, 1) * VehicleWheel.GetSuspensionOffset();
			}
		}
	}
}
