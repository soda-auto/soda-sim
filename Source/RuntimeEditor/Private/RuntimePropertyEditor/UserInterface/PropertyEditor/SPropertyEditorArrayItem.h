// Copyright Epic Games, Inc. All Rights Reserved.
// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Fonts/SlateFontInfo.h"
#include "SodaStyleSet.h"
#include "RuntimePropertyEditor/UserInterface/PropertyEditor/PropertyEditorConstants.h"
#include "RuntimeEditorModule.h"

namespace soda
{

class FPropertyEditor;
class IPropertyHandle;

struct FTitleMetadataFormatter
{
	FText Format;
	TArray<TSharedPtr<IPropertyHandle>> PropertyHandles;

	FPropertyAccess::Result GetDisplayText(FText& OutText) const;

	static TSharedPtr<FTitleMetadataFormatter> TryParse(TSharedPtr<IPropertyHandle> RootProperty, const FString& TitlePropertyRaw);
};

class SPropertyEditorArrayItem : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS( SPropertyEditorArrayItem ) 
		: _Font( FSodaStyle::GetFontStyle( PropertyEditorConstants::PropertyFontStyle ) ) 
		{}
		SLATE_ATTRIBUTE( FSlateFontInfo, Font )
	SLATE_END_ARGS()

	static bool Supports( const TSharedRef< class FPropertyEditor >& PropertyEditor );

	void Construct( const FArguments& InArgs, const TSharedRef< class FPropertyEditor>& InPropertyEditor );

	void GetDesiredWidth( float& OutMinDesiredWidth, float& OutMaxDesiredWidth );

private:
	FText GetValueAsString() const;

	/** @return True if the property can be edited */
	bool CanEdit() const;

private:
	TSharedPtr<FPropertyEditor> PropertyEditor;
	TSharedPtr<FTitleMetadataFormatter> TitlePropertyFormatter;
};

} // namespace soda
