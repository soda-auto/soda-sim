// Copyright Epic Games, Inc. All Rights Reserved.
// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "RuntimePropertyEditor/DetailsViewGenericObjectFilter.h"

namespace soda
{

FDetailsViewDefaultObjectFilter::FDetailsViewDefaultObjectFilter(bool bInAllowMultipleRoots)
	: bAllowMultipleRoots(bInAllowMultipleRoots)
{
}

TArray<FDetailsViewObjectRoot> FDetailsViewDefaultObjectFilter::FilterObjects(const TArray<UObject*>& SourceObjects)
{
	TArray<FDetailsViewObjectRoot> Roots;
	if (bAllowMultipleRoots)
	{
		Roots.Reserve(SourceObjects.Num());
		for (UObject* Object : SourceObjects)
		{
			Roots.Emplace(Object);
		}
	}
	else
	{
		Roots.Emplace(SourceObjects);
	}

	return Roots;
}

} // namespace soda