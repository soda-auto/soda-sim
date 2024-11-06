// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/VehicleComponents/Others/WheelsSkidEffect.h"
#include "Soda/Vehicles/SodaWheeledVehicle.h"
#include "Particles/ParticleSystemComponent.h"
#include "Soda/Vehicles/IWheeledVehicleMovementInterface.h"

UWheelsSkidEffectComponent::UWheelsSkidEffectComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;

	GUI.Category = TEXT("Other");
	GUI.ComponentNameOverride = TEXT("Skid Effect");
	GUI.bIsPresentInAddMenu = true;
}

void UWheelsSkidEffectComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (!HealthIsWorkable()) return;

	if (MarkParticleSystem.Num() == SmokeParticleSystem.Num() == GetWheeledVehicle()->GetWheelsSorted().Num())
	{
		for (int i = 0; i < GetWheeledVehicle()->GetWheelsSorted().Num(); ++i)
		{
			USodaVehicleWheelComponent * SodaWheel = GetWheeledVehicle()->GetWheelsSorted()[i];

			FVector GlobalVelocity = GetWheeledComponentInterface()->GetSimData().VehicleKinematic.Curr.GetGlobaVelocityAtGlobalPoint(SodaWheel->GetWheelLocation(true, true));
			//FVector LocalVelocity = GetVehicleDriver()->GetLastOutputRegs().Dyn.Curr.GlobalPose.Rotator().UnrotateVector(GlobalVelocity);
			FQuat Quat = GlobalVelocity.Rotation().Quaternion();
			//FVector2D Slip2D = GetWheeledComponentInterface()->GetWheelSlip(i);
			//float Slip = FMath::Abs(GetWheeledComponentInterface()->GetWheelSlip(i).Size());

			FVector WheelVelocity = GetWheeledVehicle()->GetWheelsSorted()[i]->GetWheelLocalVelocity();

			float SlipVx = WheelVelocity.X - SodaWheel->AngularVelocity * SodaWheel->Radius;
			float SlipVy = WheelVelocity.Y;
			float SlipV = FMath::Sqrt(SlipVx * SlipVx + SlipVy * SlipVy);

			if (MarkParticleSystem.Num() == 4)
			{
				MarkParticleSystem[i]->SetActive(SlipV > SkidEffectTreshold);
				MarkParticleSystem[i]->SetWorldRotation(Quat * FRotator(0, 0, 90).Quaternion());
				MarkParticleSystem[i]->SetRelativeLocation(
					SodaWheel->GetWheelLocation(false, false) +
					FVector(0, 0, -SodaWheel->Radius + 2.0));
			}

			if (SmokeParticleSystem.Num() == 4)
			{
				SmokeParticleSystem[i]->SetActive(SlipV > SkidEffectTreshold);
				SmokeParticleSystem[i]->SetFloatParameter(TEXT("Rate"), SlipV * SkidSmokeRateMultiplier);
				SmokeParticleSystem[i]->SetWorldRotation(Quat);
				MarkParticleSystem[i]->SetRelativeLocation(
					SodaWheel->GetWheelLocation(false, false) +
					FVector(0, 0, -SodaWheel->Radius + 2.0));
			}
		}
	}
}

bool UWheelsSkidEffectComponent::OnActivateVehicleComponent()
{
	if (!Super::OnActivateVehicleComponent())
	{
		return false;
	}

	if (MarkEmitterTemplate)
	{
		for (int i = 0; i < GetWheeledVehicle()->GetWheelsSorted().Num(); ++i)
		{
			UParticleSystemComponent* PSC = NewObject<UParticleSystemComponent>(this);
			PSC->SetupAttachment(GetWheeledVehicle()->GetMesh());
			PSC->RegisterComponent();
			PSC->bAutoDestroy = false;
			PSC->bAllowAnyoneToDestroyMe = true;
			PSC->SecondsBeforeInactive = 0.0f;
			PSC->bAutoActivate = false;
			PSC->SetTemplate(MarkEmitterTemplate);
			PSC->bOverrideLODMethod = false;
			PSC->SetRelativeRotation(FRotator(0, 0, 90));
			PSC->ActivateSystem(true);
			PSC->SetTranslucentSortPriority(0);
			MarkParticleSystem.Add(PSC);
		}
	}

	if (SmokeEmitterTemplate)
	{
		for (int i = 0; i < GetWheeledVehicle()->GetWheelsSorted().Num(); ++i)
		{
			UParticleSystemComponent* PSC = NewObject<UParticleSystemComponent>(this);
			PSC->SetupAttachment(GetWheeledVehicle()->GetMesh());
			PSC->RegisterComponent();
			PSC->bAutoDestroy = false;
			PSC->bAllowAnyoneToDestroyMe = true;
			PSC->SecondsBeforeInactive = 0.0f;
			PSC->bAutoActivate = false;
			PSC->SetTemplate(SmokeEmitterTemplate);
			PSC->bOverrideLODMethod = false;
			PSC->SetRelativeRotation(FRotator(0, 0, 90));
			PSC->ActivateSystem(true);
			PSC->SetTranslucentSortPriority(1);
			SmokeParticleSystem.Add(PSC);
		}
	}

	return true;
}

void UWheelsSkidEffectComponent::OnDeactivateVehicleComponent()
{
	Super::OnDeactivateVehicleComponent();

	for (auto& It : MarkParticleSystem)
	{
		It->DestroyComponent();
	}
	MarkParticleSystem.Empty();

	for (auto& It : SmokeParticleSystem)
	{
		It->DestroyComponent();
	}
	SmokeParticleSystem.Empty();
}


