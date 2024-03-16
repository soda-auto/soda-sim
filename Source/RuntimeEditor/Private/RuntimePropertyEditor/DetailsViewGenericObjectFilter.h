// Copyright Epic Games, Inc. All Rights Reserved.
// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "RuntimePropertyEditor/DetailsViewObjectFilter.h"

namespace soda
{

/**
 * The default object set is just a pass through to the details panel.  
 */
class FDetailsViewDefaultObjectFilter : public FDetailsViewObjectFilter
{
public:
	FDetailsViewDefaultObjectFilter(bool bInAllowMultipleRoots);

	/**
	 * FDetailsViewObjectFilter interface
	 */
	virtual TArray<FDetailsViewObjectRoot> FilterObjects(const TArray<UObject*>& SourceObjects);

private:
	bool bAllowMultipleRoots;
};

} // namespace soda