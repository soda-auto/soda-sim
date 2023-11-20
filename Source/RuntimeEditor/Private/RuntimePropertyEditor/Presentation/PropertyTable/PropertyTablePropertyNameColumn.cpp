// Copyright Epic Games, Inc. All Rights Reserved.
// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "RuntimePropertyEditor/Presentation/PropertyTable/PropertyTablePropertyNameColumn.h"
//#include "Editor/EditorEngine.h"
#include "RuntimePropertyEditor/IPropertyTableCell.h"
#include "RuntimePropertyEditor/Presentation/PropertyTable/PropertyTableCell.h"
#include "RuntimePropertyEditor/Presentation/PropertyTable/DataSource.h"
#include "RuntimeEditorUtils.h"

#define LOCTEXT_NAMESPACE "PropertyNameColumnHeader"

namespace soda
{


FPropertyTablePropertyNameColumn::FPropertyTablePropertyNameColumn( const TSharedRef< IPropertyTable >& InTable )
	: bIsHidden( false )
	, Cells()
	, DataSource( MakeShareable( new NoDataSource() ) )
	, Table( InTable )
	, Width( 2.0f )
{

}


TSharedRef< class IPropertyTableCell > FPropertyTablePropertyNameColumn::GetCell( const TSharedRef< class IPropertyTableRow >& Row )
{
	TSharedRef< IPropertyTableCell >* CellPtr = Cells.Find( Row );

	if( CellPtr != NULL )
	{
		return *CellPtr;
	}

	TSharedRef< IPropertyTableCell > Cell = MakeShareable( new FPropertyTableCell( SharedThis( this ), Row ) );
	Cells.Add( Row, Cell );

	return Cell;
}


void FPropertyTablePropertyNameColumn::Sort( TArray<TSharedRef<class IPropertyTableRow>>& Rows, const EColumnSortMode::Type PrimarySortMode, const TSharedPtr<IPropertyTableColumn>& SecondarySortColumn, const EColumnSortMode::Type SecondarySortMode )
{
	struct FCompareRowByPropertyNameAscending
	{
	public:
		FCompareRowByPropertyNameAscending( const TSharedRef< FPropertyTablePropertyNameColumn >& Column )
			: NameColumn( Column )
		{ }

		FORCEINLINE bool operator()( const TSharedRef< IPropertyTableRow >& Lhs, const TSharedRef< IPropertyTableRow >& Rhs ) const
		{
			return NameColumn->GetPropertyNameAsString( Lhs ) < NameColumn->GetPropertyNameAsString( Rhs );
		}

		TSharedRef< FPropertyTablePropertyNameColumn > NameColumn;
	};

	struct FCompareRowByPropertyNameDescending
	{
	public:
		FCompareRowByPropertyNameDescending( const TSharedRef< FPropertyTablePropertyNameColumn >& Column )
			: Comparer( Column )
		{ }

		FORCEINLINE bool operator()( const TSharedRef< IPropertyTableRow >& Lhs, const TSharedRef< IPropertyTableRow >& Rhs ) const
		{
			return !Comparer( Lhs, Rhs ); 
		}

	private:

		const FCompareRowByPropertyNameAscending Comparer;
	};

	if (PrimarySortMode == EColumnSortMode::None )
	{
		return;
	}

	if (PrimarySortMode == EColumnSortMode::Ascending )
	{
		Rows.Sort( FCompareRowByPropertyNameAscending( SharedThis( this ) ) );
	}
	else
	{
		Rows.Sort( FCompareRowByPropertyNameDescending( SharedThis( this ) ) );
	}
}


TSharedPtr<struct FCompareRowByColumnBase> FPropertyTablePropertyNameColumn::GetPropertySorter(FProperty* Property, EColumnSortMode::Type SortMode)
{
	// Does not sort properties
	return nullptr;
}

FString FPropertyTablePropertyNameColumn::GetPropertyNameAsString( const TSharedRef< IPropertyTableRow >& Row )
{
	FString PropertyName;
	if( Row->GetDataSource()->AsPropertyPath().IsValid() )
	{
		const TWeakFieldPtr< FProperty > Property = Row->GetDataSource()->AsPropertyPath()->GetLeafMostProperty().Property;
		PropertyName = FRuntimeEditorUtils::GetFriendlyName( Property.Get() );
	}
	return PropertyName;
}

} // namespace soda

#undef LOCTEXT_NAMESPACE
