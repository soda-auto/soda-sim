// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "UI/ScenarioAction/ActionNodeRow/SScenarioActionFunctionBodyRow.h"
#include "RuntimeMetaData.h"
#include "RuntimeEditorModule.h"
#include "RuntimePropertyEditor/IStructureDetailsView.h"
#include "Styling/StyleColors.h"
#include "Modules/ModuleManager.h"

void SScenarioActionFunctionBodyRow::Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTable, TWeakObjectPtr<UScenarioActionFunctionBodyNode> InNode, const TSharedRef<SScenarioActionTree>& InTree)
{
	check(InNode.IsValid());
	Node = InNode;
	Tree = InTree;

	STableRow<TWeakObjectPtr<UScenarioActionNode>>::FArguments OutArgs;
	OutArgs._SignalSelectionMode = ETableRowSignalSelectionMode::Instantaneous;

	TSharedRef<SWidget> Body = SNullWidget::NullWidget;
	if (Node->GetParentNode() && Node->GetParentNode()->IsNodeValid())
	{
		FRuntimeEditorModule& RuntimeEditorModule = FModuleManager::LoadModuleChecked<FRuntimeEditorModule>("RuntimeEditor");
		soda::FDetailsViewArgs Args;
		Args.bHideSelectionTip = true;
		Args.bLockable = false;
		Args.bShowOptions = false;
		Args.bAllowSearch = false;
		Args.NameAreaSettings = soda::FDetailsViewArgs::HideNameArea;
		Args.bHideRightColumn = true;
		soda::FStructureDetailsViewArgs StructureArgs;
		Body = RuntimeEditorModule.CreateStructureDetailView(Args, StructureArgs, Node->GetParentNodeChecked<UScenarioActionFunctionNode>()->GetStructOnScope())->GetWidget().ToSharedRef();
	}

	STableRow<TWeakObjectPtr<UScenarioActionNode>>::Construct(
		OutArgs
		.Style(&FSodaStyle::Get().GetWidgetStyle<FTableRowStyle>(TEXT("ScenarioAction.ActionRowContent")))
		.Padding(4)
		.Content()
		[
			SNew(SBox)
			.HAlign(HAlign_Left)
			[
				SNew(SBox)
				.MinDesiredWidth(350)
				[
					SNew(SBorder)
					.Padding(2, 8, 2, 8)
					.BorderImage(FSodaStyle::GetBrush("ScenarioAction.ActionNode"))
					[
						Body
					]
				]
			]
		], 
		InOwnerTable
	);
}

