// Copyright Epic Games, Inc. All Rights Reserved.
// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

namespace soda
{
/** Defines internal utilities for generating property layouts. */
class IPropertyGenerationUtilities
{
public:
	virtual ~IPropertyGenerationUtilities() { }

	/** Gets the instance type customization map for the details implementation providing this utilities object. */
	virtual const FCustomPropertyTypeLayoutMap& GetInstancedPropertyTypeLayoutMap() const = 0;

	/** Rebuilds the details tree nodes for the details implementation providing this utilities object. */
	virtual void RebuildTreeNodes() = 0;
};

} // namespace soda