// Copyright Epic Games, Inc. All Rights Reserved.
// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RuntimePropertyEditor/PropertyPath.h"
#include "RuntimePropertyEditor/IPropertyTable.h"
#include "RuntimePropertyEditor/IPropertyTableRow.h"

namespace soda
{

class FPropertyTableRow : public TSharedFromThis< FPropertyTableRow >, public IPropertyTableRow
{
public:
	
	FPropertyTableRow( const TSharedRef< class IPropertyTable >& InTable, const TWeakObjectPtr< UObject >& InObject  );

	FPropertyTableRow( const TSharedRef< class IPropertyTable >& InTable, const TSharedRef< FPropertyPath >& InPropertyPath );

	FPropertyTableRow( const TSharedRef< class IPropertyTable >& InTable, const TWeakObjectPtr< UObject >& InObject, const TSharedRef< FPropertyPath >& InPartialPropertyPath );

	FPropertyTableRow( const TSharedRef< class IPropertyTable >& InTable, const TSharedRef< FPropertyPath >& InPropertyPath, const TSharedRef< FPropertyPath >& InPartialPropertyPath );

	virtual ~FPropertyTableRow() {}

	virtual TSharedRef< class IDataSource > GetDataSource() const override { return DataSource; }

	virtual bool HasChildren() const override;

	virtual void GetChildRows( TArray< TSharedRef< class IPropertyTableRow > >& OutChildren ) const override;

	virtual TSharedRef< class IPropertyTable > GetTable() const override;

	virtual bool HasCells() const override { return true; }

	virtual TSharedRef< class FPropertyPath > GetPartialPath() const override { return PartialPath; }

	virtual void Tick() override;

	virtual void Refresh() override;

	DECLARE_DERIVED_EVENT( FPropertyTableRow, IPropertyTableRow::FRefreshed, FRefreshed );
	virtual FRefreshed* OnRefresh() override { return &Refreshed; }


private:

	void GenerateChildren();


private:

	TSharedRef< class IDataSource > DataSource;

	TWeakPtr< class IPropertyTable > Table;

	TArray< TSharedRef< class IPropertyTableRow > > Children;

	TSharedRef< class FPropertyPath > PartialPath;

	FRefreshed Refreshed;
};

} // namespace soda
