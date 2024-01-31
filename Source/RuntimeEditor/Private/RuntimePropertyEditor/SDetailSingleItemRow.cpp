// Copyright Epic Games, Inc. All Rights Reserved.
// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "RuntimePropertyEditor/SDetailSingleItemRow.h"

#include "RuntimePropertyEditor/DetailGroup.h"
#include "RuntimePropertyEditor/DetailPropertyRow.h"
#include "RuntimePropertyEditor/DetailWidgetRow.h"
//#include "Editor.h"
//#include "EditorMetadataOverrides.h"
//#include "IDetailDragDropHandler.h"
#include "RuntimePropertyEditor/IDetailPropertyExtensionHandler.h"
#include "RuntimePropertyEditor/ObjectPropertyNode.h"
#include "RuntimePropertyEditor/UserInterface/PropertyEditor/PropertyEditorConstants.h"
//#include "RuntimeEditorModule.h"
#include "RuntimePropertyEditor/PropertyHandleImpl.h"
#include "RuntimePropertyEditor/SConstrainedBox.h"
#include "RuntimePropertyEditor/SDetailExpanderArrow.h"
#include "RuntimePropertyEditor/SDetailRowIndent.h"
#include "RuntimePropertyEditor/SResetToDefaultPropertyEditor.h"

#include "HAL/PlatformApplicationMisc.h"
#include "Modules/ModuleInterface.h"
#include "Modules/ModuleManager.h"
//#include "Settings/EditorExperimentalSettings.h"
#include "Styling/StyleColors.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SComboButton.h"
#include "RuntimePropertyEditor/PropertyEditorPermissionList.h"

namespace soda
{

namespace DetailWidgetConstants
{
	const FMargin LeftRowPadding( 20.0f, 0.0f, 10.0f, 0.0f );
	const FMargin RightRowPadding( 12.0f, 0.0f, 2.0f, 0.0f );
	const float PulseAnimationLength = 0.5f;
}

namespace SDetailSingleItemRow_Helper
{
	// Get the node item number in case it is expand we have to recursively count all expanded children
	void RecursivelyGetItemShow(TSharedRef<FDetailTreeNode> ParentItem, int32& ItemShowNum)
	{
		if (ParentItem->GetVisibility() == ENodeVisibility::Visible)
		{
			ItemShowNum++;
		}

		if (ParentItem->ShouldBeExpanded())
		{
			TArray< TSharedRef<FDetailTreeNode> > Childrens;
			ParentItem->GetChildren(Childrens);
			for (TSharedRef<FDetailTreeNode> ItemChild : Childrens)
			{
				RecursivelyGetItemShow(ItemChild, ItemShowNum);
			}
		}
	}
}

/*
void SDetailSingleItemRow::OnArrayOrCustomDragLeave(const FDragDropEvent& DragDropEvent)
{
	TSharedPtr<FSodaDecoratedDragDropOp> DecoratedOp = DragDropEvent.GetOperationAs<FSodaDecoratedDragDropOp>();
	if (DecoratedOp.IsValid())
	{
		DecoratedOp->ResetToDefaultToolTip();
	}
}
*/
/** Compute the new index to which to move the item when it's dropped onto the given index/drop zone. */
static int32 ComputeNewIndex(int32 OriginalIndex, int32 DropOntoIndex, EItemDropZone DropZone)
{
	check(DropZone != EItemDropZone::OntoItem);

	int32 NewIndex = DropOntoIndex;
	if (DropZone == EItemDropZone::BelowItem)
	{
		// If the drop zone is below, then we actually move it to the next item's index
		NewIndex++;
	}
	if (OriginalIndex < NewIndex)
	{
		// If the item is moved down the list, then all the other elements below it are shifted up one
		NewIndex--;
	}

	return ensure(NewIndex >= 0) ? NewIndex : 0;
}

bool SDetailSingleItemRow::CheckValidDrop(const TSharedPtr<SDetailSingleItemRow> RowPtr, EItemDropZone DropZone) const
{
	// Can't drop onto another array item; need to drop above or below
	if (DropZone == EItemDropZone::OntoItem)
	{
		return false;
	}

	// Disallow dropping immediately below an expanded item (e.g. array of structs) since the drop zone is between the item and its children
	// User can drop above the next array item instead
	// For example:
	// - A    <- allow drop below A because it's collapsed
	// + B    <- don't allow drop below B because it's expanded;
	// | - a     dropping here seems to "parent" the dropped row to B
	// | - b
	// - C    <- user can drop above C instead
	if (DropZone == EItemDropZone::BelowItem && IsItemExpanded())
	{
		return false;
	}

	TSharedPtr<FPropertyNode> SwappingPropertyNode = RowPtr->SwappablePropertyNode;
	if (SwappingPropertyNode.IsValid() && SwappablePropertyNode.IsValid() && SwappingPropertyNode != SwappablePropertyNode)
	{
		const int32 OriginalIndex = SwappingPropertyNode->GetArrayIndex();
		const int32 NewIndex = ComputeNewIndex(OriginalIndex, SwappablePropertyNode->GetArrayIndex(), DropZone);

		if (OriginalIndex != NewIndex)
		{
			IDetailsViewPrivate* DetailsView = OwnerTreeNode.Pin()->GetDetailsView();
			TSharedPtr<IPropertyHandle> SwappingHandle = PropertyEditorHelpers::GetPropertyHandle(SwappingPropertyNode.ToSharedRef(), DetailsView->GetNotifyHook(), DetailsView->GetPropertyUtilities());
			TSharedPtr<IPropertyHandleArray> ParentHandle = SwappingHandle->GetParentHandle()->AsArray();

			if (ParentHandle.IsValid() && SwappablePropertyNode->GetParentNode() == SwappingPropertyNode->GetParentNode())
			{
				return true;
			}
		}
	}
	return false;
}

/*
FReply SDetailSingleItemRow::OnArrayAcceptDrop(const FDragDropEvent& DragDropEvent, EItemDropZone DropZone, TSharedPtr<FDetailTreeNode> TargetItem)
{
	TSharedPtr<FArrayRowDragDropOp> ArrayDropOp = DragDropEvent.GetOperationAs< FArrayRowDragDropOp >();
	if (!ArrayDropOp.IsValid())
	{
		return FReply::Unhandled();
	}

	TSharedPtr<SDetailSingleItemRow> RowPtr = ArrayDropOp->Row.Pin();
	if (!RowPtr.IsValid())
	{
		return FReply::Unhandled();
	}

	if (!CheckValidDrop(RowPtr, DropZone))
	{
		return FReply::Unhandled();
	}

	IDetailsViewPrivate* DetailsView = OwnerTreeNode.Pin()->GetDetailsView();

	TSharedPtr<FPropertyNode> SwappingPropertyNode = RowPtr->SwappablePropertyNode;
	TSharedPtr<IPropertyHandle> SwappingHandle = PropertyEditorHelpers::GetPropertyHandle(SwappingPropertyNode.ToSharedRef(), DetailsView->GetNotifyHook(), DetailsView->GetPropertyUtilities());
	TSharedPtr<IPropertyHandleArray> ParentHandle = SwappingHandle->GetParentHandle()->AsArray();
	const int32 OriginalIndex = SwappingPropertyNode->GetArrayIndex();
	const int32 NewIndex = ComputeNewIndex(OriginalIndex, SwappablePropertyNode->GetArrayIndex(), DropZone);

	// Need to swap the moving and target expansion states before saving
	bool bOriginalSwappableExpansion = SwappablePropertyNode->HasNodeFlags(EPropertyNodeFlags::Expanded) != 0;
	bool bOriginalSwappingExpansion = SwappingPropertyNode->HasNodeFlags(EPropertyNodeFlags::Expanded) != 0;
	SwappablePropertyNode->SetNodeFlags(EPropertyNodeFlags::Expanded, bOriginalSwappingExpansion);
	SwappingPropertyNode->SetNodeFlags(EPropertyNodeFlags::Expanded, bOriginalSwappableExpansion);

	DetailsView->SaveExpandedItems(SwappablePropertyNode->GetParentNodeSharedPtr().ToSharedRef());
	FScopedTransaction Transaction(NSLOCTEXT("UnrealEd", "MoveRow", "Move Row"));

	SwappingHandle->GetParentHandle()->NotifyPreChange();

	ParentHandle->MoveElementTo(OriginalIndex, NewIndex);

	FPropertyChangedEvent MoveEvent(SwappingHandle->GetParentHandle()->GetProperty(), EPropertyChangeType::Unspecified);
	SwappingHandle->GetParentHandle()->NotifyPostChange(EPropertyChangeType::Unspecified);
	if (DetailsView->GetPropertyUtilities().IsValid())
	{
		DetailsView->GetPropertyUtilities()->NotifyFinishedChangingProperties(MoveEvent);
	}

	return FReply::Handled();
}
*/

/*
TOptional<EItemDropZone> SDetailSingleItemRow::OnArrayCanAcceptDrop(const FDragDropEvent& DragDropEvent, EItemDropZone DropZone, TSharedPtr<FDetailTreeNode> TargetItem)
{
	TSharedPtr<FArrayRowDragDropOp> ArrayDropOp = DragDropEvent.GetOperationAs< FArrayRowDragDropOp >();
	if (!ArrayDropOp.IsValid())
	{
		return TOptional<EItemDropZone>();
	}

	TSharedPtr<SDetailSingleItemRow> RowPtr = ArrayDropOp->Row.Pin();
	if (!RowPtr.IsValid())
	{
		return TOptional<EItemDropZone>();
	}

	// Can't drop onto another array item, so recompute our own drop zone to ensure it's above or below
	const FGeometry& Geometry = GetTickSpaceGeometry();
	const float LocalPointerY = Geometry.AbsoluteToLocal(DragDropEvent.GetScreenSpacePosition()).Y;
	const EItemDropZone OverrideDropZone = LocalPointerY < Geometry.GetLocalSize().Y * 0.5f ? EItemDropZone::AboveItem : EItemDropZone::BelowItem;

	const bool IsValidDrop = CheckValidDrop(RowPtr, OverrideDropZone);

	ArrayDropOp->SetValidTarget(IsValidDrop);

	if (!IsValidDrop)
	{
		return TOptional<EItemDropZone>();
	}

	return OverrideDropZone;
}

FReply SDetailSingleItemRow::OnArrayHeaderAcceptDrop(const FDragDropEvent& DragDropEvent, EItemDropZone DropZone, TSharedPtr< FDetailTreeNode > Type)
{
	OnArrayOrCustomDragLeave(DragDropEvent);
	return FReply::Handled();
}

FReply SDetailSingleItemRow::OnCustomAcceptDrop(const FDragDropEvent& DragDropEvent, EItemDropZone DropZone, TSharedPtr<FDetailTreeNode> TargetItem)
{
	// This should only be registered as a delegate if there's a custom handler
	if (ensure(WidgetRow.CustomDragDropHandler))
	{
		if (WidgetRow.CustomDragDropHandler->AcceptDrop(DragDropEvent, DropZone))
		{
			return FReply::Handled();
		}
	}
	return FReply::Unhandled();
}

TOptional<EItemDropZone> SDetailSingleItemRow::OnCustomCanAcceptDrop(const FDragDropEvent& DragDropEvent, EItemDropZone DropZone, TSharedPtr<FDetailTreeNode> Type)
{
	// Disallow drop between expanded parent item and its first child
	if (DropZone == EItemDropZone::BelowItem && IsItemExpanded())
	{
		return TOptional<EItemDropZone>();
	}

	// This should only be registered as a delegate if there's a custom handler
	if (ensure(WidgetRow.CustomDragDropHandler))
	{
		return WidgetRow.CustomDragDropHandler->CanAcceptDrop(DragDropEvent, DropZone);
	}
	return TOptional<EItemDropZone>();
}
*/
TSharedPtr<FPropertyNode> SDetailSingleItemRow::GetPropertyNode() const
{
	TSharedPtr<FPropertyNode> PropertyNode = Customization->GetPropertyNode();
	if (!PropertyNode.IsValid() && Customization->DetailGroup.IsValid())
	{
		PropertyNode = Customization->DetailGroup->GetHeaderPropertyNode();
	}

	// See if a custom builder has an associated node
	if (!PropertyNode.IsValid() && Customization->HasCustomBuilder())
	{
		TSharedPtr<IPropertyHandle> PropertyHandle = Customization->CustomBuilderRow->GetPropertyHandle();
		if (PropertyHandle.IsValid())
		{
			PropertyNode = StaticCastSharedPtr<FPropertyHandleBase>(PropertyHandle)->GetPropertyNode();
		}
	}

	return PropertyNode;
}

TSharedPtr<IPropertyHandle> SDetailSingleItemRow::GetPropertyHandle() const
{
	TSharedPtr<IPropertyHandle> Handle;
	TSharedPtr<FPropertyNode> PropertyNode = GetPropertyNode();
	if (PropertyNode.IsValid())
	{
		TSharedPtr<FDetailTreeNode> OwnerTreeNodePtr = OwnerTreeNode.Pin();
		if (OwnerTreeNodePtr.IsValid())
		{
			IDetailsViewPrivate* DetailsView = OwnerTreeNodePtr->GetDetailsView();
			if (DetailsView != nullptr)
			{
				Handle = PropertyEditorHelpers::GetPropertyHandle(PropertyNode.ToSharedRef(), DetailsView->GetNotifyHook(), DetailsView->GetPropertyUtilities());
			}
		}
	}
	else if (WidgetRow.PropertyHandles.Num() > 0)
	{
		// @todo: Handle more than 1 property handle?
		Handle = WidgetRow.PropertyHandles[0];
	}

	return Handle;
}

bool SDetailSingleItemRow::UpdateResetToDefault()
{
	TSharedPtr<IPropertyHandle> PropertyHandle = GetPropertyHandle();
	if (PropertyHandle.IsValid())
	{
		if (PropertyHandle->HasMetaData("NoResetToDefault") || PropertyHandle->GetInstanceMetaData("NoResetToDefault"))
		{
			return false;
		}
	}

	if (WidgetRow.CustomResetToDefault.IsSet())
	{
		return WidgetRow.CustomResetToDefault.GetValue().IsResetToDefaultVisible(PropertyHandle);
	}
	else if (PropertyHandle.IsValid())
	{
		return PropertyHandle->CanResetToDefault();
	}

	return false;

}

void SDetailSingleItemRow::Construct( const FArguments& InArgs, FDetailLayoutCustomization* InCustomization, bool bHasMultipleColumns, TSharedRef<FDetailTreeNode> InOwnerTreeNode, const TSharedRef<STableViewBase>& InOwnerTableView )
{
	OwnerTreeNode = InOwnerTreeNode;
	bAllowFavoriteSystem = InArgs._AllowFavoriteSystem;
	Customization = InCustomization;

	TSharedRef<SWidget> Widget = SNullWidget::NullWidget;

	//FOnTableRowDragLeave DragLeaveDelegate;
	FOnAcceptDrop AcceptDropDelegate;
	FOnCanAcceptDrop CanAcceptDropDelegate;

	IDetailsViewPrivate* DetailsView = InOwnerTreeNode->GetDetailsView();
	FDetailColumnSizeData& ColumnSizeData = DetailsView->GetColumnSizeData();

	PulseAnimation.AddCurve(0.0f, DetailWidgetConstants::PulseAnimationLength, ECurveEaseFunction::CubicInOut);

	// Play on construction if animation was started from a behavior the re-constructs this widget
	if (DetailsView->IsNodeAnimating(GetPropertyNode()))
	{
		PulseAnimation.Play(SharedThis(this));
	}

	const bool bIsValidTreeNode = InOwnerTreeNode->GetParentCategory().IsValid() && InOwnerTreeNode->GetParentCategory()->IsParentLayoutValid();
	if (bIsValidTreeNode)
	{
		if (Customization->IsValidCustomization())
		{
			WidgetRow = Customization->GetWidgetRow();

			// Setup copy / paste actions
			{
				if (WidgetRow.IsCopyPasteBound())
				{
					CopyAction = WidgetRow.CopyMenuAction;
					PasteAction = WidgetRow.PasteMenuAction;
				}
				else
				{
					TSharedPtr<FPropertyNode> PropertyNode = GetPropertyNode();
					static const FName DisableCopyPasteMetaDataName("DisableCopyPaste");
					if (PropertyNode.IsValid() && !PropertyNode->ParentOrSelfHasMetaData(DisableCopyPasteMetaDataName))
					{
						CopyAction.ExecuteAction = FExecuteAction::CreateSP(this, &SDetailSingleItemRow::OnCopyProperty);
						PasteAction.ExecuteAction = FExecuteAction::CreateSP(this, &SDetailSingleItemRow::OnPasteProperty);
						PasteAction.CanExecuteAction = FCanExecuteAction::CreateSP(this, &SDetailSingleItemRow::CanPasteProperty);
					}
					else
					{
						CopyAction.ExecuteAction = FExecuteAction::CreateLambda([]() {});
						CopyAction.CanExecuteAction = FCanExecuteAction::CreateLambda([]() { return false; });
						PasteAction.ExecuteAction = FExecuteAction::CreateLambda([]() {});
						PasteAction.CanExecuteAction = FCanExecuteAction::CreateLambda([]() { return false; });
					}
				}
			}

			// Populate the extension content in the WidgetRow if there's an extension handler.
			PopulateExtensionWidget();

			TSharedPtr<SWidget> NameWidget = WidgetRow.NameWidget.Widget;

			TSharedPtr<SWidget> ValueWidget =
				SNew(SConstrainedBox)
				.MinWidth(WidgetRow.ValueWidget.MinWidth)
				.MaxWidth(WidgetRow.ValueWidget.MaxWidth)
				[
					WidgetRow.ValueWidget.Widget
				];

			TSharedPtr<SWidget> ExtensionWidget = WidgetRow.ExtensionWidget.Widget;

			// copies of attributes for lambda captures
			TAttribute<bool> PropertyEnabledAttribute = InOwnerTreeNode->IsPropertyEditingEnabled();
			TAttribute<bool> RowEditConditionAttribute = WidgetRow.EditConditionValue;
			TAttribute<bool> RowIsEnabledAttribute = WidgetRow.IsEnabledAttr;

			TAttribute<bool> IsEnabledAttribute = TAttribute<bool>::CreateLambda(
				[PropertyEnabledAttribute, RowIsEnabledAttribute, RowEditConditionAttribute]()
				{
					return PropertyEnabledAttribute.Get() && RowIsEnabledAttribute.Get(true) && RowEditConditionAttribute.Get(true);
				});

			TAttribute<bool> RowIsValueEnabledAttribute = WidgetRow.IsValueEnabledAttr;
			TAttribute<bool> IsValueEnabledAttribute = TAttribute<bool>::CreateLambda(
				[IsEnabledAttribute, RowIsValueEnabledAttribute]()
				{
					return IsEnabledAttribute.Get() && RowIsValueEnabledAttribute.Get(true);
				});

			NameWidget->SetEnabled(IsEnabledAttribute);
			ValueWidget->SetEnabled(IsValueEnabledAttribute);
			ExtensionWidget->SetEnabled(IsEnabledAttribute);

			TSharedRef<SSplitter> Splitter = SNew(SSplitter)
					.Style(FSodaStyle::Get(), "DetailsView.Splitter")
					.PhysicalSplitterHandleSize(1.0f)
					.HitDetectionSplitterHandleSize(5.0f)
					.HighlightedHandleIndex(ColumnSizeData.HoveredSplitterIndex)
					.OnHandleHovered(ColumnSizeData.OnSplitterHandleHovered);

			Widget = SNew(SBorder)
				.BorderImage(FSodaStyle::Get().GetBrush("DetailsView.CategoryMiddle"))
				.BorderBackgroundColor(this, &SDetailSingleItemRow::GetInnerBackgroundColor)
				.Padding(0)
				[
					Splitter
				];

			// create Name column:
			// | Name | Value | Right |
			TSharedRef<SHorizontalBox> NameColumnBox = SNew(SHorizontalBox)
				.Clipping(EWidgetClipping::OnDemand);

			// indentation and expander arrow
			NameColumnBox->AddSlot()
				.HAlign(HAlign_Left)
				.VAlign(VAlign_Fill)
				.Padding(0)
				.AutoWidth()
				[
					SNew(SDetailRowIndent, SharedThis(this))
				];
			/*
			if (WidgetRow.CustomDragDropHandler)
			{
				TSharedPtr<SDetailSingleItemRow> InRow = SharedThis(this);
				TSharedRef<SWidget> ReorderHandle = PropertyEditorHelpers::MakePropertyReorderHandle(InRow, IsEnabledAttribute);
				
				NameColumnBox->AddSlot()
					.HAlign(HAlign_Left)
					.VAlign(VAlign_Center)
					.Padding(-4, 0, -10, 0)
					.AutoWidth()
					[
						ReorderHandle
					];

				DragLeaveDelegate = FOnTableRowDragLeave::CreateSP(this, &SDetailSingleItemRow::OnArrayOrCustomDragLeave);
				AcceptDropDelegate = FOnAcceptDrop::CreateSP(this, &SDetailSingleItemRow::OnCustomAcceptDrop);
				CanAcceptDropDelegate = FOnCanAcceptDrop::CreateSP(this, &SDetailSingleItemRow::OnCustomCanAcceptDrop);
			}
			else */
			if (TSharedPtr<FPropertyNode> PropertyNode = Customization->GetPropertyNode())
			{
				if (PropertyNode->IsReorderable())
				{
					TSharedPtr<SDetailSingleItemRow> InRow = SharedThis(this);
					TSharedRef<SWidget> ArrayHandle = PropertyEditorHelpers::MakePropertyReorderHandle(InRow, IsEnabledAttribute);

					NameColumnBox->AddSlot()
						.HAlign(HAlign_Left)
						.VAlign(VAlign_Center)
						.Padding(-4, 0, -10, 0)
						.AutoWidth()
						[
							ArrayHandle
						];

					SwappablePropertyNode = PropertyNode;
				}
				
				if (PropertyNode->IsReorderable() || 
					(CastField<FArrayProperty>(PropertyNode->GetProperty()) != nullptr && 
					CastField<FObjectProperty>(CastField<FArrayProperty>(PropertyNode->GetProperty())->Inner) != nullptr)) // Is an object array
				{
					//DragLeaveDelegate = FOnTableRowDragLeave::CreateSP(this, &SDetailSingleItemRow::OnArrayOrCustomDragLeave);
					//AcceptDropDelegate = FOnAcceptDrop::CreateSP(this, PropertyNode->IsReorderable() ? &SDetailSingleItemRow::OnArrayAcceptDrop : &SDetailSingleItemRow::OnArrayHeaderAcceptDrop);
					//CanAcceptDropDelegate = FOnCanAcceptDrop::CreateSP(this, &SDetailSingleItemRow::OnArrayCanAcceptDrop);
				}
			}

			NameColumnBox->AddSlot()
				.HAlign(HAlign_Left)
				.VAlign(VAlign_Center)
				.Padding(2,0,0,0)
				.AutoWidth()
				[
					SNew(SDetailExpanderArrow, SharedThis(this))
				];

			NameColumnBox->AddSlot()
				.VAlign(VAlign_Center)
				.HAlign(HAlign_Left)
				.Padding(2,0,0,0)
				.AutoWidth()
				[
					SNew(SEditConditionWidget)
					.EditConditionValue(WidgetRow.EditConditionValue)
					.OnEditConditionValueChanged(WidgetRow.OnEditConditionValueChanged)
				];

			if (bHasMultipleColumns)
			{
				NameColumnBox->AddSlot()
					.HAlign(WidgetRow.NameWidget.HorizontalAlignment)
					.VAlign(WidgetRow.NameWidget.VerticalAlignment)
					.Padding(2,0,0,0)
					[
						NameWidget.ToSharedRef()
					];

				// create Name column:
				// | Name | Value | Right |
				Splitter->AddSlot()
					.Value(ColumnSizeData.NameColumnWidth)
					.OnSlotResized(ColumnSizeData.OnNameColumnResized)
					[
						NameColumnBox
					];

				// create Value column:
				// | Name | Value | Right |
				Splitter->AddSlot()
					.Value(ColumnSizeData.ValueColumnWidth)
					.OnSlotResized(ColumnSizeData.OnValueColumnResized) 
					[
						SNew(SHorizontalBox)
						.Clipping(EWidgetClipping::OnDemand)
						+ SHorizontalBox::Slot()
						.HAlign(WidgetRow.ValueWidget.HorizontalAlignment)
						.VAlign(WidgetRow.ValueWidget.VerticalAlignment)
						.Padding(DetailWidgetConstants::RightRowPadding)
						[
							ValueWidget.ToSharedRef()
						]
						// extension widget
						+ SHorizontalBox::Slot()
						.HAlign(WidgetRow.ExtensionWidget.HorizontalAlignment)
						.VAlign(WidgetRow.ExtensionWidget.VerticalAlignment)
						.Padding(5,0,0,0)
						.AutoWidth()
						[
							ExtensionWidget.ToSharedRef()
						]
					];
			}
			else
			{
				// create whole row widget, which takes up both the Name and Value columns:
				// | Name | Value | Right |
				NameColumnBox->SetEnabled(IsEnabledAttribute);
				NameColumnBox->AddSlot()
					.HAlign(WidgetRow.WholeRowWidget.HorizontalAlignment)
					.VAlign(WidgetRow.WholeRowWidget.VerticalAlignment)
					.Padding(2,0,0,0)
					[
						WidgetRow.WholeRowWidget.Widget
					];

				Splitter->AddSlot()
					.Value(ColumnSizeData.PropertyColumnWidth)
					.OnSlotResized(ColumnSizeData.OnPropertyColumnResized)
					[
						NameColumnBox
					];
			}

			if(!DetailsView->ShouldHideRightColumn())
			{
				TArray<FPropertyRowExtensionButton> ExtensionButtons;

				FPropertyRowExtensionButton& ResetToDefault = ExtensionButtons.AddDefaulted_GetRef();
				ResetToDefault.Label = NSLOCTEXT("PropertyEditor", "ResetToDefault", "Reset to Default");
				ResetToDefault.UIAction = FUIAction(
					FExecuteAction::CreateSP(this, &SDetailSingleItemRow::OnResetToDefaultClicked),
					FCanExecuteAction::CreateSP(this, &SDetailSingleItemRow::IsResetToDefaultEnabled)
				);

				// We could just collapse the Reset to Default button by setting the FIsActionButtonVisible delegate,
				// but this would cause the reset to defaults not to reserve space in the toolbar and not be aligned across all rows.
				// Instead, we show an empty icon and tooltip and disable the button.
				static FSlateIcon EnabledResetToDefaultIcon(FSodaStyle::Get().GetStyleSetName(), "PropertyWindow.DiffersFromDefault");
				static FSlateIcon DisabledResetToDefaultIcon(FAppStyle::Get().GetStyleSetName(), "NoBrush");
				ResetToDefault.Icon = TAttribute<FSlateIcon>::Create([this]()
				{
					return IsResetToDefaultEnabled() ? 
						EnabledResetToDefaultIcon :
						DisabledResetToDefaultIcon;
				});
				ResetToDefault.ToolTip = TAttribute<FText>::Create([this]() 
				{
					return IsResetToDefaultEnabled() ?
						NSLOCTEXT("PropertyEditor", "ResetToDefaultPropertyValueToolTip", "Reset this property to its default value.") :
						FText::GetEmpty();
				});

				CreateGlobalExtensionWidgets(ExtensionButtons);

				FSlimHorizontalToolBarBuilder ToolbarBuilder(TSharedPtr<FUICommandList>(), FMultiBoxCustomization::None);
				ToolbarBuilder.SetLabelVisibility(EVisibility::Collapsed);
				ToolbarBuilder.SetStyle(&FSodaStyle::Get(), "DetailsView.ExtensionToolBar");
				ToolbarBuilder.SetIsFocusable(false);

				for (const FPropertyRowExtensionButton& Extension : ExtensionButtons)
				{
					ToolbarBuilder.AddToolBarButton(Extension.UIAction, NAME_None, Extension.Label, Extension.ToolTip, Extension.Icon);
				}

				Splitter->AddSlot()
					.Value(ColumnSizeData.RightColumnWidth)
					.OnSlotResized(ColumnSizeData.OnRightColumnResized)
					.MinSize(ColumnSizeData.RightColumnMinWidth)
				[
					SNew(SBorder)
					.BorderImage(FSodaStyle::Get().GetBrush("DetailsView.CategoryMiddle"))
					.BorderBackgroundColor(this, &SDetailSingleItemRow::GetOuterBackgroundColor)
					.HAlign(HAlign_Fill)
					.VAlign(VAlign_Center)
					.Padding(0)
					[
						ToolbarBuilder.MakeWidget()
					]
				];
			}
		}
	}
	else
	{
		// details panel layout became invalid.  This is probably a scenario where a widget is coming into view in the parent tree but some external event previous in the frame has invalidated the contents of the details panel.
		// The next frame update of the details panel will fix it
		Widget = SNew(SSpacer);
	}

	TWeakPtr<STableViewBase> OwnerTableViewWeak = InOwnerTableView;
	auto GetScrollbarWellBrush = [this, OwnerTableViewWeak]()
	{
		return SDetailTableRowBase::IsScrollBarVisible(OwnerTableViewWeak) ?
			FSodaStyle::Get().GetBrush("DetailsView.GridLine") :
			FSodaStyle::Get().GetBrush("DetailsView.CategoryMiddle");
	};

	auto GetScrollbarWellTint = [this, OwnerTableViewWeak]()
	{
		return SDetailTableRowBase::IsScrollBarVisible(OwnerTableViewWeak) ?
			FStyleColors::White : 
			this->GetOuterBackgroundColor();
	};

	auto GetHighlightBorderPadding = [this]()
	{
		return this->IsHighlighted() ? FMargin(1) : FMargin(0);
	};

	this->ChildSlot
	[
		SNew( SBorder )
		.BorderImage(FSodaStyle::Get().GetBrush("DetailsView.GridLine"))
		.Padding(FMargin(0,0,0,1))
		.Clipping(EWidgetClipping::ClipToBounds)
		[
			SNew(SBox)
			.MinDesiredHeight(PropertyEditorConstants::PropertyRowHeight)
			[
				SNew( SHorizontalBox )
				+ SHorizontalBox::Slot()
				.HAlign(HAlign_Fill)
				.VAlign(VAlign_Fill)
				[
					SNew( SBorder )
					.BorderImage(FSodaStyle::Get().GetBrush("DetailsView.Highlight"))
					.Padding_Lambda(GetHighlightBorderPadding)
					[
						SNew( SBorder )
						.BorderImage(FSodaStyle::Get().GetBrush("DetailsView.CategoryMiddle"))
						.BorderBackgroundColor(this, &SDetailSingleItemRow::GetOuterBackgroundColor)
						.Padding(0)
						[
							Widget
						]
					]
				]
				+ SHorizontalBox::Slot()
				.HAlign(HAlign_Right)
				.VAlign(VAlign_Fill)
				.AutoWidth()
				[
					SNew( SBorder )
					.BorderImage_Lambda(GetScrollbarWellBrush)
					.BorderBackgroundColor_Lambda(GetScrollbarWellTint)
					.Padding(FMargin(0, 0, SDetailTableRowBase::ScrollBarPadding, 0))
				]
			]
		]
	];

	STableRow< TSharedPtr< FDetailTreeNode > >::ConstructInternal(
		STableRow::FArguments()
			.Style(FSodaStyle::Get(), "DetailsView.TreeView.TableRow")
			.ShowSelection(false)
			//.OnDragLeave(DragLeaveDelegate)
			.OnAcceptDrop(AcceptDropDelegate)
			.OnCanAcceptDrop(CanAcceptDropDelegate),
		InOwnerTableView
	);
}

FReply SDetailSingleItemRow::OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (MouseEvent.GetModifierKeys().IsShiftDown())
	{
		bool bIsHandled = false;
		if (CopyAction.CanExecute() && MouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
		{
			CopyAction.Execute();
			PulseAnimation.Play(SharedThis(this));
			bIsHandled = true;
		}
		else if (PasteAction.CanExecute() && MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
		{
			PasteAction.Execute();
			PulseAnimation.Play(SharedThis(this));
			if(TSharedPtr<FPropertyNode> PropertyNode = GetPropertyNode())
			{
				// Mark property node as animating so we will animate after any potential re-construction
				IDetailsViewPrivate* DetailsView = OwnerTreeNode.Pin()->GetDetailsView();
				DetailsView->MarkNodeAnimating(PropertyNode, DetailWidgetConstants::PulseAnimationLength);
			}
			bIsHandled = true;
		}

		if (bIsHandled)
		{
			return FReply::Handled();
		}
	}

	return SDetailTableRowBase::OnMouseButtonUp(MyGeometry, MouseEvent);
}

bool SDetailSingleItemRow::IsResetToDefaultEnabled() const
{
	return bCachedResetToDefaultEnabled;
}

void SDetailSingleItemRow::OnResetToDefaultClicked() const
{
	TSharedPtr<IPropertyHandle> PropertyHandle = GetPropertyHandle();
	if (WidgetRow.CustomResetToDefault.IsSet())
	{
		WidgetRow.CustomResetToDefault.GetValue().OnResetToDefaultClicked(PropertyHandle);
	}
	else if (PropertyHandle.IsValid())
	{
		PropertyHandle->ResetToDefault();
	}
}

/** Get the background color of the outer part of the row, which contains the edit condition and extension widgets. */
FSlateColor SDetailSingleItemRow::GetOuterBackgroundColor() const
{
	if (IsHighlighted()/* || DragOperation.IsValid()*/)
	{
		return FAppStyle::Get().GetSlateColor("Colors.Hover");
	}

	return PropertyEditorConstants::GetRowBackgroundColor(0, this->IsHovered());
}

/** Get the background color of the inner part of the row, which contains the name and value widgets. */
FSlateColor SDetailSingleItemRow::GetInnerBackgroundColor() const
{
	FSlateColor Color;

	if (IsHighlighted())
	{
		Color = FAppStyle::Get().GetSlateColor("Colors.Hover");
	}
	else
	{
		const int32 IndentLevel = GetIndentLevelForBackgroundColor();
		Color = PropertyEditorConstants::GetRowBackgroundColor(IndentLevel, this->IsHovered());
	}

	if (PulseAnimation.IsPlaying())
	{
		float Lerp = PulseAnimation.GetLerp();
		return FMath::Lerp(FAppStyle::Get().GetSlateColor("Colors.Hover2").GetSpecifiedColor(), Color.GetSpecifiedColor(), Lerp);
	}

	return Color;
}

bool SDetailSingleItemRow::OnContextMenuOpening(FMenuBuilder& MenuBuilder)
{
	FUIAction CopyDisplayNameAction = FExecuteAction::CreateSP(this, &SDetailSingleItemRow::OnCopyPropertyDisplayName);

	bool bAddedMenuEntry = false;
	if (CopyAction.IsBound() && PasteAction.IsBound())
	{
		// Hide separator line if it only contains the SearchWidget, making the next 2 elements the top of the list
		if (MenuBuilder.GetMultiBox()->GetBlocks().Num() > 1)
		{
			MenuBuilder.AddMenuSeparator();
		}

		bool bLongDisplayName = false;

		FMenuEntryParams CopyContentParams;
		CopyContentParams.LabelOverride = NSLOCTEXT("PropertyView", "CopyProperty", "Copy");
		CopyContentParams.ToolTipOverride = NSLOCTEXT("PropertyView", "CopyProperty_ToolTip", "Copy this property value");
		CopyContentParams.InputBindingOverride = FInputChord(EModifierKey::Shift, EKeys::RightMouseButton).GetInputText(bLongDisplayName);
		CopyContentParams.IconOverride = FSlateIcon(FCoreStyle::Get().GetStyleSetName(), "GenericCommands.Copy");
		CopyContentParams.DirectActions = CopyAction;
		MenuBuilder.AddMenuEntry(CopyContentParams);

		FMenuEntryParams PasteContentParams;
		PasteContentParams.LabelOverride = NSLOCTEXT("PropertyView", "PasteProperty", "Paste");
		PasteContentParams.ToolTipOverride = NSLOCTEXT("PropertyView", "PasteProperty_ToolTip", "Paste the copied value here");
		PasteContentParams.InputBindingOverride = FInputChord(EModifierKey::Shift, EKeys::LeftMouseButton).GetInputText(bLongDisplayName);
		PasteContentParams.IconOverride = FSlateIcon(FCoreStyle::Get().GetStyleSetName(), "GenericCommands.Paste");
		PasteContentParams.DirectActions = PasteAction;
		MenuBuilder.AddMenuEntry(PasteContentParams);

		MenuBuilder.AddMenuEntry(
			NSLOCTEXT("PropertyView", "CopyPropertyDisplayName", "Copy Display Name"),
			NSLOCTEXT("PropertyView", "CopyPropertyDisplayName_ToolTip", "Copy this property display name"),
			FSlateIcon(FCoreStyle::Get().GetStyleSetName(), "GenericCommands.Copy"),
			CopyDisplayNameAction);
	}

	if (OwnerTreeNode.Pin()->GetDetailsView()->IsFavoritingEnabled())
	{
		FUIAction FavoriteAction;
		FavoriteAction.ExecuteAction = FExecuteAction::CreateSP(this, &SDetailSingleItemRow::OnFavoriteMenuToggle);
		FavoriteAction.CanExecuteAction = FCanExecuteAction::CreateSP(this, &SDetailSingleItemRow::CanFavorite);

		FText FavoriteText = NSLOCTEXT("PropertyView", "FavoriteProperty", "Add to Favorites");
		FText FavoriteTooltipText = NSLOCTEXT("PropertyView", "FavoriteProperty_ToolTip", "Add this property to your favorites.");
		FName FavoriteIcon = "DetailsView.PropertyIsFavorite";

		if (IsFavorite())
		{
			FavoriteText = NSLOCTEXT("PropertyView", "RemoveFavoriteProperty", "Remove from Favorites");
			FavoriteTooltipText = NSLOCTEXT("PropertyView", "RemoveFavoriteProperty_ToolTip", "Remove this property from your favorites.");
			FavoriteIcon = "DetailsView.PropertyIsNotFavorite";
		}

		MenuBuilder.AddMenuEntry(
			FavoriteText,
			FavoriteTooltipText,
			FSlateIcon(FSodaStyle::Get().GetStyleSetName(), FavoriteIcon),
			FavoriteAction);
	}

	if (FPropertyEditorPermissionList::Get().ShouldShowMenuEntries())
	{
		// Hide separator line if it only contains the SearchWidget, making the next 2 elements the top of the list
		if (MenuBuilder.GetMultiBox()->GetBlocks().Num() > 1)
		{
			MenuBuilder.AddMenuSeparator();
		}
		
		MenuBuilder.AddMenuEntry(
        	NSLOCTEXT("PropertyView", "CopyRowName", "Copy internal row name"),
        	NSLOCTEXT("PropertyView", "CopyRowName_ToolTip", "Copy the row's parent struct and internal name to use in the property editor's allow/deny lists."),
        	FSlateIcon(),
        	FUIAction(FExecuteAction::CreateSP(this, &SDetailSingleItemRow::CopyRowNameText)));

		MenuBuilder.AddMenuEntry(
			NSLOCTEXT("PropertyView", "AddAllowList", "Add to Allowed"),
			NSLOCTEXT("PropertyView", "AddAllowList_ToolTip", "Add this row to the property editor's allowed properties list."),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateSP(this, &SDetailSingleItemRow::OnToggleAllowList), FCanExecuteAction(),
					  FIsActionChecked::CreateSP(this, &SDetailSingleItemRow::IsAllowListChecked)),
			NAME_None,
			EUserInterfaceActionType::Check);

		MenuBuilder.AddMenuEntry(
			NSLOCTEXT("PropertyView", "AddDenyList", "Add to Denied"),
			NSLOCTEXT("PropertyView", "AddDenyList_ToolTip", "Add this row to the property editor's denied properties list."),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateSP(this, &SDetailSingleItemRow::OnToggleDenyList), FCanExecuteAction(),
					  FIsActionChecked::CreateSP(this, &SDetailSingleItemRow::IsDenyListChecked)),
			NAME_None,
			EUserInterfaceActionType::Check);
			
		bAddedMenuEntry = true;
	}

	if (WidgetRow.CustomMenuItems.Num() > 0)
	{
		// Hide separator line if it only contains the SearchWidget, making the next 2 elements the top of the list
		if (MenuBuilder.GetMultiBox()->GetBlocks().Num() > 1)
		{
			MenuBuilder.AddMenuSeparator();
		}

		for (const FDetailWidgetRow::FCustomMenuData& CustomMenuData : WidgetRow.CustomMenuItems)
		{
			// Add the menu entry
			MenuBuilder.AddMenuEntry(
				CustomMenuData.Name,
				CustomMenuData.Tooltip,
				CustomMenuData.SlateIcon,
				CustomMenuData.Action);
		}
	}

	return true;
}

void SDetailSingleItemRow::OnCopyProperty()
{
	if (OwnerTreeNode.IsValid())
	{
		TSharedPtr<FPropertyNode> PropertyNode = GetPropertyNode();
		if (PropertyNode.IsValid())
		{
			IDetailsViewPrivate* DetailsView = OwnerTreeNode.Pin()->GetDetailsView();
			TSharedPtr<IPropertyHandle> Handle = PropertyEditorHelpers::GetPropertyHandle(PropertyNode.ToSharedRef(), DetailsView->GetNotifyHook(), DetailsView->GetPropertyUtilities());

			FString Value;
			if (Handle->GetValueAsFormattedString(Value, PPF_Copy) == FPropertyAccess::Success)
			{
				FPlatformApplicationMisc::ClipboardCopy(*Value);
				PulseAnimation.Play(SharedThis(this));
			}
		}
	}
}

void SDetailSingleItemRow::OnCopyPropertyDisplayName()
{
	if (OwnerTreeNode.IsValid())
	{
		TSharedPtr<FPropertyNode> PropertyNode = GetPropertyNode();
		if (PropertyNode.IsValid())
		{
			FPlatformApplicationMisc::ClipboardCopy(*PropertyNode->GetDisplayName().ToString());
		}
	}
}

void SDetailSingleItemRow::OnPasteProperty()
{
	FString ClipboardContent;
	FPlatformApplicationMisc::ClipboardPaste(ClipboardContent);

	if (!ClipboardContent.IsEmpty() && OwnerTreeNode.IsValid())
	{
		TSharedPtr<FPropertyNode> PropertyNode = GetPropertyNode();
		if (!PropertyNode.IsValid() && Customization->DetailGroup.IsValid())
		{
			PropertyNode = Customization->DetailGroup->GetHeaderPropertyNode();
		}
		if (PropertyNode.IsValid())
		{
			//FScopedTransaction Transaction(NSLOCTEXT("UnrealEd", "PasteProperty", "Paste Property"));

			IDetailsViewPrivate* DetailsView = OwnerTreeNode.Pin()->GetDetailsView();
			TSharedPtr<IPropertyHandle> Handle = PropertyEditorHelpers::GetPropertyHandle(PropertyNode.ToSharedRef(), DetailsView->GetNotifyHook(), DetailsView->GetPropertyUtilities());

			Handle->SetValueFromFormattedString(ClipboardContent, EPropertyValueSetFlags::InstanceObjects);

			// Need to refresh the details panel in case a property was pasted over another.
			OwnerTreeNode.Pin()->GetDetailsView()->ForceRefresh();

			// Mark property node as animating so we will animate after re-construction
			DetailsView->MarkNodeAnimating(PropertyNode, DetailWidgetConstants::PulseAnimationLength);
		}
	}
}

bool SDetailSingleItemRow::CanPasteProperty() const
{
	// Prevent paste from working if the property's edit condition is not met.
	TSharedPtr<FDetailPropertyRow> PropertyRow = Customization->PropertyRow;
	if (!PropertyRow.IsValid() && Customization->DetailGroup.IsValid())
	{
		PropertyRow = Customization->DetailGroup->GetHeaderPropertyRow();
	}

	if (PropertyRow.IsValid())
	{
		FPropertyEditor* PropertyEditor = PropertyRow->GetPropertyEditor().Get();
		if (PropertyEditor)
		{
			return !PropertyEditor->IsEditConst();
		}
	}

	FString ClipboardContent;
	if (OwnerTreeNode.IsValid())
	{
		FPlatformApplicationMisc::ClipboardPaste(ClipboardContent);
	}

	return !ClipboardContent.IsEmpty();
}

// Helper function to determine which parent Struct a property comes from. If PropertyName is not a real property,
// it will return the passed-in Struct (eg a DetailsCustomization with a custom name is a "fake" property).
const UStruct* GetExactStructForProperty(const UStruct* MostDerivedStruct, const FName PropertyName)
{
	if (FProperty* Property = MostDerivedStruct->FindPropertyByName(PropertyName))
	{
		return Property->GetOwnerStruct();
	}
	return MostDerivedStruct;
}

void SDetailSingleItemRow::CopyRowNameText() const
{
	if (const TSharedPtr<FDetailTreeNode> Owner = OwnerTreeNode.Pin())
	{
		const UStruct* BaseStructure = Owner->GetParentBaseStructure();
		if (BaseStructure)
		{
			const UStruct* ExactStruct = GetExactStructForProperty(Owner->GetParentBaseStructure(), Owner->GetNodeName());
			FPlatformApplicationMisc::ClipboardCopy(*FString::Printf(TEXT("(%s, %s)"), *FSoftObjectPtr(ExactStruct).ToString(), *Owner->GetNodeName().ToString()));
		}
	}
}

void SDetailSingleItemRow::OnToggleAllowList() const
{
	const TSharedPtr<FDetailTreeNode> Owner = OwnerTreeNode.Pin();
	if (Owner)
	{
		const FName OwnerName = "DetailRowContextMenu";
		const UStruct* ExactStruct = GetExactStructForProperty(Owner->GetParentBaseStructure(), Owner->GetNodeName());
		if (IsAllowListChecked())
		{
			FPropertyEditorPermissionList::Get().RemoveFromAllowList(ExactStruct, Owner->GetNodeName(), OwnerName);
		}
		else
		{
			FPropertyEditorPermissionList::Get().AddToAllowList(ExactStruct, Owner->GetNodeName(), OwnerName);
		}
	}
}

bool SDetailSingleItemRow::IsAllowListChecked() const
{
	if (const TSharedPtr<FDetailTreeNode> Owner = OwnerTreeNode.Pin())
	{
		const UStruct* ExactStruct = GetExactStructForProperty(Owner->GetParentBaseStructure(), Owner->GetNodeName());
		return FPropertyEditorPermissionList::Get().IsSpecificPropertyAllowListed(ExactStruct, Owner->GetNodeName());
	}
	return false;
}

void SDetailSingleItemRow::OnToggleDenyList() const
{
	const TSharedPtr<FDetailTreeNode> Owner = OwnerTreeNode.Pin();
	if (Owner)
	{
		const FName OwnerName = "DetailRowContextMenu";
		const UStruct* ExactStruct = GetExactStructForProperty(Owner->GetParentBaseStructure(), Owner->GetNodeName());
		if (IsDenyListChecked())
		{
			FPropertyEditorPermissionList::Get().RemoveFromDenyList(ExactStruct, Owner->GetNodeName(), OwnerName);
		}
		else
		{
			FPropertyEditorPermissionList::Get().AddToDenyList(ExactStruct, Owner->GetNodeName(), OwnerName);
		}
	}
}

bool SDetailSingleItemRow::IsDenyListChecked() const
{
	if (const TSharedPtr<FDetailTreeNode> Owner = OwnerTreeNode.Pin())
    {
    	return FPropertyEditorPermissionList::Get().IsSpecificPropertyDenyListed(Owner->GetParentBaseStructure(), Owner->GetNodeName());
    }
	return false;
}

void SDetailSingleItemRow::PopulateExtensionWidget()
{
	TSharedPtr<FDetailTreeNode> OwnerTreeNodePinned = OwnerTreeNode.Pin();
	if (OwnerTreeNodePinned.IsValid())
	{ 
		IDetailsViewPrivate* DetailsView = OwnerTreeNodePinned->GetDetailsView();
		TSharedPtr<IDetailPropertyExtensionHandler> ExtensionHandler = DetailsView->GetExtensionHandler();
		if (Customization->HasPropertyNode() && ExtensionHandler.IsValid())
		{
			TSharedPtr<IPropertyHandle> Handle = PropertyEditorHelpers::GetPropertyHandle(Customization->GetPropertyNode().ToSharedRef(), nullptr, nullptr);
			const UClass* ObjectClass = Handle->GetOuterBaseClass();
			if (Handle->IsValidHandle() && ObjectClass && ExtensionHandler->IsPropertyExtendable(ObjectClass, *Handle))
			{
				FDetailLayoutBuilderImpl& DetailLayout = OwnerTreeNodePinned->GetParentCategory()->GetParentLayoutImpl();
				ExtensionHandler->ExtendWidgetRow(WidgetRow, DetailLayout, ObjectClass, Handle);
			}
		}
	}
}

bool SDetailSingleItemRow::CanFavorite() const
{
	if (Customization->HasPropertyNode())
	{
		return true;
	}

	if (Customization->HasCustomBuilder())
	{
		TSharedPtr<IPropertyHandle> PropertyHandle = Customization->CustomBuilderRow->GetPropertyHandle();
		const FString& OriginalPath = Customization->CustomBuilderRow->GetOriginalPath();
		
		return PropertyHandle.IsValid() || !OriginalPath.IsEmpty();
	}

	return false;
}

bool SDetailSingleItemRow::IsFavorite() const
{
	if (Customization->HasPropertyNode())
	{
		return Customization->GetPropertyNode()->IsFavorite();
	}

	if (Customization->HasCustomBuilder())
	{
		TSharedPtr<FDetailTreeNode> OwnerTreeNodePinned = OwnerTreeNode.Pin();
		if (OwnerTreeNodePinned.IsValid())
		{
			TSharedPtr<FDetailCategoryImpl> ParentCategory = OwnerTreeNodePinned->GetParentCategory();
			if (ParentCategory.IsValid() && ParentCategory->IsFavoriteCategory())
			{
				return true;
			}

			TSharedPtr<IPropertyHandle> PropertyHandle = Customization->CustomBuilderRow->GetPropertyHandle();
			if (PropertyHandle.IsValid())
			{
				return PropertyHandle->GetPropertyNode()->IsFavorite();
			}

			const FString& OriginalPath = Customization->CustomBuilderRow->GetOriginalPath();
			if (!OriginalPath.IsEmpty())
			{
				return OwnerTreeNodePinned->GetDetailsView()->IsCustomBuilderFavorite(OriginalPath);
			}
		}
	}

	return false;
}

void SDetailSingleItemRow::OnFavoriteMenuToggle()
{
	if (!CanFavorite())
	{
		return; 
	}

	TSharedPtr<FDetailTreeNode> OwnerTreeNodePinned = OwnerTreeNode.Pin();
	if (!OwnerTreeNodePinned.IsValid())
	{
		return;
	}

	IDetailsViewPrivate* DetailsView = OwnerTreeNodePinned->GetDetailsView();

	bool bNewValue = !IsFavorite();
	if (Customization->HasPropertyNode())
	{
		TSharedPtr<FPropertyNode> PropertyNode = Customization->GetPropertyNode();
		PropertyNode->SetFavorite(bNewValue);
	}
	else if (Customization->HasCustomBuilder())
	{
		TSharedPtr<IPropertyHandle> PropertyHandle = Customization->CustomBuilderRow->GetPropertyHandle();
		const FString& OriginalPath = Customization->CustomBuilderRow->GetOriginalPath();

		if (PropertyHandle.IsValid())
		{
			PropertyHandle->GetPropertyNode()->SetFavorite(bNewValue); 
		}
		else if (!OriginalPath.IsEmpty())
		{		
			bNewValue = !DetailsView->IsCustomBuilderFavorite(OriginalPath);
			DetailsView->SetCustomBuilderFavorite(OriginalPath, bNewValue);
		}
	}

	// Calculate the scrolling offset (by item) to make sure the mouse stay over the same property
	int32 ExpandSize = 0;
	if (OwnerTreeNodePinned->ShouldBeExpanded())
	{
		SDetailSingleItemRow_Helper::RecursivelyGetItemShow(OwnerTreeNodePinned.ToSharedRef(), ExpandSize);
	}
	else
	{
		// if the item is not expand count is 1
		ExpandSize = 1;
	}

	// Apply the calculated offset
	DetailsView->MoveScrollOffset(bNewValue ? ExpandSize : -ExpandSize);

	// Refresh the tree
	DetailsView->ForceRefresh();
}

void SDetailSingleItemRow::CreateGlobalExtensionWidgets(TArray<FPropertyRowExtensionButton>& OutExtensions) const
{
	// fetch global extension widgets 
	FRuntimeEditorModule& PropertyEditorModule = FModuleManager::Get().GetModuleChecked<FRuntimeEditorModule>("RuntimeEditor");

	FOnGenerateGlobalRowExtensionArgs Args;
	Args.OwnerTreeNode = OwnerTreeNode;

	if (Customization->HasPropertyNode())
	{
		Args.PropertyHandle = PropertyEditorHelpers::GetPropertyHandle(Customization->GetPropertyNode().ToSharedRef(), nullptr, nullptr);
	}
	else if (WidgetRow.GetPropertyHandles().Num() && WidgetRow.GetPropertyHandles()[0] && WidgetRow.GetPropertyHandles()[0]->IsValidHandle())
	{
		Args.PropertyHandle = WidgetRow.GetPropertyHandles()[0];
	}

	PropertyEditorModule.GetGlobalRowExtensionDelegate().Broadcast(Args, OutExtensions);

	TSharedPtr<FDetailTreeNode> OwnerTreeNodePinned = OwnerTreeNode.Pin();
	if (OwnerTreeNodePinned.IsValid())
	{
		IDetailsViewPrivate* DetailsView = OwnerTreeNodePinned->GetDetailsView();
		DetailsView->GetLocalRowExtensionDelegate().Broadcast(Args, OutExtensions);
	}

	
}

bool SDetailSingleItemRow::IsHighlighted() const
{
	TSharedPtr<FDetailTreeNode> OwnerTreeNodePtr = OwnerTreeNode.Pin();
	return OwnerTreeNodePtr.IsValid() ? OwnerTreeNodePtr->IsHighlighted() : false;
}
/*
TSharedPtr<FDragDropOperation> SDetailSingleItemRow::CreateDragDropOperation()
{
	if (WidgetRow.CustomDragDropHandler)
	{
		TSharedPtr<FDragDropOperation> DragOp = WidgetRow.CustomDragDropHandler->CreateDragDropOperation();
		DragOperation = DragOp;
		return DragOp;
	}
	else
	{
		TSharedPtr<FArrayRowDragDropOp> ArrayDragOp = MakeShareable(new FArrayRowDragDropOp(SharedThis(this)));
		ArrayDragOp->Init();
		DragOperation = ArrayDragOp;
		return ArrayDragOp;
	}
}
*/
void SDetailSingleItemRow::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	bCachedResetToDefaultEnabled = UpdateResetToDefault();
}

void SArrayRowHandle::Construct(const FArguments& InArgs)
{
	ParentRow = InArgs._ParentRow;

	ChildSlot
	[
		InArgs._Content.Widget
	];
}
/*
FReply SArrayRowHandle::OnDragDetected(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (MouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton))
	{
		TSharedPtr<FDragDropOperation> DragDropOp = ParentRow.Pin()->CreateDragDropOperation();
		if (DragDropOp.IsValid())
		{
			return FReply::Handled().BeginDragDrop(DragDropOp.ToSharedRef());
		}
	}

	return FReply::Unhandled();
}

FArrayRowDragDropOp::FArrayRowDragDropOp(TSharedPtr<SDetailSingleItemRow> InRow)
{
	check(InRow.IsValid());
	Row = InRow;
	MouseCursor = EMouseCursor::GrabHandClosed;
}

void FArrayRowDragDropOp::Init()
{
	SetValidTarget(false);
	SetupDefaults();
	Construct();
}

void FArrayRowDragDropOp::SetValidTarget(bool IsValidTarget)
{
	if (IsValidTarget)
	{
		CurrentHoverText = NSLOCTEXT("ArrayDragDrop", "PlaceRowHere", "Place Row Here");
		CurrentIconBrush = FSodaStyle::GetBrush("Graph.ConnectorFeedback.OK");
	}
	else
	{
		CurrentHoverText = NSLOCTEXT("ArrayDragDrop", "CannotPlaceRowHere", "Cannot Place Row Here");
		CurrentIconBrush = FSodaStyle::GetBrush("Graph.ConnectorFeedback.Error");
	}
}
*/

} // namespace soda