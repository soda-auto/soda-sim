// Copyright Epic Games, Inc. All Rights Reserved.
// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PropertyNode.h"
#include "UObject/StructOnScope.h"
#include "RuntimePropertyEditor/IStructureDataProvider.h"

namespace soda
{
//-----------------------------------------------------------------------------
//	FStructPropertyNode - Used for the root and various sub-nodes
//-----------------------------------------------------------------------------
class FStructurePropertyNode : public FComplexPropertyNode
{
public:
	FStructurePropertyNode() : FComplexPropertyNode() {}
	virtual ~FStructurePropertyNode() override {}

	virtual FStructurePropertyNode* AsStructureNode() override { return this; }
	virtual const FStructurePropertyNode* AsStructureNode() const override { return this; }

	void RemoveStructure()
	{
		ClearCachedReadAddresses(true);
		DestroyTree();
		StructProvider = nullptr;
	}

	void SetStructure(TSharedPtr<FStructOnScope> InStructData)
	{
		RemoveStructure();
		if (InStructData)
		{
			StructProvider = MakeShared<FStructOnScopeStructureDataProvider>(InStructData);
		}
	}

	void SetStructure(TSharedPtr<IStructureDataProvider> InStructProvider)
	{
		RemoveStructure();
		StructProvider = InStructProvider;
	}

	bool HasValidStructData() const
	{
		return StructProvider.IsValid() && StructProvider->IsValid();
	}

	// Returns just the first structure. Please use GetStructProvider() or GetAllStructureData() when dealing with multiple struct instances.
	TSharedPtr<FStructOnScope> GetStructData() const
	{
		if (StructProvider)
		{
			TArray<TSharedPtr<FStructOnScope>> Instances;
			StructProvider->GetInstances(Instances);
			
			if (Instances.Num() > 0)
			{
				return Instances[0];
			}
		}
		return nullptr;
	}

	void GetAllStructureData(TArray<TSharedPtr<FStructOnScope>>& OutStructs) const
	{
		if (StructProvider)
		{
			StructProvider->GetInstances(OutStructs);
		}
	}

	TSharedPtr<IStructureDataProvider> GetStructProvider() const
	{
		return StructProvider;
	}

	bool GetReadAddressUncached(const FPropertyNode& InPropertyNode, FReadAddressListData& OutAddresses) const override
	{
		if (!HasValidStructData())
		{
			return false;
		}
		check(StructProvider.IsValid());

		const FProperty* InItemProperty = InPropertyNode.GetProperty();
		if (!InItemProperty)
		{
			return false;
		}

		UStruct* OwnerStruct = InItemProperty->GetOwnerStruct();
		if (!OwnerStruct || OwnerStruct->IsStructTrashed())
		{
			// Verify that the property is not part of an invalid trash class
			return false;
		}

		TArray<TSharedPtr<FStructOnScope>> Instances;
		StructProvider->GetInstances(Instances);
		bool bHasData = false;

		for (TSharedPtr<FStructOnScope>& Instance : Instances)
		{
			uint8* ReadAddress = Instance.IsValid() ? Instance->GetStructMemory() : nullptr;
			if (ReadAddress)
			{
				OutAddresses.Add(nullptr, InPropertyNode.GetValueBaseAddress(ReadAddress, InPropertyNode.HasNodeFlags(EPropertyNodeFlags::IsSparseClassData) != 0, /*bIsStruct=*/true), /*bIsStruct=*/true);
				bHasData = true;
			}
		}
		return bHasData;
	}

	bool GetReadAddressUncached(const FPropertyNode& InPropertyNode,
		bool InRequiresSingleSelection,
		FReadAddressListData* OutAddresses,
		bool bComparePropertyContents,
		bool bObjectForceCompare,
		bool bArrayPropertiesCanDifferInSize) const override
	{
		if (!HasValidStructData())
		{
			return false;
		}
		check(StructProvider.IsValid());

		const FProperty* InItemProperty = InPropertyNode.GetProperty();
		if (!InItemProperty)
		{
			return false;
		}

		const UStruct* OwnerStruct = InItemProperty->GetOwnerStruct();
		if (!OwnerStruct || OwnerStruct->IsStructTrashed())
		{
			// Verify that the property is not part of an invalid trash class
			return false;
		}

		bool bAllTheSame = true;

		TArray<TSharedPtr<FStructOnScope>> Instances;
		StructProvider->GetInstances(Instances);
		
		if (Instances.IsEmpty())
		{
			return false;
		}

		if (bComparePropertyContents || bObjectForceCompare)
		{
			const bool bIsSparse = InPropertyNode.HasNodeFlags(EPropertyNodeFlags::IsSparseClassData) != 0;
			const uint8* BaseAddress = nullptr;
			const UStruct* BaseStruct = nullptr;

			for (TSharedPtr<FStructOnScope>& Instance : Instances)
			{
				if (Instance.IsValid())
				{
					if (const UStruct* Struct = Instance->GetStruct())
					{
						if (const uint8* ReadAddress = InPropertyNode.GetValueBaseAddress(Instance->GetStructMemory(), bIsSparse, /*bIsStruct=*/true))
						{
							if (!BaseAddress)
							{
								BaseAddress = ReadAddress;
								BaseStruct = Struct;
							}
							else
							{
								if (BaseStruct != Struct)
								{
									bAllTheSame = false;
									break;
								}
								if (!InItemProperty->Identical(BaseAddress, ReadAddress))
								{
									bAllTheSame = false;
									break;
								}
							}
						}
					}
				}
			}
		}
		else
		{
			// Check that all are valid or invalid.
			const UStruct* BaseStruct = Instances[0].IsValid() ? Instances[0]->GetStruct() : nullptr;
			for (int32 Index = 1; Index < Instances.Num(); Index++)
			{
				const UStruct* Struct = Instances[Index].IsValid() ? Instances[Index]->GetStruct() : nullptr;
				if (BaseStruct != Struct)
				{
					bAllTheSame = false;
					break;
				}
			}
		}

		if (bAllTheSame && OutAddresses)
		{
			for (TSharedPtr<FStructOnScope>& Instance : Instances)
			{
				uint8* ReadAddress = Instance.IsValid() ? Instance->GetStructMemory() : nullptr;
				if (ReadAddress)
				{
					OutAddresses->Add(nullptr, InPropertyNode.GetValueBaseAddress(ReadAddress, InPropertyNode.HasNodeFlags(EPropertyNodeFlags::IsSparseClassData) != 0, /*bIsStruct=*/true), /*bIsStruct=*/true);
				}
			}
		}

		return bAllTheSame;
	}

	void GetOwnerPackages(TArray<UPackage*>& OutPackages) const
	{
		if (StructProvider)
		{
			TArray<TSharedPtr<FStructOnScope>> Instances;
			StructProvider->GetInstances(Instances);

			for (TSharedPtr<FStructOnScope>& Instance : Instances)
			{
				// Returning null for invalid instances, to match instance count.
				OutPackages.Add(Instance.IsValid() ? Instance->GetPackage() : nullptr);
			}
		}
	}

	/** FComplexPropertyNode Interface */
	virtual const UStruct* GetBaseStructure() const override
	{ 
		if (StructProvider)
		{
			return StructProvider->GetBaseStructure();
		}
		return nullptr; 
	}
	virtual UStruct* GetBaseStructure() override
	{
		if (StructProvider)
		{
			return const_cast<UStruct*>(StructProvider->GetBaseStructure());
		}
		return nullptr; 
	}
	virtual TArray<UStruct*> GetAllStructures() override
	{
		TArray<UStruct*> RetVal;
		if (StructProvider)
		{
			TArray<TSharedPtr<FStructOnScope>> Instances;
			StructProvider->GetInstances(Instances);
			
			for (TSharedPtr<FStructOnScope>& Instance : Instances)
			{
				const UStruct* Struct = Instance.IsValid() ? Instance->GetStruct() : nullptr;
				if (Struct)
				{
					RetVal.AddUnique(const_cast<UStruct*>(Struct));
				}
			}
		}

		return RetVal;
	}
	virtual TArray<const UStruct*> GetAllStructures() const override
	{
		TArray<const UStruct*> RetVal;
		if (StructProvider)
		{
			TArray<TSharedPtr<FStructOnScope>> Instances;
			StructProvider->GetInstances(Instances);
			
			for (TSharedPtr<FStructOnScope>& Instance : Instances)
			{
				const UStruct* Struct = Instance.IsValid() ? Instance->GetStruct() : nullptr;
				if (Struct)
				{
					RetVal.AddUnique(Struct);
				}
			}
		}
		return RetVal;
	}
	virtual int32 GetInstancesNum() const override
	{
		if (StructProvider)
		{
			TArray<TSharedPtr<FStructOnScope>> Instances;
			StructProvider->GetInstances(Instances);
			
			return Instances.Num();
		}
		return 0;
		
	}
	virtual uint8* GetMemoryOfInstance(int32 Index) const override
	{
		if (StructProvider)
		{
			TArray<TSharedPtr<FStructOnScope>> Instances;
			StructProvider->GetInstances(Instances);
			if (Instances.IsValidIndex(Index) && Instances[Index].IsValid())
			{
				return Instances[Index]->GetStructMemory();
			}
		}
		return nullptr;
	}
	virtual uint8* GetValuePtrOfInstance(int32 Index, const FProperty* InProperty, const FPropertyNode* InParentNode) const override
	{ 
		if (InProperty == nullptr || InParentNode == nullptr)
		{
			return nullptr;
		}

		uint8* StructBaseAddress = GetMemoryOfInstance(Index);
		if (StructBaseAddress == nullptr)
		{
			return nullptr;
		}

		uint8* ParentBaseAddress = InParentNode->GetValueAddress(StructBaseAddress, false, /*bIsStruct=*/true);
		if (ParentBaseAddress == nullptr)
		{
			return nullptr;
		}

		return InProperty->ContainerPtrToValuePtr<uint8>(ParentBaseAddress);
	}

	virtual TWeakObjectPtr<UObject> GetInstanceAsUObject(int32 Index) const override
	{
		return nullptr;
	}
	virtual EPropertyType GetPropertyType() const override
	{
		return EPT_StandaloneStructure;
	}

	virtual void Disconnect() override
	{
		ClearCachedReadAddresses(true);
		DestroyTree();
		StructProvider = nullptr;
	}

protected:

	virtual EPropertyDataValidationResult EnsureDataIsValid() override;

	/** FPropertyNode interface */
	virtual void InitChildNodes() override;

	virtual uint8* GetValueBaseAddress(uint8* Base, bool bIsSparseData, bool bIsStruct) const override;

	virtual bool GetQualifiedName(FString& PathPlusIndex, const bool bWithArrayIndex, const FPropertyNode* StopParent = nullptr, bool bIgnoreCategories = false) const override
	{
		bool bAddedAnything = false;
		const TSharedPtr<FPropertyNode> ParentNode = ParentNodeWeakPtr.Pin();
		if (ParentNode && StopParent != ParentNode.Get())
		{
			bAddedAnything = ParentNode->GetQualifiedName(PathPlusIndex, bWithArrayIndex, StopParent, bIgnoreCategories);
		}

		if (bAddedAnything)
		{
			PathPlusIndex += TEXT(".");
		}

		PathPlusIndex += TEXT("Struct");
		bAddedAnything = true;

		return bAddedAnything;
	}

private:
	TSharedPtr<IStructureDataProvider> StructProvider;
};

} // namespace soda

