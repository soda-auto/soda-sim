// Copyright Epic Games, Inc. All Rights Reserved.
// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "RuntimePropertyEditor/IPropertyGenerationUtilities.h"

namespace soda
{

/** Property generation utilities for widgets derived from SDetailsViewBase. */
class FDetailsViewPropertyGenerationUtilities : public IPropertyGenerationUtilities
{
public:
	FDetailsViewPropertyGenerationUtilities(SDetailsViewBase& InDetailsView)
		: DetailsView(&InDetailsView)
	{
	}

	virtual const FCustomPropertyTypeLayoutMap& GetInstancedPropertyTypeLayoutMap() const override
	{
		return DetailsView->GetCustomPropertyTypeLayoutMap();
	}

	virtual void RebuildTreeNodes() override
	{
		DetailsView->RerunCurrentFilter();
	}

private:
	SDetailsViewBase * DetailsView;
};

} // namespace soda