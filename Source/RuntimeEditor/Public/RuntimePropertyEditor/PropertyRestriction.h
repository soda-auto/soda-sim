// Copyright Epic Games, Inc. All Rights Reserved.
// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

namespace soda
{

class RUNTIMEEDITOR_API FPropertyRestriction
{
public:

	FPropertyRestriction(const FText& InReason)
		: Reason(InReason)
	{
	}

	const FText& GetReason() const { return Reason; }

	bool IsValueHidden(const FString& Value) const;
	bool IsValueDisabled(const FString& Value) const;
	void AddHiddenValue(FString Value);
	void AddDisabledValue(FString Value);

	void RemoveHiddenValue(FString Value);
	void RemoveDisabledValue(FString Value);
	void RemoveAll();

	TArray<FString>::TConstIterator GetHiddenValuesIterator() const 
	{
		return HiddenValues.CreateConstIterator();
	}

	TArray<FString>::TConstIterator GetDisabledValuesIterator() const
	{
		return DisabledValues.CreateConstIterator();
	}

private:

	TArray<FString> HiddenValues;
	TArray<FString> DisabledValues;
	FText Reason;
};

}
