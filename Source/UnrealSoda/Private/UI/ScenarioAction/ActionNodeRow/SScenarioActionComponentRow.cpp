// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "UI/ScenarioAction/ActionNodeRow/SScenarioActionComponentRow.h"
#include "UI/ScenarioAction/SScenarioActionTree.h"
#include "UI/Common/SActionButton.h"
#include "Soda/ISodaVehicleComponent.h"
#include "SodaStyleSet.h"
#include "Soda/ScenarioAction/ScenarioActionUtils.h"
#include "Soda/ISodaVehicleComponent.h"
#include "Widgets/Input/SButton.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Components/ActorComponent.h"

void SScenarioActionComponentRow::Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTable, TWeakObjectPtr<UScenarioActionComponentNode> InNode, const TSharedRef<SScenarioActionTree>& InTree)
{
	check(InNode.IsValid());
	Node = InNode;
	Tree = InTree;

	FName Icon = TEXT("ClassIcon.Actor");
	if (ISodaVehicleComponent* VehicleComponent = Cast<ISodaVehicleComponent>(Node->GetActorComponent()))
	{
		Icon = VehicleComponent->GetVehicleComponentGUI().IcanName;
	}

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
				.Image(FSodaStyle::GetBrush(Icon))
				.ColorAndOpacity(FLinearColor(1, 1, 1, 0.5))
			]
			+ SHorizontalBox::Slot()
			.VAlign(VAlign_Center)
			.FillWidth(1)
			[
				SNew(STextBlock)
				.Text(FText::FromString(Node->GetActorComponent()->GetName()))
			]
			+ SHorizontalBox::Slot()
			.VAlign(VAlign_Center)
			.AutoWidth()
			.Padding(0, 0, 5, 0)
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
			+ SHorizontalBox::Slot()
			.VAlign(VAlign_Center)
			.AutoWidth()
			[
				SNew(SButton)
				.ButtonStyle(FSodaStyle::Get(), "MenuWindow.DeleteButton")
				.OnClicked(this, &SScenarioActionComponentRow::OnDeleteAction)
			]
		],
		InOwnerTable
	);
}


TSharedRef<SWidget> SScenarioActionComponentRow::GenerateAddMenu()
{
	if (Node.IsValid() && Node->GetActorComponent())
	{
		FMenuBuilder MenuBuilder(true, nullptr);

		TMap<FString, TArray<UFunction*>> FunctionMap;
		FScenarioActionUtils::GetScenarioFunctionsByCategory(Node->GetActorComponent()->GetClass(), FunctionMap);
		TArray<FString> Categories;
		FunctionMap.GetKeys(Categories);

		for (auto& Category : Categories)
		{
			MenuBuilder.BeginSection(NAME_None, FText::FromString(Category));
			TArray<UFunction*>& Functions = FunctionMap[Category];
			for (auto& Function : Functions)
			{
				MenuBuilder.AddMenuEntry(
					FText::FromString(Function->GetName()),
					FText::FromString(Function->GetName()),
					FSlateIcon(),
					FExecuteAction::CreateRaw(this, &SScenarioActionComponentRow::OnAddFunctionNode, Function));
			}
			MenuBuilder.EndSection();
		}

		return MenuBuilder.MakeWidget();
	}
	else
	{
		return SNullWidget::NullWidget;
	}
}

void SScenarioActionComponentRow::OnAddFunctionNode(UFunction* Function)
{
	if (Node.IsValid() && Node->GetActorComponent())
	{
		UScenarioActionFunctionNode* FunctionNode = NewObject<UScenarioActionFunctionNode>(Node.Get());
		check(FunctionNode);
		FunctionNode->InitializeNode(Node->GetActorComponent(), Function);
		Node->AddChildrenNode(FunctionNode);
		Tree->RebuildNodes();
	}
}

void SScenarioActionComponentRow::OnAddPropertyNode(FProperty* Property)
{
	check(Property);
	if (Node.IsValid() && Node->GetActorComponent())
	{
		UScenarioActionPropertyNode* PropertyNode = NewObject<UScenarioActionPropertyNode>(Node.Get());
		check(PropertyNode);
		Node->CaptureProperty(Property);
		PropertyNode->UpdateChildBodyNode();
		Node->AddChildrenNode(PropertyNode);
		Tree->RebuildNodes();
	}
}

FReply SScenarioActionComponentRow::OnDeleteAction()
{
	if (Node.IsValid())
	{
		Node->RemoveThisNode();
		Tree->RebuildNodes();
	}
	return FReply::Handled();
}

