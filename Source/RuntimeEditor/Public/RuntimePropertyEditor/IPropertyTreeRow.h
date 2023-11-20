// Copyright Epic Games, Inc. All Rights Reserved.
// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

namespace soda
{

class FPropertyPath;

class IPropertyTreeRow
{
public: 

	virtual TSharedPtr< FPropertyPath > GetPropertyPath() const = 0;

	virtual bool IsCursorHovering() const = 0;
};

} // namespace soda
