// Copyright Epic Games, Inc. All Rights Reserved.
// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "RuntimePropertyEditor/UserInterface/PropertyEditor/SPropertyEditorDateTime.h"
#include "Widgets/Input/SEditableTextBox.h"

extern UScriptStruct* Z_Construct_UScriptStruct_FDateTime();	// It'd be really nice if StaticStruct() worked on types declared in Object.h

namespace soda
{

void SPropertyEditorDateTime::Construct( const FArguments& InArgs, const TSharedRef< class FPropertyEditor >& InPropertyEditor )
{
	PropertyEditor = InPropertyEditor;

	ChildSlot
	[
		SAssignNew( PrimaryWidget, SEditableTextBox )
		.Text( InPropertyEditor, &FPropertyEditor::GetValueAsText )
		.Font( InArgs._Font )
		.SelectAllTextWhenFocused(true)
		.ClearKeyboardFocusOnCommit(false)
		.OnTextCommitted(this, &SPropertyEditorDateTime::HandleTextCommitted)
		.SelectAllTextOnCommit(true)
	];

	if( InPropertyEditor->PropertyIsA( FObjectPropertyBase::StaticClass() ) )
	{
		// Object properties should display their entire text in a tooltip
		PrimaryWidget->SetToolTipText( TAttribute<FText>( InPropertyEditor, &FPropertyEditor::GetValueAsText ) );
	}

	SetEnabled(TAttribute<bool>(this, &SPropertyEditorDateTime::CanEdit));
}


bool SPropertyEditorDateTime::Supports( const TSharedRef< FPropertyEditor >& InPropertyEditor )
{
	const TSharedRef< FPropertyNode > PropertyNode = InPropertyEditor->GetPropertyNode();
	const FProperty* Property = InPropertyEditor->GetProperty();

	if (Property->IsA(FStructProperty::StaticClass()))
	{
		const FStructProperty* StructProp = CastField<const FStructProperty>(Property);
		if (Z_Construct_UScriptStruct_FDateTime() == StructProp->Struct)
		{
			return true;
		}
	}

	return false;
}


void SPropertyEditorDateTime::HandleTextCommitted( const FText& NewText, ETextCommit::Type /*CommitInfo*/ )
{
	const TSharedRef<FPropertyNode> PropertyNode = PropertyEditor->GetPropertyNode();
	const TSharedRef<IPropertyHandle> PropertyHandle = PropertyEditor->GetPropertyHandle();

	PropertyHandle->SetValueFromFormattedString(NewText.ToString());
}

/** @return True if the property can be edited */
bool SPropertyEditorDateTime::CanEdit() const
{
	return PropertyEditor.IsValid() ? !PropertyEditor->IsEditConst() : true;
}

} // namespace soda