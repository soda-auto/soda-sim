// Copyright Epic Games, Inc. All Rights Reserved.
// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

namespace soda
{

/**
 * An object root is a collection of UObjects that represent a top level set of properties in a details panel
 * When there are multiple objects in the root, the common base class for all those objects are found and the properties on that common base class are displayed
 * When a user edits one of those properties, it will propagate the result to all objects in the root.  
 * If multiple differing values are found on a single property, the details panel UI displays "multiple values" 
 */
struct FDetailsViewObjectRoot
{
	FDetailsViewObjectRoot()
	{}

	FDetailsViewObjectRoot(UObject* InObject)
	{
		Objects.Add(InObject);
	}

	FDetailsViewObjectRoot(const TArray<UObject*>& InObjects)
	{
		Objects.Reserve(InObjects.Num());
		for (UObject* Object : InObjects)
		{
			Objects.Add(Object);
		}
	}

	TArray<UObject*> Objects;
};

/**
 * An object filter determines the root objects that should be displayed from a set of given source objects passed to the details panel
 */
class FDetailsViewObjectFilter
{
public:
	virtual ~FDetailsViewObjectFilter() {}

	virtual TArray<FDetailsViewObjectRoot> FilterObjects(const TArray<UObject*>& SourceObjects) = 0;

};

} // namespace soda
