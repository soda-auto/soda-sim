// Copyright Epic Games, Inc. All Rights Reserved.
// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Fonts/SlateFontInfo.h"
#include "RuntimeEditorModule.h"

class FNotifyHook;

namespace soda
{

/**
 * Init params for a single property
 */
struct FSinglePropertyParams
{
	/** Override for the property name that will be displayed instead of the property name */
	FText NameOverride;

	/** Font to use instead of the default property font */
	FSlateFontInfo Font;

	/** Notify hook interface to use for some property change events */
	FNotifyHook* NotifyHook;

	/** Whether or not to show the name */
	EPropertyNamePlacement::Type NamePlacement;

	/** Whether to hide an asset thumbnail, if available */
	bool bHideAssetThumbnail;
		
	FSinglePropertyParams()
		: NameOverride(FText::GetEmpty())
		, Font()
		, NotifyHook( NULL )
		, NamePlacement( EPropertyNamePlacement::Left )
		, bHideAssetThumbnail( false )
	{
	}
};


/**
 * Represents a single property not in a property tree or details view for a single object
 * Structs and Array properties cannot be used with this method
 */
class ISinglePropertyView : public SCompoundWidget
{
public:
	/** Sets the object to view/edit on the widget */
	virtual void SetObject( UObject* InObject ) = 0;

	/** Sets a delegate called when the property value changes */
	virtual void SetOnPropertyValueChanged( FSimpleDelegate& InOnPropertyValueChanged ) = 0;

	/** Whether or not this widget has a valid property */	
	virtual bool HasValidProperty() const = 0;

	virtual TSharedPtr<class IPropertyHandle> GetPropertyHandle() const = 0;
};

} // namespace soda
