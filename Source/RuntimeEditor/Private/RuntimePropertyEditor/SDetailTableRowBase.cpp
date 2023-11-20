// Copyright Epic Games, Inc. All Rights Reserved.
// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "RuntimePropertyEditor/SDetailTableRowBase.h"

namespace soda
{

const float SDetailTableRowBase::ScrollBarPadding = 16.0f;

int32 SDetailTableRowBase::GetIndentLevelForBackgroundColor() const
{
	int32 IndentLevel = 0; 
	if (OwnerTablePtr.IsValid())
	{
		// every item is in a category, but we don't want to show an indent for "top-level" properties
		IndentLevel = GetIndentLevel() - 1;
	}

	TSharedPtr<FDetailTreeNode> DetailTreeNode = OwnerTreeNode.Pin();
	if (DetailTreeNode.IsValid() && 
		DetailTreeNode->GetDetailsView() != nullptr && 
		DetailTreeNode->GetDetailsView()->ContainsMultipleTopLevelObjects())
	{
		// if the row is in a multiple top level object display (eg. Project Settings), don't display an indent for the initial level
		--IndentLevel;
	}

	return FMath::Max(0, IndentLevel);
}

bool SDetailTableRowBase::IsScrollBarVisible(TWeakPtr<STableViewBase> OwnerTableViewWeak)
{
	TSharedPtr<STableViewBase> OwnerTableView = OwnerTableViewWeak.Pin();
	if (OwnerTableView.IsValid())
	{
		return OwnerTableView->GetScrollbarVisibility() == EVisibility::Visible;
	}
	return false;
}

} // namespace soda
