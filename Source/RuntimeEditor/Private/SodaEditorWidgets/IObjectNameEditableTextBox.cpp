// Copyright Epic Games, Inc. All Rights Reserved.
// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "SodaEditorWidgets/IObjectNameEditableTextBox.h"
#include "SodaEditorWidgets/SObjectNameEditableTextBox.h"
#include "Widgets/DeclarativeSyntaxSupport.h"

namespace soda
{

TSharedRef<IObjectNameEditableTextBox> IObjectNameEditableTextBox::CreateObjectNameEditableTextBox(const TArray<TWeakObjectPtr<UObject>>& Objects)
{
	TSharedRef<SObjectNameEditableTextBox> Widget = SNew(SObjectNameEditableTextBox).Objects(Objects);
	return Widget;
}
}
