// Copyright Epic Games, Inc. All Rights Reserved.
// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "RuntimePropertyEditor/StructurePropertyNode.h"
#include "RuntimePropertyEditor/ItemPropertyNode.h"
#include "RuntimePropertyEditor/PropertyEditorHelpers.h"

namespace soda
{

	void FStructurePropertyNode::InitChildNodes()
	{
		const bool bShouldShowHiddenProperties = !!HasNodeFlags(EPropertyNodeFlags::ShouldShowHiddenProperties);
		const bool bShouldShowDisableEditOnInstance = !!HasNodeFlags(EPropertyNodeFlags::ShouldShowDisableEditOnInstance);

		const UStruct* Struct = GetBaseStructure();

		TArray<FProperty*> StructMembers;

		for (TFieldIterator<FProperty> It(Struct); It; ++It)
		{
			FProperty* StructMember = *It;
			if (Cast<UFunction>(Struct) != nullptr || PropertyEditorHelpers::ShouldBeVisible(*this, StructMember)) // TODO: @ivanzhuk, check "Cast<UFunction>(Struct) != nullptr" is corrent 
			{
				StructMembers.Add(StructMember);
			}
		}

		PropertyEditorHelpers::OrderPropertiesFromMetadata(StructMembers);

		for (FProperty* StructMember : StructMembers)
		{
			TSharedPtr<FItemPropertyNode> NewItemNode(new FItemPropertyNode);

			FPropertyNodeInitParams InitParams;
			InitParams.ParentNode = SharedThis(this);
			InitParams.Property = StructMember;
			InitParams.ArrayOffset = 0;
			InitParams.ArrayIndex = INDEX_NONE;
			InitParams.bAllowChildren = true;
			InitParams.bForceHiddenPropertyVisibility = bShouldShowHiddenProperties;
			InitParams.bCreateDisableEditOnInstanceNodes = bShouldShowDisableEditOnInstance;
			InitParams.bCreateCategoryNodes = false;

			NewItemNode->InitNode(InitParams);
			AddChildNode(NewItemNode);
		}
	}

	uint8* FStructurePropertyNode::GetValueBaseAddress(uint8* StartAddress, bool bIsSparseData, bool bIsStruct) const
	{
		// If called with struct data, we expect that it is compatible with the first structure node down in the property node chain, return the address as is.
		// This gets called usually when the calling code is dealing with a parent complex node.
		if (bIsStruct)
		{
			return StartAddress;
		}

		if (StructProvider)
		{
			// Assume that this code gets called with an object or object sparse data.
			//
			// The passed object might not be the one that contains the values provided by struct provider.
			// For example this function might get called on an edited object's template object.
			// In that case the data structure is expected to match between the data edited by this node and the foreign object.

			// If the structure provider is set up as indirection, then it knows how to translate parent node's value address to
			// new value address even on data that is not the same as in the structure provider.
			if (StructProvider->IsPropertyIndirection())
			{
				const TSharedPtr<FPropertyNode> ParentNode = ParentNodeWeakPtr.Pin();
				if (!ensureMsgf(ParentNode, TEXT("Expecting valid parent node when indirection structure provider is called with Object data.")))
				{
					return nullptr;
				}
				// Resolve from parent nodes data.
				uint8* ParentValueAddress = ParentNode->GetValueAddress(StartAddress, bIsSparseData);
				uint8* ValueAddress = StructProvider->GetValueBaseAddress(ParentValueAddress, GetBaseStructure());
				return ValueAddress;
			}

			// The struct is really standalone, in which case we always return the standalone struct data.
			// In that case we can only support one instance, since we cannot discern them.
			// Note: Multiple standalone structure instances are supported when bIsStruct is true (e.g. when the structure property is root node).
			TArray<TSharedPtr<FStructOnScope>> Instances;
			StructProvider->GetInstances(Instances);
			ensureMsgf(Instances.Num() <= 1, TEXT("Expecting max one instance on standalone structure provider."));
			if (Instances.Num() == 1 && Instances[0].IsValid())
			{
				return Instances[0]->GetStructMemory();
			}
		}

		return nullptr;
	}

	EPropertyDataValidationResult FStructurePropertyNode::EnsureDataIsValid()
	{
		CachedReadAddresses.Reset();

		const UStruct* BaseStruct = GetBaseStructure();

		// Check that struct node's children still belong to the current base struct.
		for (const TSharedPtr<FPropertyNode>& ChildNode : ChildNodes)
		{
			if (ChildNode.IsValid())
			{
				if (const FProperty* ChildProperty = ChildNode->GetProperty())
				{
					const UStruct* OwnerStruct = ChildProperty->GetOwnerStruct();
					if (!OwnerStruct
						|| OwnerStruct->IsStructTrashed()
						|| !BaseStruct
						|| !BaseStruct->IsChildOf(OwnerStruct)) // OwnerStruct can be BaseStruct or one of structs BaseStruct is derived from.
					{
						RebuildChildren();
						return EPropertyDataValidationResult::ChildrenRebuilt;
					}
				}
			}
		}

		return FPropertyNode::EnsureDataIsValid();
	}


} // namespace soda
