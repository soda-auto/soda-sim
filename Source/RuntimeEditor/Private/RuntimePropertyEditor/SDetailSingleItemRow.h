// Copyright Epic Games, Inc. All Rights Reserved.
// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RuntimePropertyEditor/DetailTreeNode.h"
#include "RuntimePropertyEditor/PropertyCustomizationHelpers.h"
#include "RuntimePropertyEditor/SDetailTableRowBase.h"
#include "RuntimePropertyEditor/SDetailsViewBase.h"
//#include "ScopedTransaction.h"

//#include "DragAndDrop/DecoratedDragDropOp.h"
#include "Framework/Commands/Commands.h"
#include "Input/Reply.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SWidget.h"
#include "Widgets/Views/STableViewBase.h"

namespace soda
{

class IDetailKeyframeHandler;
struct FDetailLayoutCustomization;
class SDetailSingleItemRow;

class SArrayRowHandle : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SArrayRowHandle)
	{}
	SLATE_DEFAULT_SLOT(FArguments, Content)
	SLATE_ARGUMENT(TSharedPtr<SDetailSingleItemRow>, ParentRow)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	/*
	FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
	{
		return FReply::Handled().DetectDrag(SharedThis(this), EKeys::LeftMouseButton);
	};
	*/

	//FReply OnDragDetected(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;

private:
	TWeakPtr<SDetailSingleItemRow> ParentRow;
};

/**
 * A widget for details that span the entire tree row and have no columns                                                              
 */
class SDetailSingleItemRow : public SDetailTableRowBase
{
public:
	SLATE_BEGIN_ARGS( SDetailSingleItemRow )
		: _ColumnSizeData() {}

		SLATE_ARGUMENT( FDetailColumnSizeData, ColumnSizeData )
		SLATE_ARGUMENT( bool, AllowFavoriteSystem)
	SLATE_END_ARGS()

	/**
	 * Construct the widget
	 */
	void Construct( const FArguments& InArgs, FDetailLayoutCustomization* InCustomization, bool bHasMultipleColumns, TSharedRef<FDetailTreeNode> InOwnerTreeNode, const TSharedRef<STableViewBase>& InOwnerTableView );

	// ~Begin SWidget Interface
	virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	// ~Begin End Interface

	//TSharedPtr<FDragDropOperation> CreateDragDropOperation();

	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime);
protected:
	virtual bool OnContextMenuOpening( FMenuBuilder& MenuBuilder ) override;
private:
	void OnCopyProperty();
	void OnCopyPropertyDisplayName();
	void OnPasteProperty();
	bool CanPasteProperty() const;
	FSlateColor GetOuterBackgroundColor() const;
	FSlateColor GetInnerBackgroundColor() const;

	void CreateGlobalExtensionWidgets(TArray<FPropertyRowExtensionButton>& ExtensionButtons) const;
	void PopulateExtensionWidget();
	void OnResetToDefaultClicked() const;
	bool IsResetToDefaultEnabled() const;

	bool IsHighlighted() const;

	void OnFavoriteMenuToggle();
	bool CanFavorite() const;
	bool IsFavorite() const;

	/** UIActions to help populate the PropertyEditorPermissionList, which must first be turned on through FPropertyEditorPermissionList::Get().SetShouldShowMenuEntries */
	void CopyRowNameText() const;
	void OnToggleAllowList() const;
	bool IsAllowListChecked() const;
	void OnToggleDenyList() const;
	bool IsDenyListChecked() const;

	/*
	void OnArrayOrCustomDragLeave(const FDragDropEvent& DragDropEvent);
	FReply OnArrayAcceptDrop(const FDragDropEvent& DragDropEvent, EItemDropZone DropZone, TSharedPtr<FDetailTreeNode> TargetItem);
	FReply OnArrayHeaderAcceptDrop(const FDragDropEvent& DragDropEvent, EItemDropZone DropZone, TSharedPtr<FDetailTreeNode> TargetItem);

	TOptional<EItemDropZone> OnArrayCanAcceptDrop(const FDragDropEvent& DragDropEvent, EItemDropZone DropZone, TSharedPtr< FDetailTreeNode > Type);

	FReply OnCustomAcceptDrop(const FDragDropEvent& DragDropEvent, EItemDropZone DropZone, TSharedPtr<FDetailTreeNode> TargetItem);
	TOptional<EItemDropZone> OnCustomCanAcceptDrop(const FDragDropEvent& DragDropEvent, EItemDropZone DropZone, TSharedPtr<FDetailTreeNode> Type);
	*/

	/** Checks if the current drop event is being dropped into a valid location
	 */
	bool CheckValidDrop(const TSharedPtr<SDetailSingleItemRow> RowPtr, EItemDropZone DropZone) const;
	
	TSharedPtr<FPropertyNode> GetPropertyNode() const;
	TSharedPtr<IPropertyHandle> GetPropertyHandle() const;

	bool UpdateResetToDefault();
private:
	/** Customization for this widget */
	FDetailLayoutCustomization* Customization;
	FDetailWidgetRow WidgetRow;
	bool bAllowFavoriteSystem;
	bool bCachedResetToDefaultEnabled;
	TSharedPtr<FPropertyNode> SwappablePropertyNode;
	TSharedPtr<SButton> ExpanderArrow;
	//TWeakPtr<FDragDropOperation> DragOperation; // last drag initiated by this widget
	FUIAction CopyAction;
	FUIAction PasteAction;

	/** Animation curve for displaying pulse */
	FCurveSequence PulseAnimation;
};
/*
class FArrayRowDragDropOp : public FSodaDecoratedDragDropOp
{
public:
	DRAG_DROP_OPERATOR_TYPE(FArrayRowDragDropOp, FSodaDecoratedDragDropOp)

	FArrayRowDragDropOp(TSharedPtr<SDetailSingleItemRow> InRow);

	//Inits the tooltip, needs to be called after constructing 
	void Init();

	// Update the drag tool tip indicating whether the current drop target is valid
	void SetValidTarget(bool IsValidTarget);

	TWeakPtr<SDetailSingleItemRow> Row;
};
*/

} // namespace soda