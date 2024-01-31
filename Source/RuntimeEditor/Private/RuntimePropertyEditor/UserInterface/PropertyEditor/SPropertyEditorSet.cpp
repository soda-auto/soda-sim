// Copyright Epic Games, Inc. All Rights Reserved.
// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "RuntimePropertyEditor/UserInterface/PropertyEditor/SPropertyEditorSet.h"
#include "UObject/UnrealType.h"
#include "RuntimePropertyEditor/PropertyNode.h"
#include "Widgets/Text/STextBlock.h"
#include "RuntimePropertyEditor/Presentation/PropertyEditor/PropertyEditor.h"

#define LOCTEXT_NAMESPACE "PropertyEditor"

namespace soda
{

void SPropertyEditorSet::Construct( const FArguments& InArgs, const TSharedRef< class FPropertyEditor >& InPropertyEditor )
{
	PropertyEditor = InPropertyEditor;

	TAttribute<FText> TextAttr(this, &SPropertyEditorSet::GetSetTextValue);

	ChildSlot
	.Padding(0.0f, 0.0f, 2.0f, 0.0f)
	[
		SNew(STextBlock)
		.Text(TextAttr)
		.Font(InArgs._Font)
	];

	SetToolTipText(GetSetTooltipText());

	SetEnabled(TAttribute<bool>(this, &SPropertyEditorSet::CanEdit));
}

bool SPropertyEditorSet::Supports( const TSharedRef< FPropertyEditor >& InPropertyEditor)
{
	const TSharedRef< FPropertyNode > PropertyNode = InPropertyEditor->GetPropertyNode();
	const FProperty* Property = InPropertyEditor->GetProperty();

	if (!PropertyNode->HasNodeFlags(EPropertyNodeFlags::EditInlineNew)
		&& Property->IsA<FSetProperty>())
	{
		return true;
	}

	return false;
}

void SPropertyEditorSet::GetDesiredWidth( float& OutMinDesiredWidth, float& OutMaxDesiredWidth )
{
	OutMinDesiredWidth = 170.0f;
	OutMaxDesiredWidth = 170.0f;
}

FText SPropertyEditorSet::GetSetTextValue() const
{
	return FText::Format(LOCTEXT("NumSetItemsFmt", "{0} Set elements"), FText::AsNumber(PropertyEditor->GetPropertyNode()->GetNumChildNodes()));
}

FText SPropertyEditorSet::GetSetTooltipText() const
{
	return LOCTEXT("RichSetTooltipText", "Sets are unordered containers. Each element in a set must be unique.");
}

bool SPropertyEditorSet::CanEdit() const
{
	return PropertyEditor.IsValid() ? !PropertyEditor->IsEditConst() : true;
}

} // namespace soda

#undef LOCTEXT_NAMESPACE
