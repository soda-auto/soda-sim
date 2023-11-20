// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "UI/ScenarioAction/ActionNodeRow/SScenarioActionPropertyBodyRow.h"
#include "UI/ScenarioAction/SScenarioActionTree.h"
#include "RuntimeMetaData.h"
#include "RuntimeEditorModule.h"
#include "RuntimePropertyEditor/IStructureDetailsView.h"
#include "RuntimePropertyEditor/PropertyHandle.h"
#include "Modules/ModuleManager.h"

void SScenarioActionPropertyBodyRow::Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTable, TWeakObjectPtr<UScenarioActionPropertyBodyNode> InNode, const TSharedRef<SScenarioActionTree>& InTree)
{
	check(InNode.IsValid() && InNode->IsNodeValid());
	Node = InNode;
	Tree = InTree;

	STableRow<TWeakObjectPtr<UScenarioActionNode>>::FArguments OutArgs;
	OutArgs._SignalSelectionMode = ETableRowSignalSelectionMode::Instantaneous;

	TSharedRef<SWidget> Body = SNullWidget::NullWidget;

	UScenarioObjectNode* ObjectNode = nullptr;
	
	if (Node.IsValid() && Node->GetParentNode() && Node->GetParentNode())
	{
		ObjectNode = Cast<UScenarioObjectNode>(Node->GetParentNode()->GetParentNode());
	}

	if (ObjectNode && ObjectNode->GetCapturedProperties().Num())
	{
		FRuntimeEditorModule& RuntimeEditorModule = FModuleManager::LoadModuleChecked<FRuntimeEditorModule>("RuntimeEditor");
		soda::FDetailsViewArgs Args;
		Args.bHideSelectionTip = true;
		Args.bLockable = false;
		Args.bShowOptions = false;
		Args.bAllowSearch = false;
		Args.NameAreaSettings = soda::FDetailsViewArgs::HideNameArea;
		//Args.bHideRightColumn = true;
		Args.OnGeneratLocalRowExtension.AddLambda([this](const soda::FOnGenerateGlobalRowExtensionArgs& Args, TArray<soda::FPropertyRowExtensionButton>& OutExtensions) 
		{
			soda::FPropertyRowExtensionButton& RemovePreoperty = OutExtensions.AddDefaulted_GetRef();
			RemovePreoperty.Label = FText::FromString(TEXT("Remove Property"));
			RemovePreoperty.ToolTip = FText::FromString(TEXT("Remove Property"));
			RemovePreoperty.Icon = FSlateIcon(FSodaStyle::Get().GetStyleSetName(), "Icons.Delete");
			RemovePreoperty.UIAction = FUIAction(
				FExecuteAction::CreateSP(this, &SScenarioActionPropertyBodyRow::OnDeleteAction, Args.PropertyHandle->GetProperty())
			);
		});

		soda::FStructureDetailsViewArgs StructureArgs;
		TSharedRef<soda::IStructureDetailsView> StructureDetailsView = RuntimeEditorModule.CreateStructureDetailView(Args, StructureArgs, TSharedPtr<FStructOnScope>());
		StructureDetailsView->GetDetailsView()->SetIsPropertyVisibleDelegate(soda::FIsPropertyVisible::CreateLambda([ObjectNode =TWeakObjectPtr<UScenarioObjectNode>(ObjectNode)](const soda::FPropertyAndParent& PropertyAndParent)
		{
			return ObjectNode.IsValid() && (
				PropertyAndParent.ParentProperties.Num() >= 1 || 
				ObjectNode->GetCapturedProperties().Contains(const_cast<FProperty*>(&PropertyAndParent.Property))
			);
		}));
		StructureDetailsView->SetStructureData(ObjectNode->GetStructOnScope());
		Body = StructureDetailsView->GetWidget().ToSharedRef();
	}
	else
	{
		Body = SNew(STextBlock)
			.TextStyle(FAppStyle::Get(), "HintText")
			.Margin(10)
			.Text(FText::FromString(TEXT("Add properties you want to edit")));
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

void SScenarioActionPropertyBodyRow::OnDeleteAction(FProperty* Property)
{
	if (Node.IsValid() && Node->GetParentNode() && Node->GetParentNode()->GetParentNode())
	{
		UScenarioObjectNode* ObjectNode = Node->GetParentNode()->GetParentNodeChecked<UScenarioObjectNode>();
		ObjectNode->GetCapturedProperties().Remove(Property);
		Tree->RebuildNodes();
	}
}