// Copyright Epic Games, Inc. All Rights Reserved.
// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "RuntimePropertyEditor/DetailTreeNode.h"
#include "RuntimePropertyEditor/IDetailsViewPrivate.h"
#include "InputCoreTypes.h"
#include "Framework/Application/MenuStack.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Commands/UIAction.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Input/Reply.h"
#include "Layout/WidgetPath.h"
#include "Textures/SlateIcon.h"
#include "Widgets/Views/SListView.h"
#include "Widgets/Views/STableViewBase.h"
#include "Widgets/Views/STableRow.h"

namespace soda
{

class SDetailTableRowBase : public STableRow< TSharedPtr< FDetailTreeNode > >
{
public:
	virtual FReply OnMouseButtonUp( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent ) override
	{
		if( OwnerTreeNode.IsValid() && MouseEvent.GetEffectingButton() == EKeys::RightMouseButton && !StaticCastSharedRef<STableViewBase>( OwnerTablePtr.Pin()->AsWidget() )->IsRightClickScrolling() )
		{
			FMenuBuilder MenuBuilder( true, nullptr, nullptr, true );

			FDetailNodeList VisibleChildren;
			OwnerTreeNode.Pin()->GetChildren( VisibleChildren );

			bool bShouldOpenMenu = false;
			// Open context menu if this node can be expanded 
			if( VisibleChildren.Num() )
			{
				bShouldOpenMenu = true;

				FUIAction ExpandAllAction( FExecuteAction::CreateSP( this, &SDetailTableRowBase::OnExpandAllClicked ) );
				FUIAction CollapseAllAction( FExecuteAction::CreateSP( this, &SDetailTableRowBase::OnCollapseAllClicked ) );

				MenuBuilder.BeginSection( NAME_None, NSLOCTEXT("PropertyView", "ExpansionHeading", "Expansion") );
					MenuBuilder.AddMenuEntry( NSLOCTEXT("PropertyView", "CollapseAll", "Collapse All"), NSLOCTEXT("PropertyView", "CollapseAll_ToolTip", "Collapses this item and all children"), FSlateIcon(), CollapseAllAction );
					MenuBuilder.AddMenuEntry( NSLOCTEXT("PropertyView", "ExpandAll", "Expand All"), NSLOCTEXT("PropertyView", "ExpandAll_ToolTip", "Expands this item and all children"), FSlateIcon(), ExpandAllAction );
				MenuBuilder.EndSection();

			}

			bShouldOpenMenu |= OnContextMenuOpening(MenuBuilder);

			if( bShouldOpenMenu )
			{
				FWidgetPath WidgetPath = MouseEvent.GetEventPath() != nullptr ? *MouseEvent.GetEventPath() : FWidgetPath();

				FSlateApplication::Get().PushMenu(AsShared(), WidgetPath, MenuBuilder.MakeWidget(), MouseEvent.GetScreenSpacePosition(), FPopupTransitionEffect::ContextMenu);

				return FReply::Handled();
			}
		}

		return STableRow< TSharedPtr< FDetailTreeNode > >::OnMouseButtonUp( MyGeometry, MouseEvent );
	}

	int32 GetIndentLevelForBackgroundColor() const;

	static bool IsScrollBarVisible(TWeakPtr<STableViewBase> OwnerTableViewWeak);
	static const float ScrollBarPadding;

protected:
	/**
	 * Called when the user opens the context menu on this row
	 *
	 * @param MenuBuilder	The menu builder to add menu items to
	 * @return true if menu items were added
	 */
	virtual bool OnContextMenuOpening( FMenuBuilder& MenuBuilder ) { return false; }

private:
	void OnExpandAllClicked()
	{
		TSharedPtr<FDetailTreeNode> OwnerTreeNodePin = OwnerTreeNode.Pin();
		if( OwnerTreeNodePin.IsValid() )
		{
			const bool bRecursive = true;
			const bool bIsExpanded = true;
			OwnerTreeNodePin->GetDetailsView()->SetNodeExpansionState( OwnerTreeNodePin.ToSharedRef(), bIsExpanded, bRecursive );
		}
	}

	void OnCollapseAllClicked()
	{
		TSharedPtr<FDetailTreeNode> OwnerTreeNodePin = OwnerTreeNode.Pin();
		if( OwnerTreeNodePin.IsValid() )
		{
			const bool bRecursive = true;
			const bool bIsExpanded = false;
			OwnerTreeNodePin->GetDetailsView()->SetNodeExpansionState( OwnerTreeNodePin.ToSharedRef(), bIsExpanded, bRecursive );
		}
	}
protected:
	TWeakPtr<FDetailTreeNode> OwnerTreeNode;
};

} // namespace soda
