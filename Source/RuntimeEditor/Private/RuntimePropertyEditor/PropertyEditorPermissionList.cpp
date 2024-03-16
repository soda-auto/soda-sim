// Copyright Epic Games, Inc. All Rights Reserved.
// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "RuntimePropertyEditor/PropertyEditorPermissionList.h"

//#include "Editor.h"
#include "Misc/CoreDelegates.h"
#include "UObject/UnrealType.h"

namespace soda
{
namespace
{
	const FName PropertyEditorPermissionListOwner = "PropertyEditorPermissionList";
}

FPropertyEditorPermissionList::FPropertyEditorPermissionList()
{
	/*
	if (GEditor)
	{
		RegisterOnBlueprintCompiled();
	}
	else
	*/
	{
		FCoreDelegates::OnPostEngineInit.AddRaw(this, &FPropertyEditorPermissionList::RegisterOnBlueprintCompiled);
	}
}

void FPropertyEditorPermissionList::RegisterOnBlueprintCompiled()
{
	/*
	if (ensure(GEditor))
	{
		GEditor->OnBlueprintCompiled().AddRaw(this, &FPropertyEditorPermissionList::ClearCache);
	}
	*/
}

FPropertyEditorPermissionList::~FPropertyEditorPermissionList()
{
	/*
	if (GEditor)
	{
		GEditor->OnBlueprintCompiled().RemoveAll(this);
	}
	*/
}

void FPropertyEditorPermissionList::AddPermissionList(TSoftObjectPtr<UStruct> Struct, const FNamePermissionList& PermissionList, EPropertyEditorPermissionListRules Rules)
{
	FPropertyEditorPermissionListEntry& Entry = RawPropertyEditorPermissionList.FindOrAdd(Struct);
	Entry.PermissionList.Append(PermissionList);
	Entry.Rules = Rules;
	// The cache isn't too expensive to recompute, so it is cleared
	// and lazily repopulated any time the raw PermissionList changes.
	ClearCache();
	PermissionListUpdatedDelegate.Broadcast(Struct, NAME_None);
}

void FPropertyEditorPermissionList::RemovePermissionList(TSoftObjectPtr<UStruct> Struct)
{
	if (RawPropertyEditorPermissionList.Remove(Struct) > 0)
	{
		ClearCache();
		PermissionListUpdatedDelegate.Broadcast(Struct, NAME_None);
	}
}

void FPropertyEditorPermissionList::ClearPermissionList()
{
	TArray<TSoftObjectPtr<UStruct>> Keys;
	RawPropertyEditorPermissionList.Reset();
	PermissionListUpdatedDelegate.Broadcast(nullptr, NAME_None);
}

void FPropertyEditorPermissionList::AddToAllowList(TSoftObjectPtr<UStruct> Struct, const FName PropertyName, const FName Owner)
{
	FPropertyEditorPermissionListEntry& Entry = RawPropertyEditorPermissionList.FindOrAdd(Struct);
	if (Entry.PermissionList.AddAllowListItem(Owner, PropertyName))
	{
		ClearCache();
		PermissionListUpdatedDelegate.Broadcast(Struct, Owner);
	}
}

void FPropertyEditorPermissionList::RemoveFromAllowList(TSoftObjectPtr<UStruct> Struct, const FName PropertyName, const FName Owner)
{
	FPropertyEditorPermissionListEntry& Entry = RawPropertyEditorPermissionList.FindOrAdd(Struct);
	if (Entry.PermissionList.RemoveAllowListItem(Owner, PropertyName))
	{
		ClearCache();
		PermissionListUpdatedDelegate.Broadcast(Struct, Owner);
	}
}

void FPropertyEditorPermissionList::AddToDenyList(TSoftObjectPtr<UStruct> Struct, const FName PropertyName, const FName Owner)
{
	FPropertyEditorPermissionListEntry& Entry = RawPropertyEditorPermissionList.FindOrAdd(Struct);
	if (Entry.PermissionList.AddDenyListItem(Owner, PropertyName))
	{
		ClearCache();
		PermissionListUpdatedDelegate.Broadcast(Struct, Owner);
	}
}

void FPropertyEditorPermissionList::RemoveFromDenyList(TSoftObjectPtr<UStruct> Struct, const FName PropertyName, const FName Owner)
{
	FPropertyEditorPermissionListEntry& Entry = RawPropertyEditorPermissionList.FindOrAdd(Struct);
	if (Entry.PermissionList.RemoveDenyListItem(Owner, PropertyName))
	{
		ClearCache();
		PermissionListUpdatedDelegate.Broadcast(Struct, Owner);
	}
}

void FPropertyEditorPermissionList::SetEnabled(bool bEnable)
{
	bEnablePropertyEditorPermissionList = bEnable;
	PermissionListEnabledDelegate.Broadcast();
}

void FPropertyEditorPermissionList::ClearCache()
{
	CachedPropertyEditorPermissionList.Reset();
}

bool FPropertyEditorPermissionList::DoesPropertyPassFilter(const UStruct* ObjectStruct, FName PropertyName) const
{
	if (bEnablePropertyEditorPermissionList && ObjectStruct)
	{
		return GetCachedPermissionListForStruct(ObjectStruct).PassesFilter(PropertyName);
	}
	return true;
}

const FNamePermissionList& FPropertyEditorPermissionList::GetCachedPermissionListForStruct(const UStruct* Struct) const
{
	const FNamePermissionList* CachedPermissionList = CachedPropertyEditorPermissionList.Find(Struct);
	if (CachedPermissionList)
	{
		return *CachedPermissionList;
	}

	// Default value doesn't matter since it's a no-op until the first PermissionList is encountered, at which
	// point the rules will re-assign the value.
	bool bShouldPermissionListAllProperties = true;
	return GetCachedPermissionListForStructHelper(Struct, bShouldPermissionListAllProperties);
}

const FNamePermissionList& FPropertyEditorPermissionList::GetCachedPermissionListForStructHelper(const UStruct* Struct, bool& bInOutShouldAllowListAllProperties) const
{
	check(Struct);

	const FPropertyEditorPermissionListEntry* Entry = RawPropertyEditorPermissionList.Find(Struct);
	bool bIsThisAllowListEmpty = Entry ? Entry->PermissionList.GetAllowList().Num() == 0 : true;

	// Normally this case would be caught in GetCachedPermissionListForStruct, but when being called recursively from a subclass
	// we still need to update bInOutShouldAllowListAllProperties so that new PermissionLists cache properly.
	FNamePermissionList* PermissionList = CachedPropertyEditorPermissionList.Find(Struct);
	if (PermissionList)
	{
		// Same check as below, but it specifically has to come after the recursive call so we have to duplicate the code here
		if (Entry && Entry->Rules == EPropertyEditorPermissionListRules::AllowListAllProperties)
		{
			bInOutShouldAllowListAllProperties = true;
		}
	}
	else
	{
		FNamePermissionList NewPermissionList;

		UStruct* SuperStruct = Struct->GetSuperStruct();
		// Recursively fill the cache for all parent structs
		if (SuperStruct)
		{
			NewPermissionList.Append(GetCachedPermissionListForStructHelper(SuperStruct, bInOutShouldAllowListAllProperties));
		}

		// Append this struct's PermissionList on top of the parent's PermissionList
		if (Entry)
		{
			NewPermissionList.Append(Entry->PermissionList);

			if (Entry->Rules == EPropertyEditorPermissionListRules::AllowListAllProperties)
			{
				bInOutShouldAllowListAllProperties = true;
			}
		}

		// PermissionList all properties if the flag is set, the parent Struct has a PermissionList, and this Struct has no PermissionList
		// If the parent Struct's PermissionList is empty then that already implies all properties are visible
		// If this Struct has a PermissionList, the manually-specified list always overrides the ShouldPermissionListAllProperties rule
		if (bInOutShouldAllowListAllProperties && NewPermissionList.GetAllowList().Num() > 0 && bIsThisAllowListEmpty)
		{
			for (TFieldIterator<FProperty> Property(Struct, EFieldIteratorFlags::ExcludeSuper, EFieldIteratorFlags::ExcludeDeprecated); Property; ++Property)
			{
				NewPermissionList.AddAllowListItem(PropertyEditorPermissionListOwner, Property->GetFName());
			}
		}

		PermissionList = &CachedPropertyEditorPermissionList.Add(Struct, NewPermissionList);
	}

	// If this Struct has no PermissionList, then the ShouldPermissionListAllProperties rule just forwards its current value on to the next subclass.
	// This causes an issue in the case where a Struct should have no PermissionListed properties but wants to PermissionList all subclass properties.
	// In this case, simply add a dummy entry to the Struct's PermissionList that (likely) won't ever collide with a real property name
	if (!bIsThisAllowListEmpty)
	{
		bInOutShouldAllowListAllProperties = Entry->Rules == EPropertyEditorPermissionListRules::AllowListAllSubclassProperties;
	}

	return *PermissionList;
}

bool FPropertyEditorPermissionList::IsSpecificPropertyAllowListed(const UStruct* ObjectStruct, FName PropertyName) const
{
	const FPropertyEditorPermissionListEntry* Entry = RawPropertyEditorPermissionList.Find(ObjectStruct);
	if (Entry)
	{
		return Entry->PermissionList.GetAllowList().Contains(PropertyName);
	}
	return false;
}

bool FPropertyEditorPermissionList::IsSpecificPropertyDenyListed(const UStruct* ObjectStruct, FName PropertyName) const
{
	const FPropertyEditorPermissionListEntry* Entry = RawPropertyEditorPermissionList.Find(ObjectStruct);
	if (Entry)
	{
		return Entry->PermissionList.GetDenyList().Contains(PropertyName);
	}
	return false;
}

} // namespace soda