// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/VehicleComponents/Others/WheelsRendering.h"
#include "Soda/Vehicles/SodaWheeledVehicle.h"
#include "Soda/Vehicles/IWheeledVehicleMovementInterface.h"

UWheelsRenderingComponent::UWheelsRenderingComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	GUI.Category = TEXT("Other");
	GUI.ComponentNameOverride = TEXT("Wheels Rendering");
	GUI.bIsPresentInAddMenu = true;

	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;

	WheelsAnimationData.SetNumZeroed(4);
}

void UWheelsRenderingComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (!HealthIsWorkable()) return;

	for (int32 i = 0; i < 4; ++i)
	{
		FWheelAnimationData& WheelInstance = WheelsAnimationData[i];
		const USodaVehicleWheelComponent* SodaWheel = GetWheeledVehicle()->GetWheel4WD(E4WDWheelIndex(i));
		UStaticMeshComponent* WheelMesheComponent = WheelMesheComponents[i];
		bool bFlip = bool(((i % 2 != 0) + bFlipWheelsMesh) % 2);

		if (bStagecoachEffect && WheelSpokeCount > 0 && ShutterSpeed > 0 && MaxAngularVelocity > SMALL_NUMBER) // employ stagecoach effect
		{
			// normalized spoke transition value
			float AngularVelocity = SodaWheel->AngularVelocity / M_PI * 180;
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
			float RotationDelta = CorrectedAngularVelocity * DeltaTime;
			WheelInstance.RotOffset.Pitch += RotationDelta;

			int ExcessRotations = (int)(WheelInstance.RotOffset.Pitch / 360.0f);
			if (FMath::Abs(ExcessRotations) > 1)
			{
				WheelInstance.RotOffset.Pitch -= ExcessRotations * 360.0f * (bFlip ? -1 : 1);
			}
		}
		else
		{
			WheelInstance.RotOffset.Pitch = SodaWheel->Pitch / M_PI * 180 * (bFlip ? -1 : 1);
		}

		WheelInstance.RotOffset.Yaw = SodaWheel->Steer / M_PI * 180 + (bFlip ? 180 : 0);
		WheelInstance.RotOffset.Roll = 0.f;
		WheelInstance.LocOffset = FVector(0, 0, SodaWheel->SuspensionOffset);

		/*
		if (i % 2)
		{
			Rot = FRotator(-WheelInstance.RotOffset., WheelSteer + 180.0, 0.0);
		}
		
		else
		{
			Rot = FRotator(Wheels[i]->Pitch / M_PI * 180.0, WheelSteer, 0.0);
		}
		*/
		

		WheelMesheComponent->SetRelativeTransform(
			FTransform(
				SodaWheel->RestingRotation + WheelInstance.RotOffset,
				SodaWheel->RestingLocation + WheelInstance.LocOffset),
			false, nullptr, ETeleportType::None);
	}
}

bool UWheelsRenderingComponent::OnActivateVehicleComponent()
{
	if (!Super::OnActivateVehicleComponent())
	{
		return false;
	}

	if (!GetWheeledVehicle()->Is4WDVehicle())
	{
		SetHealth(EVehicleComponentHealth::Error, TEXT("Support only 4WD vehicles"));
		return false;
	}

	for (size_t i = 0; i < GetWheeledVehicle()->GetWheels().Num(); i++)
	{
		UStaticMeshComponent* WheelMeshComponent = NewObject<UStaticMeshComponent>(this);
		WheelMeshComponent->SetupAttachment(GetWheeledVehicle()->GetMesh());
		WheelMeshComponent->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
		//WheelMeshComponent->SetRelativeLocation(Mesh->GetBoneLocation(WheelSetups[i].BoneName, EBoneSpaces::ComponentSpace));
		//WheelMeshComponent->SetRelativeRotation(Mesh->GetBoneQuaternion(WheelSetups[i].BoneName, EBoneSpaces::ComponentSpace).Rotator());
		WheelMeshComponent->SetStaticMesh((i < 2) ? FrontWheelMesh : RearWheelMesh); // TODO: Determinate front/reaer wheel
		WheelMeshComponent->RegisterComponent();
		WheelMesheComponents.Add(WheelMeshComponent);
	}

	WheelsAnimationData.SetNumZeroed(4);

	return true;
}

void UWheelsRenderingComponent::OnDeactivateVehicleComponent()
{
	Super::OnDeactivateVehicleComponent();

	for (auto& It : WheelMesheComponents)
	{
		It->DestroyComponent();
	}
	WheelMesheComponents.Empty();
}

