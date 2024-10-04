// Copyright Epic Games, Inc. All Rights Reserved.
// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "RuntimePropertyEditor/Presentation/PropertyTable/PropertyTablePropertyNameCell.h"
#include "RuntimePropertyEditor/IPropertyTableColumn.h"
#include "RuntimePropertyEditor/Presentation/PropertyTable/PropertyTablePropertyNameColumn.h"

namespace soda
{

FPropertyTablePropertyNameCell::FPropertyTablePropertyNameCell( const TSharedRef< class FPropertyTablePropertyNameColumn >& InColumn, const TSharedRef< class IPropertyTableRow >& InRow )
	: bInEditMode( false )
	, bIsBound( true )
	, Column( InColumn )
	, EnteredEditModeEvent()
	, ExitedEditModeEvent()
	, Row( InRow )
{
	Refresh();
}

void FPropertyTablePropertyNameCell::Refresh()
{
	const TSharedRef< IPropertyTableColumn > ColumnRef = Column.Pin().ToSharedRef();
	const TSharedRef< IPropertyTableRow > RowRef = Row.Pin().ToSharedRef();

	ObjectNode = GetTable()->GetObjectPropertyNode( ColumnRef, RowRef );

	bIsBound = ObjectNode.IsValid();
}

FString FPropertyTablePropertyNameCell::GetValueAsString() const
{
	return Row.Pin()->GetDataSource()->AsPropertyPath()->ToString();
}

FText FPropertyTablePropertyNameCell::GetValueAsText() const
{
	return FText::FromString(GetValueAsString());
}

TSharedRef< class IPropertyTable > FPropertyTablePropertyNameCell::GetTable() const
{
	return Column.Pin()->GetTable();
}

void FPropertyTablePropertyNameCell::EnterEditMode() 
{
}

TSharedRef< class IPropertyTableColumn > FPropertyTablePropertyNameCell::GetColumn() const
{ 
	return Column.Pin().ToSharedRef(); 
}

TWeakObjectPtr< UObject > FPropertyTablePropertyNameCell::GetObject() const
{
	if ( !ObjectNode.IsValid() )
	{
		return NULL;
	}

	return ObjectNode->GetUObject( 0 );
}

TSharedRef< class IPropertyTableRow > FPropertyTablePropertyNameCell::GetRow() const
{ 
	return Row.Pin().ToSharedRef(); 
}

void FPropertyTablePropertyNameCell::ExitEditMode()
{
}

} // namespace soda