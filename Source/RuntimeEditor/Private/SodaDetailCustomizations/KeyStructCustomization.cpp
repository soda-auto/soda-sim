// Copyright Epic Games, Inc. All Rights Reserved.
// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "SodaDetailCustomizations/KeyStructCustomization.h"
#include "RuntimePropertyEditor/DetailWidgetRow.h"
//#include "InputSettingsDetails.h"
#include "SodaEditorWidgets/SKeySelector.h"
#include "UObject/UnrealType.h"

#define LOCTEXT_NAMESPACE "FKeyStructCustomization"

namespace soda
{

namespace InputConstants
{
	const FMargin PropertyPadding(2.0f, 0.0f, 2.0f, 0.0f);
	const float TextBoxWidth = 250.0f;
	const float ScaleBoxWidth = 50.0f;
}

/* FKeyStructCustomization static interface
 *****************************************************************************/

TSharedRef<IPropertyTypeCustomization> FKeyStructCustomization::MakeInstance( )
{
	return MakeShareable(new FKeyStructCustomization);
}

/* IPropertyTypeCustomization interface
 *****************************************************************************/

void FKeyStructCustomization::CustomizeHeader( TSharedRef<class IPropertyHandle> StructPropertyHandle, class FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils )
{
	PropertyHandle = StructPropertyHandle;

	// create struct header
	HeaderRow.NameContent()
	[
		StructPropertyHandle->CreatePropertyNameWidget()
	]
	.ValueContent()
	.MinDesiredWidth(125.0f)
	.MaxDesiredWidth(325.0f)
	[
		SNew(SKeySelector)
		.CurrentKey(this, &FKeyStructCustomization::GetCurrentKey)
		.OnKeyChanged(this, &FKeyStructCustomization::OnKeyChanged)
		.Font(StructCustomizationUtils.GetRegularFont())
		.AllowClear(!StructPropertyHandle->GetProperty()->HasAnyPropertyFlags(CPF_NoClear))
		.FilterBlueprintBindable(false)
	];
}

void FKeyStructCustomization::CustomizeHeaderOnlyWithButton(TSharedRef<class IPropertyHandle> StructPropertyHandle, class FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils, TSharedRef<SWidget> Button)
{
	PropertyHandle = StructPropertyHandle;

	// create struct header
	HeaderRow.NameContent()
	.MinDesiredWidth(125.0f)
	.MaxDesiredWidth(325.0f)
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.Padding(InputConstants::PropertyPadding)
		//.AutoWidth()
		[
			SNew(SKeySelector)
			.CurrentKey(this, &FKeyStructCustomization::GetCurrentKey)
			.OnKeyChanged(this, &FKeyStructCustomization::OnKeyChanged)
			.Font(StructCustomizationUtils.GetRegularFont())
			.AllowClear(!StructPropertyHandle->GetProperty()->HasAnyPropertyFlags(CPF_NoClear))
		    .FilterBlueprintBindable(false)
		]
		+ SHorizontalBox::Slot()
		.Padding(InputConstants::PropertyPadding)
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		.AutoWidth()
		[
			Button
		]
	];
}

TOptional<FKey> FKeyStructCustomization::GetCurrentKey() const
{
	TArray<void*> StructPtrs;
	PropertyHandle->AccessRawData(StructPtrs);

	if (StructPtrs.Num() > 0)
	{
		FKey* SelectedKey = (FKey*)StructPtrs[0];

		if (SelectedKey)
		{
			for(int32 StructPtrIndex = 1; StructPtrIndex < StructPtrs.Num(); ++StructPtrIndex)
			{
				if (*(FKey*)StructPtrs[StructPtrIndex] != *SelectedKey)
				{
					return TOptional<FKey>();
				}
			}

			return *SelectedKey;
		}
	}

	return FKey();
}

void FKeyStructCustomization::OnKeyChanged(TSharedPtr<FKey> SelectedKey)
{
	PropertyHandle->SetValueFromFormattedString(SelectedKey->ToString());
}

}

#undef LOCTEXT_NAMESPACE
