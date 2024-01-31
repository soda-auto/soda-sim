// Copyright Epic Games, Inc. All Rights Reserved.
// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "RuntimePropertyEditor/UserInterface/PropertyEditor/SPropertyEditorText.h"
#include "UObject/TextProperty.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "RuntimeMetaData.h"

#define LOCTEXT_NAMESPACE "PropertyEditor"

namespace soda
{

void SPropertyEditorText::Construct( const FArguments& InArgs, const TSharedRef< class FPropertyEditor >& InPropertyEditor )
{
	PropertyEditor = InPropertyEditor;

	bIsFNameProperty = InPropertyEditor->PropertyIsA(FNameProperty::StaticClass());
	bIsMultiLine = FRuntimeMetaData::GetBoolMetaData(InPropertyEditor->GetPropertyHandle()->GetMetaDataProperty(), "MultiLine");

	const bool bIsPassword = FRuntimeMetaData::GetBoolMetaData(InPropertyEditor->GetPropertyHandle()->GetMetaDataProperty(), "PasswordField");
	
	TSharedPtr<SHorizontalBox> HorizontalBox;
	if(bIsMultiLine)
	{
		ChildSlot
		[
			SAssignNew(HorizontalBox, SHorizontalBox)
			+SHorizontalBox::Slot()
			.FillWidth(1.0f)
			[
				SAssignNew(MultiLineWidget, SMultiLineEditableTextBox)
				.Text(InPropertyEditor, &FPropertyEditor::GetValueAsText)
				.Font(InArgs._Font)
				.SelectAllTextWhenFocused(false)
				.ClearKeyboardFocusOnCommit(false)
				.OnTextCommitted(this, &SPropertyEditorText::OnTextCommitted)
				.OnTextChanged(this, &SPropertyEditorText::OnMultiLineTextChanged)
				.SelectAllTextOnCommit(false)
				.IsReadOnly(this, &SPropertyEditorText::IsReadOnly)
				.AutoWrapText(true)
				.ModiferKeyForNewLine(EModifierKey::Shift)
				//.IsPassword( bIsPassword )
			]
		];

		PrimaryWidget = MultiLineWidget;
	}
	else
	{
		ChildSlot
		[
			SAssignNew(HorizontalBox, SHorizontalBox)
			+SHorizontalBox::Slot()
			.FillWidth(1.0f)
			[
				SAssignNew( SingleLineWidget, SEditableTextBox )
				.Text( InPropertyEditor, &FPropertyEditor::GetValueAsText )
				.Font( InArgs._Font )
				.SelectAllTextWhenFocused( true )
				.ClearKeyboardFocusOnCommit(false)
				.OnTextCommitted( this, &SPropertyEditorText::OnTextCommitted )
				.OnTextChanged( this, &SPropertyEditorText::OnSingleLineTextChanged )
				.SelectAllTextOnCommit( true )
				.IsReadOnly(this, &SPropertyEditorText::IsReadOnly)
				.IsPassword( bIsPassword )
			]
		];

		PrimaryWidget = SingleLineWidget;
	}

	if (bIsPassword)
	{
		// Passwords should be obfuscated rather than reveal the property value in the tooltip
		PrimaryWidget->SetToolTipText(LOCTEXT("PasswordToolTip", "<hidden>"));
	}
	else if (InPropertyEditor->PropertyIsA(FObjectPropertyBase::StaticClass()))
	{
		// Object properties should display their entire text in a tooltip
		PrimaryWidget->SetToolTipText(TAttribute<FText>(InPropertyEditor, &FPropertyEditor::GetValueAsText));
	}
}

void SPropertyEditorText::GetDesiredWidth( float& OutMinDesiredWidth, float& OutMaxDesiredWidth )
{
	if(bIsMultiLine)
	{
		OutMinDesiredWidth = 250.0f;
	}
	else
	{
		OutMinDesiredWidth = 125.0f;
	}
	
	OutMaxDesiredWidth = 600.0f;
}

bool SPropertyEditorText::Supports( const TSharedRef< FPropertyEditor >& InPropertyEditor )
{
	const TSharedRef< FPropertyNode > PropertyNode = InPropertyEditor->GetPropertyNode();
	const FProperty* Property = InPropertyEditor->GetProperty();

	if(	!PropertyNode->HasNodeFlags(EPropertyNodeFlags::EditInlineNew)
		&&	( (Property->IsA(FNameProperty::StaticClass()) && Property->GetFName() != NAME_InitialState)
		||	Property->IsA(FStrProperty::StaticClass())
		||	Property->IsA(FTextProperty::StaticClass())
		||	(Property->IsA(FObjectPropertyBase::StaticClass()) && !Property->HasAnyPropertyFlags(CPF_InstancedReference))
		) )
	{
		return true;
	}

	return false;
}

void SPropertyEditorText::OnTextCommitted( const FText& NewText, ETextCommit::Type /*CommitInfo*/ )
{
	const TSharedRef< FPropertyNode > PropertyNode = PropertyEditor->GetPropertyNode();
	const TSharedRef< IPropertyHandle > PropertyHandle = PropertyEditor->GetPropertyHandle();

	FText CurrentText;
	if( (PropertyHandle->GetValueAsFormattedText( CurrentText ) != FPropertyAccess::MultipleValues || NewText.ToString() != FPropertyEditor::MultipleValuesDisplayName)
		&& !NewText.ToString().Equals(CurrentText.ToString(), ESearchCase::CaseSensitive))
	{
		PropertyHandle->SetValueFromFormattedString( NewText.ToString() );
	}
}

static FText ValidateNameLength( const FText& Text )
{
	if( Text.ToString().Len() >= NAME_SIZE )
	{
		static FText ErrorString = FText::Format( LOCTEXT("NamePropertySizeTooLongError", "Name properties may only be a maximum of {0} characters"), FText::AsNumber( NAME_SIZE ) );
		return ErrorString;
	}

	return FText::GetEmpty();
}

void SPropertyEditorText::OnMultiLineTextChanged( const FText& NewText )
{
	if( bIsFNameProperty )
	{
		FText ErrorMessage = ValidateNameLength( NewText );
		MultiLineWidget->SetError( ErrorMessage );
	}
}

void SPropertyEditorText::OnSingleLineTextChanged( const FText& NewText )
{
	if( bIsFNameProperty )
	{
		FText ErrorMessage = ValidateNameLength( NewText );
		SingleLineWidget->SetError( ErrorMessage );
	}
}

bool SPropertyEditorText::SupportsKeyboardFocus() const
{
	return PrimaryWidget.IsValid() && PrimaryWidget->SupportsKeyboardFocus() && CanEdit();
}

FReply SPropertyEditorText::OnFocusReceived( const FGeometry& MyGeometry, const FFocusEvent& InFocusEvent )
{
	// Forward keyboard focus to our editable text widget
	return FReply::Handled().SetUserFocus(PrimaryWidget.ToSharedRef(), InFocusEvent.GetCause());
}

bool SPropertyEditorText::CanEdit() const
{
	return PropertyEditor.IsValid() ? !PropertyEditor->IsEditConst() : true;
}

bool SPropertyEditorText::IsReadOnly() const
{
	return !CanEdit();
}

} // namespace soda

#undef LOCTEXT_NAMESPACE
