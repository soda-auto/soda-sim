// Copyright Epic Games, Inc. All Rights Reserved.
// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Templates/SubclassOf.h"
#include "Components/ActorComponent.h"
#include "Engine/EngineTypes.h"
#include "GameFramework/Actor.h"
#include "UObject/UObjectHash.h"

namespace soda
{

class FComponentEditorUtils
{
public:

	// Given a template and a property, propagates a default value change to all instances (only if applicable)
	template<typename T>
	static void PropagateDefaultValueChange(class USceneComponent* InSceneComponentTemplate, const class FProperty* InProperty, const T& OldDefaultValue, const T& NewDefaultValue, TSet<class USceneComponent*>& UpdatedInstances, int32 PropertyOffset = INDEX_NONE)
	{
		TArray<UObject*> ArchetypeInstances;
		if(InSceneComponentTemplate->HasAnyFlags(RF_ArchetypeObject))
		{
			InSceneComponentTemplate->GetArchetypeInstances(ArchetypeInstances);
			for(int32 InstanceIndex = 0; InstanceIndex < ArchetypeInstances.Num(); ++InstanceIndex)
			{
				USceneComponent* InstancedSceneComponent = static_cast<USceneComponent*>(ArchetypeInstances[InstanceIndex]);
				if(InstancedSceneComponent != nullptr && !UpdatedInstances.Contains(InstancedSceneComponent) && ApplyDefaultValueChange(InstancedSceneComponent, InProperty, OldDefaultValue, NewDefaultValue, PropertyOffset))
				{
					UpdatedInstances.Add(InstancedSceneComponent);
				}
			}
		}
		else if(UObject* Outer = InSceneComponentTemplate->GetOuter())
		{
			Outer->GetArchetypeInstances(ArchetypeInstances);
			for(int32 InstanceIndex = 0; InstanceIndex < ArchetypeInstances.Num(); ++InstanceIndex)
			{
				USceneComponent* InstancedSceneComponent = static_cast<USceneComponent*>(FindObjectWithOuter(ArchetypeInstances[InstanceIndex], InSceneComponentTemplate->GetClass(), InSceneComponentTemplate->GetFName()));
				if(InstancedSceneComponent != nullptr && !UpdatedInstances.Contains(InstancedSceneComponent) && ApplyDefaultValueChange(InstancedSceneComponent, InProperty, OldDefaultValue, NewDefaultValue, PropertyOffset))
				{
					UpdatedInstances.Add(InstancedSceneComponent);
				}
			}
		}
	}

	// Given an instance of a template and a property, set a default value change to the instance (only if applicable)
	template<typename T>
	static bool ApplyDefaultValueChange(class USceneComponent* InSceneComponent, const class FProperty* InProperty, const T& OldDefaultValue, const T& NewDefaultValue, int32 PropertyOffset)
	{
		check(InProperty != nullptr);
		check(InSceneComponent != nullptr);

		ensureMsgf(CastField<FBoolProperty>(InProperty) == nullptr, TEXT("ApplyDefaultValueChange cannot be safely called on a bool property with a non-bool value, becuase of bitfields"));

		T* CurrentValue = PropertyOffset == INDEX_NONE ? InProperty->ContainerPtrToValuePtr<T>(InSceneComponent) : (T*)((uint8*)InSceneComponent + PropertyOffset);
		check(CurrentValue);

		return ApplyDefaultValueChange(InSceneComponent, *CurrentValue, OldDefaultValue, NewDefaultValue);
	}

	// Bool specialization so it can properly handle bitfields
	static bool ApplyDefaultValueChange(class USceneComponent* InSceneComponent, const class FProperty* InProperty, const bool& OldDefaultValue, const bool& NewDefaultValue, int32 PropertyOffset)
	{
		check(InProperty != nullptr);
		check(InSceneComponent != nullptr);
		
		// Only bool properties can have bool values
		const FBoolProperty* BoolProperty = CastField<FBoolProperty>(InProperty);
		check(BoolProperty);

		uint8* CurrentValue = PropertyOffset == INDEX_NONE ? InProperty->ContainerPtrToValuePtr<uint8>(InSceneComponent) : ((uint8*)InSceneComponent + PropertyOffset);
		check(CurrentValue);

		bool CurrentBool = BoolProperty->GetPropertyValue(CurrentValue);
		if (ApplyDefaultValueChange(InSceneComponent, CurrentBool, OldDefaultValue, NewDefaultValue, false))
		{
			BoolProperty->SetPropertyValue(CurrentValue, CurrentBool);

			InSceneComponent->ReregisterComponent();

			return true;
		}

		return false;
	}

	// Given an instance of a template and a current value, propagates a default value change to the instance (only if applicable)
	template<typename T>
	static bool ApplyDefaultValueChange(class USceneComponent* InSceneComponent, T& CurrentValue, const T& OldDefaultValue, const T& NewDefaultValue, bool bReregisterComponent = true)
	{
		check(InSceneComponent != nullptr);

		// Propagate the change only if the current instanced value matches the previous default value (otherwise this could overwrite any per-instance override)
		if(NewDefaultValue != OldDefaultValue && CurrentValue == OldDefaultValue)
		{
			// Ensure that this instance will be included in any undo/redo operations, and record it into the transaction buffer.
			// Note: We don't do this for components that originate from script, because they will be re-instanced from the template after an undo, so there is no need to record them.
			if (!InSceneComponent->IsCreatedByConstructionScript())
			{
				InSceneComponent->SetFlags(RF_Transactional);
				InSceneComponent->Modify();
			}

			// We must also modify the owner, because we'll need script components to be reconstructed as part of an undo operation.
			AActor* Owner = InSceneComponent->GetOwner();
			if(Owner != nullptr)
			{
				Owner->Modify();
			}

			// Modify the value
			CurrentValue = NewDefaultValue;

			if (bReregisterComponent && InSceneComponent->IsRegistered())
			{
				// Re-register the component with the scene so that transforms are updated for display
				if (InSceneComponent->AllowReregistration())
				{
					InSceneComponent->ReregisterComponent();
				}
				else
				{
					InSceneComponent->UpdateComponentToWorld();
				}
			}
			
			return true;
		}

		return false;
	}

	// Try to find the correct variable name for a given native component template or instance (which can have a mismatch)
	static FName FindVariableNameGivenComponentInstance(const UActorComponent* ComponentInstance);

	/** Is the instance component is editable */
	static bool CanEditComponentInstance(const UActorComponent* ActorComp, const UActorComponent* ParentSceneComp, bool bAllowUserContructionScript);

	/**
	 * Make a FComponentReference from a component pointer.
	 * @param ExpectedComponentOwner The expected component owner. Should be the same as OwningActor from FComponentReference::GetComponent().
	 * @param Component The component we would like to initialize the FComponentReference with.
	 */
	static FComponentReference MakeComponentReference(const AActor* ExpectedComponentOwner, const UActorComponent* Component);

	/**
	* Test if the native component is editable. If it is, return a valid pointer to it's FProperty
	* Otherwise, return nullptr. A native component is editable if it is marked as EditAnywhere
	* via meta data tags or is within an editable property container
	*/
	static FProperty* GetPropertyForEditableNativeComponent(const UActorComponent* NativeComponent);
};

} // namespace soda