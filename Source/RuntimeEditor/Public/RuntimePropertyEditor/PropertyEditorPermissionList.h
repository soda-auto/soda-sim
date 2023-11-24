// Copyright Epic Games, Inc. All Rights Reserved.
// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/SoftObjectPtr.h"
#include "UObject/WeakObjectPtr.h"
#include "Misc/NamePermissionList.h"

namespace soda
{

/** Struct, OwnerName */
DECLARE_MULTICAST_DELEGATE_TwoParams(FPermissionListUpdated, TSoftObjectPtr<UStruct>, FName);

/**
 * A hierarchical set of rules that can be used to PermissionList all properties of specific Structs without
 * having to manually add every single property in those Structs. These rules are applied in order from
 * the base Struct to the leaf Struct. UseExistingPermissionList has dual-functionality to alternatively
 * inherit the parent Struct's rule if no PermissionList is manually defined.
 * 
 * For example, if you have:
 * class A - (UseExistingPermissionList "MyProp")						PermissionList = "MyProp"
 * class B : public class A - (AllowListListAllProperties)				PermissionList = "MyProp","PropA1","PropA2"
 * class C : public class B - (UseExistingPermissionList "AnotherProp")	PermissionList = "MyProp","PropA1","PropA2","AnotherProp"
 * class D : public class B - (UseExistingPermissionList)				PermissionList = "MyProp","PropA1","PropA2","PropD1","PropD2"
 * Note that because class C manually defines a PermissionList, it does not inherit the AllowListAllProperties rule from class B, while
 * class D does not define a PermissionList, so it does inherit the rule, causing all of class D's properties to also get added to the AllowList.
 */
enum class EPropertyEditorPermissionListRules : uint8
{
	// If a PermissionList is manually defined for this struct, PermissionList those properties. Otherwise, use the parent Struct's rule.
	UseExistingPermissionList,
	// If no PermissionList is manually defined for this Struct, AllowList all properties from this Struct and its subclasses
	AllowListAllProperties,
	// If a PermissionList is manually defined for this Struct, AllowList all properties from this Struct's subclasses.
	// If this functionality is needed without any properties to AllowList, a fake property must be added to AllowList instead.
	AllowListAllSubclassProperties
};

struct FPropertyEditorPermissionListEntry
{
    FNamePermissionList PermissionList;
    EPropertyEditorPermissionListRules Rules = EPropertyEditorPermissionListRules::UseExistingPermissionList;
};

class RUNTIMEEDITOR_API FPropertyEditorPermissionList
{
public:
	static FPropertyEditorPermissionList& Get()
	{
		static FPropertyEditorPermissionList PermissionList;
		return PermissionList;
	}

	/** Add a set of rules for a specific base UStruct to determine which properties are visible in all details panels */
	void AddPermissionList(TSoftObjectPtr<UStruct> Struct, const FNamePermissionList& PermissionList, EPropertyEditorPermissionListRules Rules = EPropertyEditorPermissionListRules::UseExistingPermissionList);
	/** Remove a set of rules for a specific base UStruct to determine which properties are visible in all details panels */
	void RemovePermissionList(TSoftObjectPtr<UStruct> Struct);
	/** Remove all rules */
	void ClearPermissionList();

	/** Add a specific property to a UStruct's AllowList */
	void AddToAllowList(TSoftObjectPtr<UStruct> Struct, const FName PropertyName, const FName Owner = NAME_None);
	/** Remove a specific property from a UStruct's AllowList */
	void RemoveFromAllowList(TSoftObjectPtr<UStruct> Struct, const FName PropertyName, const FName Owner = NAME_None);
	/** Add a specific property to a UStruct's DenyList */
	void AddToDenyList(TSoftObjectPtr<UStruct> Struct, const FName PropertyName, const FName Owner = NAME_None);
	/** Remove a specific property from a UStruct's DenyList */
    void RemoveFromDenyList(TSoftObjectPtr<UStruct> Struct, const FName PropertyName, const FName Owner = NAME_None);

	/** When the PermissionList or DenyList for any struct was added to or removed from. */
    FPermissionListUpdated PermissionListUpdatedDelegate;
    
	/** When the entire PermissionList is enabled or disabled */
	FSimpleMulticastDelegate PermissionListEnabledDelegate;

	/** Controls whether DoesPropertyPassFilter always returns true or performs property-based filtering. */
	bool IsEnabled() const { return bEnablePropertyEditorPermissionList; }
	/** Turn on or off the property editor PermissionList. DoesPropertyPassFilter will always return true if disabled. */
	void SetEnabled(bool bEnable);

	/** Whether the Details View should show special menu entries to add/remove items in the PermissionList */
	bool ShouldShowMenuEntries() const { return bShouldShowMenuEntries;}
	/** Turn on or off menu entries to modify the PermissionList from a Details View */
	void SetShouldShowMenuEntries(bool bShow) { bShouldShowMenuEntries = bShow; }

	/**
	 * Checks if a property passes the PermissionList/DenyList filtering specified by PropertyEditorPermissionLists
	 * This should be relatively fast as it maintains a flattened cache of all inherited PermissionLists for every UStruct (which is generated lazily).
	 */
	bool DoesPropertyPassFilter(const UStruct* ObjectStruct, FName PropertyName) const;

	/** Check whether a property exists on the PermissionList for a specific Struct - this will return false if the property is AllowListed on a parent Struct */
	bool IsSpecificPropertyAllowListed(const UStruct* ObjectStruct, FName PropertyName) const;
	/** Check whether a property exists on the DenyList for a specific Struct - this will return false if the property is DenyListed on a parent Struct */
	bool IsSpecificPropertyDenyListed(const UStruct* ObjectStruct, FName PropertyName) const;

	/** Gets a read-only copy of the original, un-flattened PermissionList. */
	const TMap<TSoftObjectPtr<UStruct>, FPropertyEditorPermissionListEntry>& GetRawPermissionList() const { return RawPropertyEditorPermissionList; }

	/** Clear CachedPropertyEditorPermissionList to cause the PermissionListed property list to be regenerated next time it's queried */
	void ClearCache();

private:
	FPropertyEditorPermissionList();
	~FPropertyEditorPermissionList();

	void RegisterOnBlueprintCompiled();
	
	/** Whether DoesPropertyPassFilter should perform its PermissionList check or always return true */
	bool bEnablePropertyEditorPermissionList = false;
	/** Whether SDetailSingleItemRow should add menu items to add/remove properties to/from the PermissionList */
	bool bShouldShowMenuEntries = false;
	
	/** Stores assigned PermissionLists from AddPermissionList(), which are later flattened and stored in CachedPropertyEditorPermissionList. */
	TMap<TSoftObjectPtr<UStruct>, FPropertyEditorPermissionListEntry> RawPropertyEditorPermissionList;

	/** Lazily-constructed combined cache of both the flattened class PermissionList and struct PermissionList */
	mutable TMap<TWeakObjectPtr<const UStruct>, FNamePermissionList> CachedPropertyEditorPermissionList;

	/** Get or create the cached PermissionList for a specific UStruct */
	const FNamePermissionList& GetCachedPermissionListForStruct(const UStruct* Struct) const;
	const FNamePermissionList& GetCachedPermissionListForStructHelper(const UStruct* Struct, bool& bInOutShouldAllowListAllProperties) const;
};

} // namespace soda