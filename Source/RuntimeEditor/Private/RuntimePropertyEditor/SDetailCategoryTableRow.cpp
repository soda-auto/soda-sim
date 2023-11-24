// Copyright Epic Games, Inc. All Rights Reserved.
// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "RuntimePropertyEditor/SDetailCategoryTableRow.h"

#include "RuntimePropertyEditor/UserInterface/PropertyEditor/PropertyEditorConstants.h"
#include "RuntimePropertyEditor/SDetailExpanderArrow.h"
#include "RuntimePropertyEditor/SDetailRowIndent.h"
#include "Styling/StyleColors.h"

namespace soda
{

void SDetailCategoryTableRow::Construct(const FArguments& InArgs, TSharedRef<FDetailTreeNode> InOwnerTreeNode, const TSharedRef<STableViewBase>& InOwnerTableView)
{
	OwnerTreeNode = InOwnerTreeNode;

	bIsInnerCategory = InArgs._InnerCategory;
	bShowBorder = InArgs._ShowBorder;

	FDetailColumnSizeData& ColumnSizeData = InOwnerTreeNode->GetDetailsView()->GetColumnSizeData();

	TSharedRef<SHorizontalBox> HeaderBox = SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Fill)
		.AutoWidth()
		[
			SNew(SDetailRowIndent, SharedThis(this))
		]
		+ SHorizontalBox::Slot()
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Center)
		.Padding(2, 0, 0, 0)
		.AutoWidth()
		[
			SNew(SDetailExpanderArrow, SharedThis(this))
		]
		+ SHorizontalBox::Slot()
		.VAlign(VAlign_Center)
		.Padding(4, 0, 0, 0)
		.FillWidth(1)
		[
			SNew(STextBlock)
			.Text(InArgs._DisplayName)
			.Font(FSodaStyle::Get().GetFontStyle(bIsInnerCategory ? PropertyEditorConstants::PropertyFontStyle : PropertyEditorConstants::CategoryFontStyle))
			.TextStyle(FSodaStyle::Get(), "DetailsView.CategoryTextStyle")
		];

	if (InArgs._HeaderContent.IsValid())
	{
		HeaderBox->AddSlot()
			.VAlign(VAlign_Center)
			.FillWidth(1)
			[
				InArgs._HeaderContent.ToSharedRef()
			];
	}

	TWeakPtr<STableViewBase> OwnerTableViewWeak = InOwnerTableView;
	auto GetScrollbarWellBrush = [this, OwnerTableViewWeak]()
	{
		return SDetailTableRowBase::IsScrollBarVisible(OwnerTableViewWeak) ?
			FSodaStyle::Get().GetBrush("DetailsView.GridLine") :
			this->GetBackgroundImage();
	};

	auto GetScrollbarWellTint = [this, OwnerTableViewWeak]()
	{
		return SDetailTableRowBase::IsScrollBarVisible(OwnerTableViewWeak) ?
			FSlateColor(EStyleColor::White) :
			this->GetInnerBackgroundColor();
	};

	this->ChildSlot
	[
		SNew(SBorder)
		.BorderImage(FSodaStyle::Get().GetBrush("DetailsView.GridLine"))
		.Padding(FMargin(0, 0, 0, 1))
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.HAlign(HAlign_Fill)
			.Padding(0)
			[
				SNew(SBorder)
				.BorderImage(this, &SDetailCategoryTableRow::GetBackgroundImage)
				.BorderBackgroundColor(this, &SDetailCategoryTableRow::GetInnerBackgroundColor)
				.Padding(0)
				[
					SNew(SBox)
					.MinDesiredHeight(PropertyEditorConstants::PropertyRowHeight)
					.VAlign(VAlign_Center)
					[
						HeaderBox
					]
				]
			]
			+ SHorizontalBox::Slot()
			.HAlign(HAlign_Right)
			.VAlign(VAlign_Fill)
			.Padding(0)
			.AutoWidth()
			[
				SNew(SBorder)
				.BorderImage_Lambda(GetScrollbarWellBrush)
				.BorderBackgroundColor_Lambda(GetScrollbarWellTint)
				.Padding(FMargin(0, 0, SDetailTableRowBase::ScrollBarPadding, 0))
			]
		]
	];

	STableRow< TSharedPtr< FDetailTreeNode > >::ConstructInternal(
		STableRow::FArguments()
		.Style(FSodaStyle::Get(), "DetailsView.TreeView.TableRow")
		.ShowSelection(false),
		InOwnerTableView
	);
}

EVisibility SDetailCategoryTableRow::IsSeparatorVisible() const
{
	return bIsInnerCategory || IsItemExpanded() ? EVisibility::Collapsed : EVisibility::Visible;
}

const FSlateBrush* SDetailCategoryTableRow::GetBackgroundImage() const
{
	if (bShowBorder)
	{
		if (bIsInnerCategory)
		{
			return FSodaStyle::Get().GetBrush("DetailsView.CategoryMiddle");
		}

		// intentionally no hover on outer categories
		return FSodaStyle::Get().GetBrush("DetailsView.CategoryTop");
	}

	return nullptr;
}

FSlateColor SDetailCategoryTableRow::GetInnerBackgroundColor() const
{
	if (bShowBorder && bIsInnerCategory)
	{
		int32 IndentLevel = -1;
		if (OwnerTablePtr.IsValid())
		{
			IndentLevel = GetIndentLevel();
		}

		IndentLevel = FMath::Max(IndentLevel - 1, 0);

		return PropertyEditorConstants::GetRowBackgroundColor(IndentLevel, this->IsHovered());
	}

	return FSlateColor(FLinearColor::White);
}

FSlateColor SDetailCategoryTableRow::GetOuterBackgroundColor() const
{
	if (IsHovered())
	{
		return FSodaStyle::Get().GetSlateColor("Colors.Header");
	}

	return FSodaStyle::Get().GetSlateColor("Colors.Panel");
}

FReply SDetailCategoryTableRow::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		ToggleExpansion();
		return FReply::Handled();
	}
	else
	{
		return FReply::Unhandled();
	}
}

FReply SDetailCategoryTableRow::OnMouseButtonDoubleClick(const FGeometry& InMyGeometry, const FPointerEvent& InMouseEvent)
{
	return OnMouseButtonDown(InMyGeometry, InMouseEvent);
}

} // namespace soda