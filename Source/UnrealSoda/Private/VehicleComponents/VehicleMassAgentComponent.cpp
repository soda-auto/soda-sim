// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/VehicleComponents/VehicleMassAgentComponent.h"
#include "Soda/Vehicles/SodaVehicle.h"
#include "Soda/UnrealSoda.h"
#include "MassActorHelper.h"
#include "MassAgentSubsystem.h"
#include "MassEntitySubsystem.h"
#include "MassActorSubsystem.h"
#include "MassEntityView.h"

UVehicleMassAgentComponent::UVehicleMassAgentComponent()
{
	GUI.Category = TEXT("Mass");
	GUI.IcanName = TEXT("SodaIcons.Tire");
	GUI.bIsPresentInAddMenu = true;

/*
#if WITH_EDITOR
	auto* EntityConfigProperty = FindFProperty<FProperty>(UVehicleMassAgentComponent::StaticClass(), *GET_MEMBER_NAME_CHECKED(UVehicleMassAgentComponent, EntityConfig).ToString());
	check(EntityConfigProperty);
	EntityConfigProperty->SetMetaData(TEXT("EditInRuntime"), TEXT(""));
#endif
*/
}

ASodaVehicle* UVehicleMassAgentComponent::GetVehicle() const 
{
	return Cast<ASodaVehicle>(GetOwner()); 
}

bool UVehicleMassAgentComponent::OnActivateVehicleComponent()
{
	if (!ISodaVehicleComponent::OnActivateVehicleComponent())
	{
		return false;
	}

	if (Traits.Num())
	{
		//TODO: use UMassAgentSubsystem::UpdateAgentComponent() insted of UnregisterWithAgentSubsystem() and RegisterWithAgentSubsystem(), but it isn't implemented now in (UE5.4)
		UnregisterWithAgentSubsystem();

		FMassEntityConfig NewConfig;
		if (EntityConfig.GetParent())
		{
			NewConfig.SetParentAsset(*EntityConfig.GetParent());
		}
			
		for (auto& It : OriginEntityConfig.GetTraits())
		{
			NewConfig.AddTrait(*It);
		}

		for (auto& It : Traits)
		{
			NewConfig.AddTrait(*It);
		}

		SetEntityConfig(NewConfig);

		RegisterWithAgentSubsystem();
	}

	return true;
}

void UVehicleMassAgentComponent::OnDeactivateVehicleComponent()
{
	ISodaVehicleComponent::OnDeactivateVehicleComponent();

	if (Traits.Num())
	{
		UnregisterWithAgentSubsystem();
		SetEntityConfig(OriginEntityConfig);
		RegisterWithAgentSubsystem();
	}
}

#if WITH_EDITOR
void UVehicleMassAgentComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	if (!HasBegunPlay())
	{
		Super::PostEditChangeProperty(PropertyChangedEvent);
	}
}

void UVehicleMassAgentComponent::PostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent)
{
	if (!HasBegunPlay())
	{
		Super::PostEditChangeChainProperty(PropertyChangedEvent);
	}
}
#endif

void UVehicleMassAgentComponent::OnRegister()
{
	OriginEntityConfig = EntityConfig;
	if (UMassAgentSubsystem* AgentSubsystem = UWorld::GetSubsystem<UMassAgentSubsystem>(GetWorld()))
	{
		AgentSubsystem->GetOnMassAgentComponentEntityAssociated().AddUObject(this, &UVehicleMassAgentComponent::OnMassAgentComponentEntityAssociated);
	}

	Super::OnRegister();
}

void UVehicleMassAgentComponent::OnUnregister()
{
	Super::OnUnregister();

	if (UMassAgentSubsystem* AgentSubsystem = UWorld::GetSubsystem<UMassAgentSubsystem>(GetWorld()))
	{
		AgentSubsystem->GetOnMassAgentComponentEntityAssociated().RemoveAll(this);
	}
}

void UVehicleMassAgentComponent::OnMassAgentComponentEntityAssociated(const UMassAgentComponent& AgentComponent)
{
	if (&AgentComponent == this)
	{
		if (UMassEntitySubsystem* EntitySubsystem = UWorld::GetSubsystem<UMassEntitySubsystem>(GetWorld()))
		{
			if (AgentHandle.IsValid())
			{
				const FMassEntityView EntityView(EntitySubsystem->GetEntityManager(), AgentHandle);
				if (FMassActorFragment* ActorInfo = EntityView.GetFragmentDataPtr<FMassActorFragment>())
				{
					checkf(!ActorInfo->IsValid(), TEXT("Expecting ActorInfo fragment to be null"));
					ActorInfo->SetAndUpdateHandleMap(AgentHandle, GetOwner(), false);
				}
			}
		}
	}
}

