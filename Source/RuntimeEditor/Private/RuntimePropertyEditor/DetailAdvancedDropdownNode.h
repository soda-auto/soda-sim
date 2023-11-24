// Copyright Epic Games, Inc. All Rights Reserved.
// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Misc/Attribute.h"
#include "Input/Reply.h"
#include "RuntimePropertyEditor/IPropertyUtilities.h"
#include "RuntimePropertyEditor/DetailTreeNode.h"
#include "Widgets/Views/STableViewBase.h"
#include "Widgets/Views/STableRow.h"
#include "RuntimePropertyEditor/DetailCategoryBuilderImpl.h"
#include "RuntimePropertyEditor/PropertyCustomizationHelpers.h"

namespace soda
{

class FAdvancedDropdownNode : public FDetailTreeNode, public TSharedFromThis<FAdvancedDropdownNode>
{
public:
	FAdvancedDropdownNode(TSharedRef<FDetailCategoryImpl> InParentCategory, const TAttribute<bool>& InExpanded, const TAttribute<bool>& InEnabled, TAttribute<bool> InVisible)
		: ParentCategory(InParentCategory.Get())
		, IsEnabled(InEnabled)
		, IsExpanded(InExpanded)
		, IsVisible(InVisible)
	{
		SetParentNode(InParentCategory);
	}

private:
	/** IDetailTreeNode Interface */
	virtual IDetailsView* GetNodeDetailsView() const override { return ParentCategory.GetNodeDetailsView(); }
	virtual IDetailsViewPrivate* GetDetailsView() const override { return ParentCategory.GetDetailsView(); }
	virtual TSharedRef< ITableRow > GenerateWidgetForTableView( const TSharedRef<STableViewBase>& OwnerTable, bool bAllowFavoriteSystem) override;
	virtual bool GenerateStandaloneWidget(FDetailWidgetRow& OutRow) const override;
	virtual void GetChildren(FDetailNodeList& OutChildren) override {}
	virtual void OnItemExpansionChanged( bool bIsExpanded, bool bShouldSaveState) override {}
	virtual bool ShouldBeExpanded() const override { return false; }
	virtual ENodeVisibility GetVisibility() const override;
	virtual void FilterNode( const FDetailFilter& InFilter ) override {}
	virtual void Tick( float DeltaTime ) override {}
	virtual bool ShouldShowOnlyChildren() const override { return false; }
	virtual FName GetNodeName() const override { return NAME_None; }

	virtual EDetailNodeType GetNodeType() const override { return EDetailNodeType::Advanced; }
	virtual TSharedPtr<IPropertyHandle> CreatePropertyHandle() const override { return nullptr; }

	/** Called when the advanced drop down arrow is clicked */
	FReply OnAdvancedDropDownClicked();

private:
	FDetailCategoryImpl& ParentCategory;
	TAttribute<bool> IsEnabled;
	TAttribute<bool> IsExpanded;
	TAttribute<bool> IsVisible;
};

} // namespace soda
