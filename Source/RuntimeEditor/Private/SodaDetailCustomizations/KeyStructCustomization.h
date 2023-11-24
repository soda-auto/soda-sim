// Copyright Epic Games, Inc. All Rights Reserved.
// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "InputCoreTypes.h"
#include "RuntimePropertyEditor/IPropertyTypeCustomization.h"
#include "RuntimePropertyEditor/PropertyHandle.h"

namespace soda
{

/**
 * Implements a details panel customization for FKey structures.
 * As  "Key"				<SKeySelector>
 */
class  FKeyStructCustomization
	: public IPropertyTypeCustomization
{
public:
	// IPropertyTypeCustomization interface

	virtual void CustomizeHeader(TSharedRef<class IPropertyHandle> StructPropertyHandle, class FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;
	virtual void CustomizeChildren(TSharedRef<IPropertyHandle> StructPropertyHandle, class IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils) override { };
	virtual bool ShouldInlineKey() const override { return true; }

	// Helper variant that generates the key struct in the header and appends a single button at the end
	// TODO: Is there a better way?
	void CustomizeHeaderOnlyWithButton(TSharedRef<class IPropertyHandle> StructPropertyHandle, class FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils, TSharedRef<SWidget> Button);

public:

	/**
	 * Creates a new instance.
	 *
	 * @return A new struct customization for Keys.
	 */
	static TSharedRef<IPropertyTypeCustomization> MakeInstance( );

protected:

	/** Gets the current Key being edited. */
	TOptional<FKey> GetCurrentKey() const;

	/** Updates the property when a new key is selected. */
	void OnKeyChanged(TSharedPtr<FKey> SelectedKey);

	/** Holds a handle to the property being edited. */
	TSharedPtr<IPropertyHandle> PropertyHandle;
};

} //namespace soda
