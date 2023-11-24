// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "UI/ScenarioAction/ActionNodeRow/SScenarioActionPropertyRow.h"
#include "UI/ScenarioAction/SScenarioActionTree.h"
#include "UI/Common/SActionButton.h"
#include "RuntimeMetaData.h"
#include "RuntimeEditorModule.h"
#include "RuntimePropertyEditor/IStructureDetailsView.h"
#include "Soda/ScenarioAction/ScenarioActionUtils.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"

void SScenarioActionPropertyRow::Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTable, TWeakObjectPtr<UScenarioActionPropertyNode> InNode, const TSharedRef<SScenarioActionTree>& InTree)
{
	check(InNode.IsValid() && InNode->IsNodeValid());
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
				.Image(FSodaStyle::GetBrush(TEXT("Icons.Pillset")))
				.ColorAndOpacity(FLinearColor(1, 1, 1, 0.5))
			]
			+ SHorizontalBox::Slot()
			.VAlign(VAlign_Center)
			.FillWidth(1)
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("Properties")))
			]
			+ SHorizontalBox::Slot()
			.VAlign(VAlign_Center)
			.AutoWidth()
			.Padding(0, 0, 21, 0)
			[
				SNew(SActionButton)
				.Icon(nullptr)
				.ContentPadding(8)
				.ButtonStyle(FSodaStyle::Get(), "MenuWindow.AddButton")
				.MenuContent()
				[
					GenerateAddMenu()
				]
			]			
		],
		InOwnerTable
	);
}

TSharedRef<SWidget> SScenarioActionPropertyRow::GenerateAddMenu()
{
	if (Node.IsValid() && Node->GetParentNode() && Node->GetParentNode())
	{
		UScenarioObjectNode* ParentNode = Node->GetParentNode<UScenarioObjectNode>();
		if (ParentNode && ParentNode->GetObject())
		{
			TMap<FString, TArray<FProperty*>> PropertyMap;
			FScenarioActionUtils::GetScenarioPropertiesByCategory(ParentNode->GetObject()->GetClass(), PropertyMap);
			TArray<FString> Categories;
			PropertyMap.GetKeys(Categories);

			FMenuBuilder MenuBuilder(true, nullptr);

			for (auto& Category : Categories)
			{
				MenuBuilder.BeginSection(NAME_None, FText::FromString(Category));
				TArray<FProperty*>& Properties = PropertyMap[Category];
				for (auto& Propertiy : Properties)
				{
					MenuBuilder.AddMenuEntry(
						FText::FromString(Propertiy->GetName()),
						FText::FromString(Propertiy->GetName()),
						FSlateIcon(),
						FExecuteAction::CreateRaw(this, &SScenarioActionPropertyRow::OnAddPropertyNode, Propertiy));
				}
				MenuBuilder.EndSection();
			}
			return MenuBuilder.MakeWidget();
		}
	}

	return SNew(STextBlock)
		.TextStyle(FAppStyle::Get(), "HintText")
		.Text(FText::FromString(TEXT("Invalid Node")));
}

void SScenarioActionPropertyRow::OnAddPropertyNode(FProperty* Property)
{
	check(Property);
	if (Node.IsValid() && Node->GetParentNode())
	{
		if (UScenarioObjectNode* Parent = Cast<UScenarioObjectNode>(Node->GetParentNode()))
		{
			if (Parent->GetObject())
			{
				Parent->CaptureProperty(Property);
				Node->UpdateChildBodyNode();
				Tree->RebuildNodes();
			}
		}
	}
}

