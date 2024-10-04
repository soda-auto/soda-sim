// Copyright Epic Games, Inc. All Rights Reserved.
// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Fonts/SlateFontInfo.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Input/Reply.h"
#include "Widgets/SWidget.h"
#include "SodaStyleSet.h"
#include "RuntimePropertyEditor/UserInterface/PropertyEditor/PropertyEditorConstants.h"
#include "SSearchableComboBox.h"

class SToolTip;

namespace soda
{

class SPropertyComboBox : public SSearchableComboBox
{
public:

	SLATE_BEGIN_ARGS( SPropertyComboBox )
		: _Font( FSodaStyle::GetFontStyle( PropertyEditorConstants::PropertyFontStyle ) )
		, _ShowSearchForItemCount(-1)
	{}
		SLATE_ATTRIBUTE( TArray< TSharedPtr< FString > >, ComboItemList )
		SLATE_ATTRIBUTE( TArray< bool >, RestrictedList )
		SLATE_ATTRIBUTE( FString, VisibleText )
		SLATE_ARGUMENT( TArray< TSharedPtr< SToolTip > >, RichToolTipList)
		SLATE_EVENT( FOnSelectionChanged, OnSelectionChanged )
		SLATE_EVENT( FOnComboBoxOpening, OnComboBoxOpening )
		SLATE_ARGUMENT( FSlateFontInfo, Font )
		SLATE_ARGUMENT( int32, ShowSearchForItemCount )
	SLATE_END_ARGS()

	
	void Construct( const FArguments& InArgs );

	~SPropertyComboBox();

	/**
	 *	Sets the currently selected item for the combo box.
	 *	@param InSelectedItem			The name of the item to select.
	 */
	void SetSelectedItem( const FString& InSelectedItem );

	/** 
	 *	Sets the item list for the combo box.
	 *	@param InItemList			The item list to use.
	 *	@param InTooltipList		The list of tooltips to use.
	 *	@param InRestrictedList		The list of restricted items.
	 */
	void SetItemList(TArray< TSharedPtr< FString > >& InItemList, TArray< TSharedPtr< SToolTip > >& InRichTooltips, TArray<bool>& InRestrictedList);


private:

	void OnSelectionChangedInternal( TSharedPtr<FString> InSelectedItem, ESelectInfo::Type SelectInfo );

	TSharedRef<SWidget> OnGenerateComboWidget( TSharedPtr<FString> InComboString );
	
	EVisibility GetSearchVisibility() const;

	/** SWidget interface */
	FReply OnKeyDown( const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent ) override;

private:

	/** List of items in our combo box. Only generated once as combo items dont change at runtime */
	TArray< TSharedPtr<FString> > ComboItemList;
	TArray< TSharedPtr< SToolTip > > RichToolTips;
	FOnSelectionChanged OnSelectionChanged;
	FSlateFontInfo Font;
	TArray< bool > RestrictedList;
	int32 ShowSearchForItemCount;
};

} // namespace soda