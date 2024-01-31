// Copyright Epic Games, Inc. All Rights Reserved.
// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "Containers/Array.h"
#include "Templates/SharedPointer.h"
#include "UObject/StructOnScope.h"

namespace soda
{

//-----------------------------------------------------------------------------
//	IStructureDataProvider - Used to provide struct data for a property node.
//
// The IsPropertyIndirection() and GetValueBaseAddress() allow a struct provider to expose data to be edited that
// is not supported by the property system. When IsPropertyIndirection() returns true, GetValueBaseAddress() is called
// relative to the parent property node to retrieve pointer to the data to be edited.
// 
// In the following example, a struct provider is added via ParentProperty property handle. In this case "ParentProperty"
// is the parent property node, and the value passed into GetValueBaseAddress() of the provider is the value of the parent property.
//
//		TSharedPtr<IPropertyHandle> ParentProperty = ...; // E.g. handle to structure being customized
//		...
//		TSharedRef<FMyStructProvider> NewStructProvider = MakeShared<FMyStructProvider>(ParentProperty);
//		TArray<TSharedPtr<IPropertyHandle>> ChildProperties = ParentProperty->AddChildStructure(NewStructProvider);
// 
// The GetValueBaseAddress() may get called on foreign objects too (e.g. object templates), and if property indirection is used
// it should not rely on other values cached in the provider.
// 
//		class FMyStructProvider : public IStructureDataProvider
//		{
//		public:
//			...
//
//			virtual bool IsPropertyIndirection() const override { return true; }
//
//			virtual uint8* GetValueBaseAddress(uint8* ParentValueAddress, const UStruct* ExpectedType) const override
//			{
//				if (ParentValueAddress)
//				{
//					// "ParentValueAddress" is pointer to a value of the type defined in "ParentProperty".
//					FMyStruct& Value = *reinterpret_cast<FMyStruct*>(ParentValueAddress);
//					// The return value should match the ExpectedType. The expected type should be the same as the GetBaseStructure() returned by this provider.
//					return (Value.GetScriptStruct() == ExpectedType) ? Value.GetMemory() : nullptr;
//				}
//				return nullptr;
//			}
//		};
// 
// If IsPropertyIndirection() is false, there structures are expected to be standalone, and GetInstances() is used instead to retried the instance values.
// When indirection is not used and the struct provider is part of an another instance, the provider should return only one instance, since there is no to associate the provided value
// with an foreign object.
// 
// Multiple instances are supported when the provider is the root node (e.g. on Structure details view).
// 
//-----------------------------------------------------------------------------
class IStructureDataProvider
{
public:
	virtual ~IStructureDataProvider() {}

	/** @return true if we have any data instances. */
	virtual bool IsValid() const = 0;
	
	/** @return the most common struct of the instance data. This struct will be used to build the UI for the instances. */
	virtual const UStruct* GetBaseStructure() const = 0;

	/** @return instances to edit. Each provided struct should be compatible with base struct. */
	virtual void GetInstances(TArray<TSharedPtr<FStructOnScope>>& OutInstances) const = 0;

	/** @return true, if the provider supports handling indirections using GetValueBaseAddress(). */
	virtual bool IsPropertyIndirection() const { return false; }

	/**
	 * Returns base address of provided struct based on parent property nodes value.
	 * @param ParentValueAddress Value of the parent 
	 * @param ExpectedType the type of struct that is expected to be returned (generally the same type as returned by GetBaseStructure())
	 * @return value base address based on parent base address.
	 */
	virtual uint8* GetValueBaseAddress(uint8* ParentValueAddress, const UStruct* ExpectedType) const { return ParentValueAddress; }
};


//-----------------------------------------------------------------------------
//	FStructOnScopeStructureDataProvider - Implementation of standalone struct that holds one value.
//-----------------------------------------------------------------------------
class FStructOnScopeStructureDataProvider : public IStructureDataProvider
{
public:
	FStructOnScopeStructureDataProvider() = default;
	FStructOnScopeStructureDataProvider(const TSharedPtr<FStructOnScope>& InStructData)
		: StructData(InStructData)
	{
	}
	
	void SetStructData(const TSharedPtr<FStructOnScope>& InStructData)
	{
		StructData = InStructData;
	}
	
	virtual bool IsValid() const override
	{
		return StructData.IsValid() && StructData->IsValid();
	};
	
	virtual const UStruct* GetBaseStructure() const override
	{
		return StructData.IsValid() ? StructData->GetStruct() : nullptr;
	}
	
	virtual void GetInstances(TArray<TSharedPtr<FStructOnScope>>& OutInstances) const override
	{
		OutInstances.Add(StructData);
	}

protected:
	TSharedPtr<FStructOnScope> StructData;
};

} // namespace soda