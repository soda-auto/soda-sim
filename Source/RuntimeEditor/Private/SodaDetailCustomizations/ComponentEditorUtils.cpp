// Copyright Epic Games, Inc. All Rights Reserved.
// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "SodaDetailCustomizations/ComponentEditorUtils.h"
//#include "HAL/FileManager.h"
#include "Misc/Paths.h"
#include "UObject/PropertyPortFlags.h"
#include "Textures/SlateIcon.h"
#include "Framework/Commands/UIAction.h"
//#include "EditorStyleSet.h"
#include "Components/PrimitiveComponent.h"
#include "Components/MeshComponent.h"
//#include "Exporters/Exporter.h"
//#include "Materials/Material.h"
//#include "Editor/UnrealEdEngine.h"
//#include "Components/DecalComponent.h"
//#include "UnrealEdGlobals.h"
//#include "ScopedTransaction.h"
//#include "EdGraphSchema_K2.h"
//#include "Kismet2/BlueprintEditorUtils.h"
//#include "Kismet2/KismetEditorUtilities.h"
//#include "Factories.h"
//#include "UnrealExporter.h"
//#include "Framework/Commands/GenericCommands.h"
//#include "SourceCodeNavigation.h"
//#include "Styling/SlateIconFinder.h"
//#include "Framework/MultiBox/MultiBoxBuilder.h"
//#include "Editor.h"

//#include "HAL/PlatformApplicationMisc.h"
//#include "ToolMenus.h"
//#include "Kismet2/ComponentEditorContextMenuContex.h"
//#include "Subsystems/AssetEditorSubsystem.h"

#define LOCTEXT_NAMESPACE "ComponentEditorUtils"

namespace soda
{

bool FComponentEditorUtils::CanEditComponentInstance(const UActorComponent* ActorComp, const UActorComponent* ParentSceneComp, bool bAllowUserContructionScript)
{
	// Exclude nested DSOs attached to BP-constructed instances, which are not mutable.
	return (ActorComp != nullptr
		//&& (!ActorComp->IsVisualizationComponent())
		&& (ActorComp->CreationMethod != EComponentCreationMethod::UserConstructionScript || bAllowUserContructionScript)
		&& (ParentSceneComp == nullptr || !ParentSceneComp->IsCreatedByConstructionScript() || !ActorComp->HasAnyFlags(RF_DefaultSubObject)))
		&& (ActorComp->CreationMethod != EComponentCreationMethod::Native || FComponentEditorUtils::GetPropertyForEditableNativeComponent(ActorComp));
}

FName FComponentEditorUtils::FindVariableNameGivenComponentInstance(const UActorComponent* ComponentInstance)
{
	check(ComponentInstance != nullptr);

	// When names mismatch, try finding a differently named variable pointing to the the component (the mismatch should only be possible for native components)
	auto FindPropertyReferencingComponent = [](const UActorComponent* Component) -> FProperty*
	{
		if (AActor* OwnerActor = Component->GetOwner())
		{
			UClass* OwnerClass = OwnerActor->GetClass();
			AActor* OwnerCDO = CastChecked<AActor>(OwnerClass->GetDefaultObject());
			check(OwnerCDO->HasAnyFlags(RF_ClassDefaultObject));

			for (TFieldIterator<FObjectProperty> PropIt(OwnerClass, EFieldIteratorFlags::IncludeSuper); PropIt; ++PropIt)
			{
				FObjectProperty* TestProperty = *PropIt;
				if (Component->GetClass()->IsChildOf(TestProperty->PropertyClass))
				{
					void* TestPropertyInstanceAddress = TestProperty->ContainerPtrToValuePtr<void>(OwnerCDO);
					UObject* ObjectPointedToByProperty = TestProperty->GetObjectPropertyValue(TestPropertyInstanceAddress);
					if (ObjectPointedToByProperty == Component)
					{
						// This property points to the component archetype, so it's an anchor even if it was named wrong
						return TestProperty;
					}
				}
			}

			for (TFieldIterator<FArrayProperty> PropIt(OwnerClass, EFieldIteratorFlags::IncludeSuper); PropIt; ++PropIt)
			{
				FArrayProperty* TestProperty = *PropIt;
				void* ArrayPropInstAddress = TestProperty->ContainerPtrToValuePtr<void>(OwnerCDO);

				FObjectProperty* ArrayEntryProp = CastField<FObjectProperty>(TestProperty->Inner);
				if ((ArrayEntryProp == nullptr) || !ArrayEntryProp->PropertyClass->IsChildOf<UActorComponent>())
				{
					continue;
				}

				FScriptArrayHelper ArrayHelper(TestProperty, ArrayPropInstAddress);
				for (int32 ComponentIndex = 0; ComponentIndex < ArrayHelper.Num(); ++ComponentIndex)
				{
					UObject* ArrayElement = ArrayEntryProp->GetObjectPropertyValue(ArrayHelper.GetRawPtr(ComponentIndex));
					if (ArrayElement == Component)
					{
						return TestProperty;
					}
				}
			}
		}

		return nullptr;
	};

	// First see if the name just works
	if (AActor* OwnerActor = ComponentInstance->GetOwner())
	{
		UClass* OwnerActorClass = OwnerActor->GetClass();
		if (FObjectProperty* TestProperty = FindFProperty<FObjectProperty>(OwnerActorClass, ComponentInstance->GetFName()))
		{
			if (ComponentInstance->GetClass()->IsChildOf(TestProperty->PropertyClass))
					{
						return TestProperty->GetFName();
					}
				}

		if (FProperty* ReferencingProp = FindPropertyReferencingComponent(ComponentInstance))
		{
			return ReferencingProp->GetFName();
		}
			}

	if (UActorComponent* Archetype = Cast<UActorComponent>(ComponentInstance->GetArchetype()))
	{
		if (FProperty* ReferencingProp = FindPropertyReferencingComponent(Archetype))
		{
			return ReferencingProp->GetFName();
		}
	}

	return NAME_None;
}

FComponentReference FComponentEditorUtils::MakeComponentReference(const AActor* InExpectedComponentOwner, const UActorComponent* InComponent)
{
	FComponentReference Result;
	if (InComponent)
	{
		AActor* ComponentOwner = InComponent->GetOwner();
		const AActor* Owner = InExpectedComponentOwner;
		if (ComponentOwner && ComponentOwner != Owner)
		{
			Result.OtherActor = ComponentOwner;
			Owner = ComponentOwner;
		}

		if (InComponent->CreationMethod == EComponentCreationMethod::Native || InComponent->CreationMethod == EComponentCreationMethod::SimpleConstructionScript)
		{
			Result.ComponentProperty = FComponentEditorUtils::FindVariableNameGivenComponentInstance(InComponent);
		}
		if (Result.ComponentProperty.IsNone() && InComponent->CreationMethod != EComponentCreationMethod::UserConstructionScript)
		{
			Result.PathToComponent = InComponent->GetPathName(ComponentOwner);
		}
	}
	return Result;
}

FProperty* FComponentEditorUtils::GetPropertyForEditableNativeComponent(const UActorComponent* NativeComponent)
{
	// A native component can be edited if it is bound to a member variable and that variable is marked as visible in the editor
	// Note: We aren't concerned with whether the component is marked editable - the component itself is responsible for determining which of its properties are editable	
	UObject* ComponentOuter = (NativeComponent ? NativeComponent->GetOuter() : nullptr);
	UClass* OwnerClass = (ComponentOuter ? ComponentOuter->GetClass() : nullptr);
	UObject* OwnerCDO = (OwnerClass ? OwnerClass->GetDefaultObject() : nullptr);

	if (OwnerClass != nullptr)
	{
		for (TFieldIterator<FObjectProperty> It(OwnerClass); It; ++It)
		{
			FObjectProperty* ObjectProp = *It;

			// Must be visible - note CPF_Edit is set for all properties that should be visible, not just those that are editable
			if ((ObjectProp->PropertyFlags & (CPF_Edit)) == 0)
			{
				continue;
			}

			UObject* Object = ObjectProp->GetObjectPropertyValue(ObjectProp->ContainerPtrToValuePtr<void>(ComponentOuter));
			if (Object != nullptr && Object->GetFName() == NativeComponent->GetFName())
			{
				return ObjectProp;
			}
		}

		// We have to check for array properties as well because they are not FObjectProperties and we want to be able to
		// edit the inside of it
		if (OwnerCDO != nullptr)
		{
			for (TFieldIterator<FArrayProperty> PropIt(OwnerClass, EFieldIteratorFlags::IncludeSuper); PropIt; ++PropIt)
			{
				FArrayProperty* TestProperty = *PropIt;
				void* ArrayPropInstAddress = TestProperty->ContainerPtrToValuePtr<void>(OwnerCDO);

				// Ensure that this property is valid
				FObjectProperty* ArrayEntryProp = CastField<FObjectProperty>(TestProperty->Inner);
				if ((ArrayEntryProp == nullptr) || !ArrayEntryProp->PropertyClass->IsChildOf<UActorComponent>() || ((TestProperty->PropertyFlags & CPF_Edit) == 0))
				{
					continue;
				}

				// For each object in this array
				FScriptArrayHelper ArrayHelper(TestProperty, ArrayPropInstAddress);
				for (int32 ComponentIndex = 0; ComponentIndex < ArrayHelper.Num(); ++ComponentIndex)
				{
					// If this object is the native component we are looking for, then we should be allowed to edit it
					const uint8* ArrayValRawPtr = ArrayHelper.GetRawPtr(ComponentIndex);
					UObject* ArrayElement = ArrayEntryProp->GetObjectPropertyValue(ArrayValRawPtr);

					if (ArrayElement != nullptr && ArrayElement->GetFName() == NativeComponent->GetFName())
					{
						return ArrayEntryProp;
					}
				}
			}

			// Check for map properties as well
			for (TFieldIterator<FMapProperty> PropIt(OwnerClass, EFieldIteratorFlags::IncludeSuper); PropIt; ++PropIt)
			{
				FMapProperty* TestProperty = *PropIt;
				void* MapPropInstAddress = TestProperty->ContainerPtrToValuePtr<void>(OwnerCDO);

				// Ensure that this property is valid and that it is marked as visible in the editor
				FObjectProperty* MapValProp = CastField<FObjectProperty>(TestProperty->ValueProp);
				if ((MapValProp == nullptr) || !MapValProp->PropertyClass->IsChildOf<UActorComponent>() || ((TestProperty->PropertyFlags & CPF_Edit) == 0))
				{
					continue;
				}

				FScriptMapHelper MapHelper(TestProperty, MapPropInstAddress);
				for (int32 MapSparseIndex = 0; MapSparseIndex < MapHelper.GetMaxIndex(); ++MapSparseIndex)
				{
					// For each value in the map (don't bother checking the keys, they won't be what the user can edit in this case)
					if (MapHelper.IsValidIndex(MapSparseIndex))
					{
						const uint8* MapValueData = MapHelper.GetValuePtr(MapSparseIndex);
						UObject* ValueElement = MapValProp->GetObjectPropertyValue(MapValueData);
						if (ValueElement != nullptr && ValueElement->GetFName() == NativeComponent->GetFName())
						{
							return MapValProp;
						}
					}
				}
			}

			// Finally we should check for set properties
			for (TFieldIterator<FSetProperty> PropIt(OwnerClass, EFieldIteratorFlags::IncludeSuper); PropIt; ++PropIt)
			{
				FSetProperty* TestProperty = *PropIt;
				void* SetPropInstAddress = TestProperty->ContainerPtrToValuePtr<void>(OwnerCDO);

				// Ensure that this property is valid and that it is marked visible
				FObjectProperty* SetValProp = CastField<FObjectProperty>(TestProperty->ElementProp);
				if ((SetValProp == nullptr) || !SetValProp->PropertyClass->IsChildOf<UActorComponent>() || ((TestProperty->PropertyFlags & CPF_Edit) == 0))
				{
					continue;
				}

				// For each item in the set
				FScriptSetHelper SetHelper(TestProperty, SetPropInstAddress);
				for (int32 i = 0; i < SetHelper.Num(); ++i)
				{
					if (SetHelper.IsValidIndex(i))
					{
						const uint8* SetValData = SetHelper.GetElementPtr(i);
						UObject* SetValueElem = SetValProp->GetObjectPropertyValue(SetValData);

						if (SetValueElem != nullptr && SetValueElem->GetFName() == NativeComponent->GetFName())
						{
							return SetValProp;
						}
					}
				}
			}
		}
	}

	return nullptr;
}

}


#undef LOCTEXT_NAMESPACE
