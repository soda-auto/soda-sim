// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "UI/ScenarioAction/ActionNodeRow/SScenarioActionFunctionRow.h"
#include "UI/ScenarioAction/SScenarioActionTree.h"
#include "SodaStyleSet.h"
#include "Widgets/Input/SButton.h"
#include "UObject/StructOnScope.h"

void SScenarioActionFunctionRow::Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTable, TWeakObjectPtr<UScenarioActionFunctionNode> InNode, const TSharedRef<SScenarioActionTree>& InTree)
{
	check(InNode.IsValid());
	Node = InNode;
	Tree = InTree;
	STableRow<TWeakObjectPtr<UScenarioActionNode>>::FArguments OutArgs;
	OutArgs._SignalSelectionMode = ETableRowSignalSelectionMode::Instantaneous;
	STableRow<TWeakObjectPtr<UScenarioActionNode>>::Construct(
		OutArgs
		.Style(&FSodaStyle::Get().GetWidgetStyle<FTableRowStyle>(TEXT("ScenarioAction.ActionRowDefault")))
		.Padding(2)
		.Content()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(1, 1, 3, 1)
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Center)
			[
				SNew(SImage)
				.Image(FSodaStyle::GetBrush(TEXT("Icons.Function")))
				.ColorAndOpacity(FLinearColor(1, 1, 1, 0.5))
			]
			+ SHorizontalBox::Slot()
			.VAlign(VAlign_Center)
			.FillWidth(1)
			[
				SNew(STextBlock)
				.Text(FText::FromString(Node->GetStructOnScope()->GetStruct()->GetName()))
			]
			+ SHorizontalBox::Slot()
			.VAlign(VAlign_Center)
			.AutoWidth()
			[
				SNew(SButton)
				.ButtonStyle(FSodaStyle::Get(), "MenuWindow.DeleteButton")
				.OnClicked(this, &SScenarioActionFunctionRow::OnDeleteAction)
			]
		],
		InOwnerTable
	);
}

FReply SScenarioActionFunctionRow::OnDeleteAction()
{
	if (Node.IsValid())
	{
		Node->RemoveThisNode();
		Tree->RebuildNodes();
	}
	return FReply::Handled();
}
