// Copyright Epic Games, Inc. All Rights Reserved.
// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Fonts/SlateFontInfo.h"
#include "Widgets/SWidget.h"
#include "SodaStyleSet.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"
#include "RuntimePropertyEditor/Presentation/PropertyEditor/PropertyEditor.h"
#include "RuntimePropertyEditor/UserInterface/PropertyEditor/PropertyEditorConstants.h"

namespace soda
{

class SPropertyEditorDateTime : public SCompoundWidget
{
public:

	SLATE_BEGIN_ARGS( SPropertyEditorDateTime )
		: _Font( FSodaStyle::GetFontStyle( PropertyEditorConstants::PropertyFontStyle ) ) 
		{}
		SLATE_ATTRIBUTE( FSlateFontInfo, Font )
	SLATE_END_ARGS()

	static bool Supports( const TSharedRef< class FPropertyEditor >& InPropertyEditor );

	void Construct( const FArguments& InArgs, const TSharedRef< class FPropertyEditor >& InPropertyEditor );

	void GetDesiredWidth( float& OutMinDesiredWidth, float& OutMaxDesiredWidth );

private:

	virtual void HandleTextCommitted( const FText& NewText, ETextCommit::Type CommitInfo );

	bool CanEdit() const;

private:

	TSharedPtr< class FPropertyEditor > PropertyEditor;

	TSharedPtr< class SWidget > PrimaryWidget;
};

} // namespace soda