// Copyright Epic Games, Inc. All Rights Reserved.
// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "RuntimePropertyEditor/UserInterface/PropertyEditor/SPropertyEditorCombo.h"
#include "RuntimeDocumentation/IDocumentation.h"

#include "RuntimePropertyEditor/PropertyEditorHelpers.h"
#include "RuntimePropertyEditor/UserInterface/PropertyEditor/SPropertyComboBox.h"

#include "RuntimeMetaData.h"
#include "RuntimeEditorUtils.h"

#define LOCTEXT_NAMESPACE "PropertyEditor"

namespace soda
{

static int32 FindEnumValueIndex(UEnum* Enum, FString const& ValueString)
{
	int32 Index = INDEX_NONE;
	for(int32 ValIndex = 0; ValIndex < Enum->NumEnums(); ++ValIndex)
	{
		FString const EnumName    = Enum->GetNameStringByIndex(ValIndex);
		FString const DisplayName = Enum->GetDisplayNameTextByIndex(ValIndex).ToString();

		if (DisplayName.Len() > 0)
		{
			if (DisplayName == ValueString)
			{
				Index = ValIndex;
				break;
			}
		}
		
		if (EnumName == ValueString)
		{
			Index = ValIndex;
			break;
		}
	}
	return Index;
}

void SPropertyEditorCombo::GetDesiredWidth( float& OutMinDesiredWidth, float& OutMaxDesiredWidth )
{
	OutMinDesiredWidth = 125.0f;
	OutMaxDesiredWidth = 400.0f;
}

bool SPropertyEditorCombo::Supports( const TSharedRef< class FPropertyEditor >& InPropertyEditor )
{
	const TSharedRef< FPropertyNode > PropertyNode = InPropertyEditor->GetPropertyNode();
	const FProperty* Property = InPropertyEditor->GetProperty();
	int32 ArrayIndex = PropertyNode->GetArrayIndex();

	if(	((Property->IsA(FByteProperty::StaticClass()) && CastField<const FByteProperty>(Property)->Enum)
		||	Property->IsA(FEnumProperty::StaticClass())
		|| (Property->IsA(FStrProperty::StaticClass()) && FRuntimeMetaData::HasMetaData(Property, TEXT("Enum")))
		|| !PropertyEditorHelpers::GetPropertyOptionsMetaDataKey(Property).IsNone()
		)
		&&	( ( ArrayIndex == -1 && Property->ArrayDim == 1 ) || ( ArrayIndex > -1 && Property->ArrayDim > 0 ) ) )
	{
		return true;
	}

	return false;
}

void SPropertyEditorCombo::Construct( const FArguments& InArgs, const TSharedPtr< class FPropertyEditor >& InPropertyEditor )
{
	PropertyEditor = InPropertyEditor;
	ComboArgs = InArgs._ComboArgs;

	TAttribute<FText> TooltipAttribute;

	if (PropertyEditor.IsValid())
	{
		ComboArgs.PropertyHandle = PropertyEditor->GetPropertyHandle();
		TooltipAttribute = TAttribute<FText>::Create(TAttribute<FText>::FGetter::CreateSP(PropertyEditor.ToSharedRef(), &FPropertyEditor::GetValueAsText));
	}

	TArray<TSharedPtr<FString>> ComboItems;
	TArray<bool> Restrictions;
	TArray<TSharedPtr<SToolTip>> RichToolTips;

	if (!ComboArgs.Font.HasValidFont())
	{
		ComboArgs.Font = FSodaStyle::GetFontStyle(PropertyEditorConstants::PropertyFontStyle);
	}

	GenerateComboBoxStrings(ComboItems, RichToolTips, Restrictions);

	SAssignNew(ComboBox, SPropertyComboBox)
		.Font( ComboArgs.Font )
		.RichToolTipList( RichToolTips )
		.ComboItemList( ComboItems )
		.RestrictedList( Restrictions )
		.OnSelectionChanged( this, &SPropertyEditorCombo::OnComboSelectionChanged )
		.OnComboBoxOpening( this, &SPropertyEditorCombo::OnComboOpening )
		.VisibleText( this, &SPropertyEditorCombo::GetDisplayValueAsString )
		.ToolTipText( TooltipAttribute )
		.ShowSearchForItemCount( ComboArgs.ShowSearchForItemCount );

	ChildSlot
	[
		ComboBox.ToSharedRef()
	];

	SetEnabled( TAttribute<bool>( this, &SPropertyEditorCombo::CanEdit ) );
}

FString SPropertyEditorCombo::GetDisplayValueAsString() const
{
	if (ComboArgs.OnGetValue.IsBound())
	{
		return ComboArgs.OnGetValue.Execute();
	}
	else if (PropertyEditor.IsValid())
	{
		return (bUsesAlternateDisplayValues) ? PropertyEditor->GetValueAsDisplayString() : PropertyEditor->GetValueAsString();
	}
	else
	{
		FString ValueString;

		if (bUsesAlternateDisplayValues)
		{
			ComboArgs.PropertyHandle->GetValueAsDisplayString(ValueString);
		}
		else
		{
			ComboArgs.PropertyHandle->GetValueAsFormattedString(ValueString);
		}
		return ValueString;
	}
}

void SPropertyEditorCombo::GenerateComboBoxStrings( TArray< TSharedPtr<FString> >& OutComboBoxStrings, TArray<TSharedPtr<SToolTip>>& RichToolTips, TArray<bool>& OutRestrictedItems )
{
	if (ComboArgs.OnGetStrings.IsBound())
	{
		ComboArgs.OnGetStrings.Execute(OutComboBoxStrings, RichToolTips, OutRestrictedItems);
		return;
	}

	TArray<FText> BasicTooltips;
	bUsesAlternateDisplayValues = ComboArgs.PropertyHandle->GeneratePossibleValues(OutComboBoxStrings, BasicTooltips, OutRestrictedItems);

	// For enums, look for rich tooltip information
	if(ComboArgs.PropertyHandle.IsValid())
	{
		if(const FProperty* Property = ComboArgs.PropertyHandle->GetProperty())
		{
			UEnum* Enum = nullptr;

			if(const FByteProperty* ByteProperty = CastField<FByteProperty>(Property))
			{
				Enum = ByteProperty->Enum;
			}
			else if(const FEnumProperty* EnumProperty = CastField<FEnumProperty>(Property))
			{
				Enum = EnumProperty->GetEnum();
			}

			if(Enum)
			{
				TArray<FName> AllowedPropertyEnums = PropertyEditorHelpers::GetValidEnumsFromPropertyOverride(Property, Enum);

				// Get enum doc link (not just GetDocumentationLink as that is the documentation for the struct we're in, not the enum documentation)
				FString DocLink = PropertyEditorHelpers::GetEnumDocumentationLink(Property);

				for(int32 EnumIdx = 0; EnumIdx < Enum->NumEnums() - 1; ++EnumIdx)
				{
					FString Excerpt = Enum->GetNameStringByIndex(EnumIdx);

					bool bShouldBeHidden = FRuntimeMetaData::HasMetaData(Enum, TEXT("Hidden"), EnumIdx) || FRuntimeMetaData::HasMetaData(Enum, TEXT("Spacer"), EnumIdx);
					if(!bShouldBeHidden && AllowedPropertyEnums.Num() != 0)
					{
						bShouldBeHidden = AllowedPropertyEnums.Find(Enum->GetNameByIndex(EnumIdx)) == INDEX_NONE;
					}

					if (!bShouldBeHidden)
					{
						bShouldBeHidden = ComboArgs.PropertyHandle->IsHidden(Excerpt);
					}
				
					if(!bShouldBeHidden)
					{
						RichToolTips.Add(soda::IDocumentation::Get()->CreateToolTip(MoveTemp(BasicTooltips[EnumIdx]), nullptr, DocLink, MoveTemp(Excerpt)));
					}
				}
			}
		}
	}
}

void SPropertyEditorCombo::OnComboSelectionChanged( TSharedPtr<FString> NewValue, ESelectInfo::Type SelectInfo )
{
	if ( NewValue.IsValid() )
	{
		SendToObjects( *NewValue );
	}
}

void SPropertyEditorCombo::OnComboOpening()
{
	TArray<TSharedPtr<FString>> ComboItems;
	TArray<TSharedPtr<SToolTip>> RichToolTips;
	TArray<bool> Restrictions;
	GenerateComboBoxStrings(ComboItems, RichToolTips, Restrictions);

	ComboBox->SetItemList(ComboItems, RichToolTips, Restrictions);

	// try and re-sync the selection in the combo list in case it was changed since Construct was called
	// this would fail if the displayed value doesn't match the equivalent value in the combo list
	FString CurrentDisplayValue = GetDisplayValueAsString();
	ComboBox->SetSelectedItem(CurrentDisplayValue);
}

void SPropertyEditorCombo::SendToObjects( const FString& NewValue )
{
	FString Value = NewValue;
	if (ComboArgs.OnValueSelected.IsBound())
	{
		ComboArgs.OnValueSelected.Execute(NewValue);
	}
	else if (ComboArgs.PropertyHandle.IsValid())
	{
		FProperty* Property = ComboArgs.PropertyHandle->GetProperty();

		if (bUsesAlternateDisplayValues && !Property->IsA(FStrProperty::StaticClass()))
		{
			// currently only enum properties can use alternate display values; this 
			// might change, so assert here so that if support is expanded to other 
			// property types without updating this block of code, we'll catch it quickly
			UEnum* Enum = nullptr;
			if (FByteProperty* ByteProperty = CastField<FByteProperty>(Property))
			{
				Enum = ByteProperty->Enum;
			}
			else if (FEnumProperty* EnumProperty = CastField<FEnumProperty>(Property))
			{
				Enum = EnumProperty->GetEnum();
			}
			check(Enum != nullptr);

			const int32 Index = FindEnumValueIndex(Enum, NewValue);
			check(Index != INDEX_NONE);

			Value = Enum->GetNameStringByIndex(Index);
		}

		ComboArgs.PropertyHandle->SetValueFromFormattedString(Value);
	}
}

bool SPropertyEditorCombo::CanEdit() const
{
	return PropertyEditor.IsValid() ? !PropertyEditor->IsEditConst() : true;
}

} // namespace soda

#undef LOCTEXT_NAMESPACE
