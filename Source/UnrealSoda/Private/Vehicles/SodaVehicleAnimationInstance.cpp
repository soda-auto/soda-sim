// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.


#include "Soda/Vehicles/SodaVehicleAnimationInstance.h"
#include "Soda/Vehicles/SodaWheeledVehicle.h"
#include "AnimationRuntime.h"

USodaVehicleAnimationInstance::USodaVehicleAnimationInstance(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}


void USodaVehicleAnimationInstance::NativeInitializeAnimation()
{
	// Find a wheeled movement component
	if (ASodaWheeledVehicle* Actor = Cast<ASodaWheeledVehicle>(GetOwningActor()))
	{
		SetVehicle(Actor);
	}
}

FAnimInstanceProxy* USodaVehicleAnimationInstance::CreateAnimInstanceProxy()
{
	return &AnimInstanceProxy;
}

void USodaVehicleAnimationInstance::DestroyAnimInstanceProxy(FAnimInstanceProxy* InProxy)
{
}

const ASodaWheeledVehicle* USodaVehicleAnimationInstance::GetVehicle() const
{
	return SodaVehicle.Get(); 
}

//---------------------------------------------------------------------------------------------------------------------------------

void FSodaVehicleAnimationInstanceProxy::SetVehicle(const ASodaWheeledVehicle* SodaVehicle)
{
	check(SodaVehicle);

	const int32 NumOfwheels = SodaVehicle->GetWheelsSorted().Num();
	WheelInstances.Empty(NumOfwheels);
	if (NumOfwheels > 0)
	{
		WheelInstances.AddZeroed(NumOfwheels);
		// now add wheel data
		for (int32 WheelIndex = 0; WheelIndex < WheelInstances.Num(); ++WheelIndex)
		{
			FSodaWheelAnimationData& WheelInstance = WheelInstances[WheelIndex];
			const auto & Wheel = SodaVehicle->GetWheelsSorted()[WheelIndex];

			// set data
			WheelInstance.BoneName = Wheel->BoneName;
			WheelInstance.LocOffset = FVector::ZeroVector;
			WheelInstance.RotOffset = FRotator::ZeroRotator;
		}
	}
}

void FSodaVehicleAnimationInstanceProxy::PreUpdate(UAnimInstance* InAnimInstance, float DeltaSeconds)
{
	Super::PreUpdate(InAnimInstance, DeltaSeconds);

	const USodaVehicleAnimationInstance* VehicleAnimInstance = CastChecked<USodaVehicleAnimationInstance>(InAnimInstance);
	if (const ASodaWheeledVehicle* Vehicle= VehicleAnimInstance->GetVehicle())
	{
		for (int32 WheelIndex = 0; WheelIndex < WheelInstances.Num(); ++WheelIndex)
		{
			FSodaWheelAnimationData& WheelInstance = WheelInstances[WheelIndex];
			//if (Vehicle->Wheels.IsValidIndex(WheelIndex))
			{
				const auto & VehicleWheel = Vehicle->GetWheelsSorted()[WheelIndex];
				if (WheelSpokeCount > 0 && ShutterSpeed > 0 && MaxAngularVelocity > SMALL_NUMBER) // employ stagecoach effect
				{
					// normalized spoke transition value
					float AngularVelocity = VehicleWheel->AngularVelocity / M_PI * 180;
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
					WheelInstance.RotOffset.Pitch = VehicleWheel->Pitch / M_PI * 180;
				}
				WheelInstance.RotOffset.Yaw = VehicleWheel->Steer / M_PI * 180.0;
				WheelInstance.RotOffset.Roll = 0.f;
				WheelInstance.LocOffset = VehicleWheel->SuspensionOffset2;
			}
		}
	}
}
