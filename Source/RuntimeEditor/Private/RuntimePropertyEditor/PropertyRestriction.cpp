// Copyright Epic Games, Inc. All Rights Reserved.
// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "RuntimePropertyEditor/PropertyRestriction.h"

namespace soda
{

bool FPropertyRestriction::IsValueHidden(const FString& InValue) const
{
	for (const FString& Value : HiddenValues)
	{
		if (InValue == Value)
		{
			return true;
		}
	}
	return false;
}

bool FPropertyRestriction::IsValueDisabled(const FString& InValue)const
{
	for (const FString& Value : DisabledValues)
	{
		if (InValue == Value)
		{
			return true;
		}
	}
	return false;
}

void FPropertyRestriction::AddHiddenValue(FString InValue)
{
	HiddenValues.Add(MoveTemp(InValue));
}

void FPropertyRestriction::AddDisabledValue(FString InValue)
{
	DisabledValues.Add(MoveTemp(InValue));
}

void FPropertyRestriction::RemoveHiddenValue(FString Value)
{
	HiddenValues.Remove(Value);
}

void FPropertyRestriction::RemoveDisabledValue(FString Value)
{
	DisabledValues.Remove(Value);
}

void FPropertyRestriction::RemoveAll()
{
	HiddenValues.Empty();
	DisabledValues.Empty();
}

} // namespace soda
