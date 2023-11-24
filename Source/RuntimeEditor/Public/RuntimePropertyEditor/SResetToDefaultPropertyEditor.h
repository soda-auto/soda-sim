// Copyright Epic Games, Inc. All Rights Reserved.
// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Layout/Visibility.h"
#include "Input/Reply.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"
#include "RuntimePropertyEditor/IDetailPropertyRow.h"

namespace soda
{

// Forward decl
class FResetToDefaultOverride;

/** Widget showing the reset to default value button */
class RUNTIMEEDITOR_API SResetToDefaultPropertyEditor : public SCompoundWidget
{
public:

	SLATE_BEGIN_ARGS( SResetToDefaultPropertyEditor )
		: _NonVisibleState( EVisibility::Hidden )
		{}
		SLATE_ARGUMENT( EVisibility, NonVisibleState )
		SLATE_ARGUMENT( TOptional<FResetToDefaultOverride>, CustomResetToDefault )
	SLATE_END_ARGS()

	~SResetToDefaultPropertyEditor();

	void Construct( const FArguments& InArgs, const TSharedPtr< class IPropertyHandle>& InPropertyHandle );

private:
	FText GetResetToolTip() const;

	EVisibility GetDiffersFromDefaultAsVisibility() const;

	void Tick( const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime );

	FReply OnResetClicked();

	void UpdateDiffersFromDefaultState();

private:
	TOptional<FResetToDefaultOverride> OptionalCustomResetToDefault;

	TSharedPtr< class IPropertyHandle > PropertyHandle;

	EVisibility NonVisibleState;

	bool bValueDiffersFromDefault;
};

} // namespace soda
