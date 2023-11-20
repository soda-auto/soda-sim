// Copyright Epic Games, Inc. All Rights Reserved.
// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SWidget.h"
#include "Layout/Margin.h"
#include "Widgets/SCompoundWidget.h"

namespace soda
{

/** Interface for the widget that wraps an editable text box for viewing the names of objects or editing the labels of actors */
class IObjectNameEditableTextBox : public SCompoundWidget
{
public:
	static TSharedRef<IObjectNameEditableTextBox> CreateObjectNameEditableTextBox(const TArray<TWeakObjectPtr<UObject>>& Objects);
};

}