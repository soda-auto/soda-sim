// Copyright Epic Games, Inc. All Rights Reserved.
// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RuntimePropertyEditor/IPropertyUtilities.h"

namespace soda
{

class IPropertyTableUtilities : public IPropertyUtilities
{
public:

	virtual void RemoveColumn( const TSharedRef< class IPropertyTableColumn >& Column ) = 0;

};

} // namespace soda
