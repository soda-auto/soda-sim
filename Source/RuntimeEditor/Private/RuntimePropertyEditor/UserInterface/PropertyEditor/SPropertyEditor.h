// Copyright Epic Games, Inc. All Rights Reserved.
// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/UnrealType.h"
#include "Fonts/SlateFontInfo.h"
#include "SodaStyleSet.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"
#include "RuntimePropertyEditor/Presentation/PropertyEditor/PropertyEditor.h"
#include "RuntimePropertyEditor/UserInterface/PropertyEditor/PropertyEditorConstants.h"
#include "Widgets/Input/SEditableTextBox.h"

#define LOCTEXT_NAMESPACE "PropertyEditor"

namespace soda
{

class SPropertyEditor : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS( SPropertyEditor )
		: _Font( FSodaStyle::GetFontStyle( PropertyEditorConstants::PropertyFontStyle ) ) 
	{}
	SLATE_ATTRIBUTE( FSlateFontInfo, Font )
		SLATE_END_ARGS()

	void Construct( const FArguments& InArgs, const TSharedRef< class FPropertyEditor >& InPropertyEditor )
	{
		PropertyEditor = InPropertyEditor;

		if( ShouldShowValue( InPropertyEditor ) )
		{
			ChildSlot
			[
				// Make a read only text box so that copy still works
				SNew( SEditableTextBox )
				.Text( InPropertyEditor, &FPropertyEditor::GetValueAsText )
				.ToolTipText( InPropertyEditor, &FPropertyEditor::GetValueAsText )
				.Font( InArgs._Font )
				.IsReadOnly( true )
			];
		}
	}

	void GetDesiredWidth( float& OutMinDesiredWidth, float& OutMaxDesiredWidth )
	{
		OutMinDesiredWidth = 125.0f;
		OutMaxDesiredWidth = 0.0f;

		if( PropertyEditor.IsValid() )
		{
			const FProperty* Property = PropertyEditor->GetProperty();
			if( Property && Property->IsA<FStructProperty>() )
			{
				// Struct headers with nothing in them have no min width
				OutMinDesiredWidth = 0;
				OutMaxDesiredWidth = 130.0f;
			}
			
		}
	}
private:
	bool ShouldShowValue( const TSharedRef< class FPropertyEditor >& InPropertyEditor ) const 
	{
		return PropertyEditor->GetProperty() && !PropertyEditor->GetProperty()->IsA<FStructProperty>();
	}
private:
	TSharedPtr< class FPropertyEditor > PropertyEditor;
};

} // namespace soda

#undef LOCTEXT_NAMESPACE
