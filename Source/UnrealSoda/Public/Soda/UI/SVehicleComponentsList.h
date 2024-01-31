// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/WeakInterfacePtr.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/STreeView.h"
#include "Templates/SharedPointer.h"
#include "Soda/ISodaVehicleComponent.h"
#include "Templates/SharedPointer.h"
#include "UObject/WeakObjectPtrTemplates.h"

//class UVehicleComponent;
class ASodaVehicle;
class ITableRow;
class STableViewBase;
class SSearchBox;
class SInlineEditableTextBlock;

namespace soda
{
class IComponentsBarTreeNode;

class  SVehicleComponentsList : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SVehicleComponentsList)
	{ }
	SLATE_END_ARGS()

	virtual ~SVehicleComponentsList() override;

	void Construct( const FArguments& InArgs, ASodaVehicle* Vehicle, bool bInteractiveMode);
	void RebuildNodes();
	void SetVehicle(ASodaVehicle* InVehicle) { Vehicle = InVehicle; }
	void RequestSelectRow(ISodaVehicleComponent* VehicleComponent) { RequestedSelecteComponent = VehicleComponent; }
	void RequestEditeTextBox(TWeakPtr<SInlineEditableTextBlock> EditableTextBlock) { RequestedEditeTextBox = EditableTextBlock; }
	const TSharedPtr<STreeView<TSharedRef<IComponentsBarTreeNode>>>& GetTreeView() const { return TreeView; }

protected:
	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;

protected:
	TSharedRef< ITableRow > MakeRowWidget(TSharedRef<IComponentsBarTreeNode> Item, const TSharedRef< STableViewBase >& OwnerTable);
	void OnSelectionChanged(TSharedPtr<IComponentsBarTreeNode> Component, ESelectInfo::Type SelectInfo);
	TSharedPtr<SWidget> OnContextMenuOpening();
	void OnGetChildren(TSharedRef<IComponentsBarTreeNode> InTreeNode, TArray< TSharedRef<IComponentsBarTreeNode> >& OutChildren);
	void OnDoubleClick(TSharedRef<IComponentsBarTreeNode> Component);
	TSharedRef<SWidget> MakeComponentDetailsBar(ISodaVehicleComponent* VehicleComponent);
	UActorComponent* OnComponentClassSelected(TSubclassOf<UActorComponent> Component, UObject* Object);

protected:
	bool bInteractiveMode = false;
	TArray<TSharedRef<IComponentsBarTreeNode>> RootNodes;
	TSharedPtr<STreeView<TSharedRef<IComponentsBarTreeNode>>> TreeView;
	TSharedPtr<SSearchBox> SearchBox;
	TWeakObjectPtr<ASodaVehicle> Vehicle;
	TWeakInterfacePtr<ISodaVehicleComponent> RequestedSelecteComponent;
	TWeakPtr<SInlineEditableTextBlock> RequestedEditeTextBox;
	TWeakInterfacePtr<ISodaVehicleComponent> LastSelectdComponen;
};

} // namespace soda