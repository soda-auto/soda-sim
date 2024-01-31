// Copyright Epic Games, Inc. All Rights Reserved.
// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "RuntimePropertyEditor/DetailTreeNode.h"
#include "Input/Reply.h"
#include "Layout/Visibility.h"
#include "RuntimePropertyEditor/SDetailTableRowBase.h"
#include "Templates/SharedPointer.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/Views/STableRow.h"
#include "Widgets/Views/STableViewBase.h"

namespace soda
{

class SDetailCategoryTableRow : public SDetailTableRowBase
{
public:
	SLATE_BEGIN_ARGS(SDetailCategoryTableRow)
		: _InnerCategory(false)
		, _ShowBorder(true)
	{}
		SLATE_ARGUMENT(FText, DisplayName)
		SLATE_ARGUMENT(bool, InnerCategory)
		SLATE_ARGUMENT(TSharedPtr<SWidget>, HeaderContent)
		SLATE_ARGUMENT(bool, ShowBorder)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, TSharedRef<FDetailTreeNode> InOwnerTreeNode, const TSharedRef<STableViewBase>& InOwnerTableView);

private:
	EVisibility IsSeparatorVisible() const;
	const FSlateBrush* GetBackgroundImage() const;
	FSlateColor GetInnerBackgroundColor() const;
	FSlateColor GetOuterBackgroundColor() const;

	FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	FReply OnMouseButtonDoubleClick(const FGeometry& InMyGeometry, const FPointerEvent& InMouseEvent) override;

private:
	bool bIsInnerCategory;
	bool bShowBorder;
};

}
