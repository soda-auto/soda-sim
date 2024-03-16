// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "UI/ScenarioAction/ActionNodeRow/SScenarioActionActorRow.h"
#include "UI/ScenarioAction/SScenarioActionTree.h"
#include "UI/Common/SActionButton.h"
#include "SodaStyleSet.h"
#include "Soda/ScenarioAction/ScenarioActionUtils.h"
#include "Soda/ISodaActor.h" 
#include "Soda/SodaGameMode.h"
#include "Soda/ISodaVehicleComponent.h"
#include "Soda/Vehicles/SodaVehicle.h"
#include "Widgets/Input/SButton.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"

void SScenarioActionActorRow::Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTable, TWeakObjectPtr<UScenarioActionActorNode> InNode, const TSharedRef<SScenarioActionTree>& InTree)
{
	check(InNode.IsValid());
	Node = InNode;
	Tree = InTree;

	USodaGameModeComponent* GameMode = USodaGameModeComponent::Get();
	check(GameMode);

	FName Icon = TEXT("ClassIcon.Actor");
	if (ISodaActor* SodaActor = Cast<ISodaActor>(Node->GetActor()))
	{
		const FSodaActorDescriptor& Desc = GameMode->GetSodaActorDescriptor(Node->GetActor()->GetClass());
		Icon = Desc.Icon;
	}

	STableRow<TWeakObjectPtr<UScenarioActionNode>>::FArguments OutArgs;
	OutArgs._SignalSelectionMode = ETableRowSignalSelectionMode::Instantaneous;
	STableRow<TWeakObjectPtr<UScenarioActionNode>>::Construct(OutArgs
		.Padding(2)
		.Style(&FSodaStyle::Get().GetWidgetStyle<FTableRowStyle>(TEXT("ScenarioAction.ActionRowDefault")))
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
				.Text(FText::FromString(Node->GetActor()->GetName()))
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
				.OnClicked(this, &SScenarioActionActorRow::OnDeleteAction)
			]
		], 
		InOwnerTable);
}

TSharedRef<SWidget> SScenarioActionActorRow::GenerateAddMenu()
{
	if (Node.IsValid() && Node->GetActor())
	{
		FMenuBuilder MenuBuilder(true, nullptr);

		if (Cast<ASodaVehicle>(Node->GetActor()))
		{
			MenuBuilder.BeginSection(NAME_None, FText::FromString(TEXT("Vehicle Components")));
			MenuBuilder.AddSubMenu(
				FText::FromString("Components"),
				FText::FromString("Components"),
				FNewMenuDelegate::CreateRaw(this, &SScenarioActionActorRow::GenerateAddComponentMenu),
				false,
				FSlateIcon(FSodaStyle::Get().GetStyleSetName(), "SodaViewport.SubMenu.Stats")
			);
			MenuBuilder.EndSection();
		}

		TMap<FString, TArray<UFunction*>> FunctionMap;
		FScenarioActionUtils::GetScenarioFunctionsByCategory(Node->GetActor()->GetClass(), FunctionMap);
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
					FExecuteAction::CreateRaw(this, &SScenarioActionActorRow::OnAddFunctionNode, Function));
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

void SScenarioActionActorRow::GenerateAddComponentMenu(FMenuBuilder& MenuBuilder)
{
	if (Node.IsValid() && Node->GetActor())
	{
		TMap<FString, TArray<UActorComponent*>> ComponentMap;
		FScenarioActionUtils::GetComponentsByCategory(Node->GetActor(), ComponentMap);
		TArray<FString> Categories;
		ComponentMap.GetKeys(Categories);
		
		for (auto& Category : Categories)
		{
			MenuBuilder.BeginSection(NAME_None, FText::FromString(Category));
			TArray<UActorComponent*>& Components = ComponentMap[Category];
			for (auto& Component : Components)
			{
				ISodaVehicleComponent* VehicleComponent = Cast<ISodaVehicleComponent>(Component);
				check(VehicleComponent);

				MenuBuilder.AddMenuEntry(
					FText::FromString(Component->GetName()),
					FText::FromString(Component->GetName()),
					FSlateIcon(FSodaStyle::Get().GetStyleSetName(), VehicleComponent->GetVehicleComponentGUI().IcanName),
					FExecuteAction::CreateRaw(this, &SScenarioActionActorRow::OnAddComponentNode, Component));
			}
			MenuBuilder.EndSection();
		}
	}
}

void SScenarioActionActorRow::GenerateAddFunctionsMenu(FMenuBuilder& MenuBuilder)
{

}

void SScenarioActionActorRow::OnAddComponentNode(UActorComponent * Component)
{
	if (Node.IsValid() && Node->GetActor())
	{
		UScenarioActionComponentNode* ComponentNode = NewObject<UScenarioActionComponentNode>(Node.Get());
		check(ComponentNode);
		ComponentNode->InitializeNode(Component);
		Node->AddChildrenNode(ComponentNode);
		Tree->RebuildNodes();
	}
}

void SScenarioActionActorRow::OnAddFunctionNode(UFunction* Function)
{
	if (Node.IsValid() && Node->GetActor())
	{
		UScenarioActionFunctionNode* FunctionNode = NewObject<UScenarioActionFunctionNode>(Node.Get());
		check(FunctionNode);
		FunctionNode->InitializeNode(Node->GetActor(), Function);
		Node->AddChildrenNode(FunctionNode);
		Tree->RebuildNodes();
	}
}

void SScenarioActionActorRow::OnAddPropertyNode(FProperty * Property)
{
	check(Property);
	if (Node.IsValid() && Node->GetActor())
	{
		UScenarioActionPropertyNode* PropertyNode = NewObject<UScenarioActionPropertyNode>(Node.Get());
		check(PropertyNode);
		Node->CaptureProperty(Property);
		PropertyNode->UpdateChildBodyNode();
		Node->AddChildrenNode(PropertyNode);
		Tree->RebuildNodes();
	}
}

FReply SScenarioActionActorRow::OnDeleteAction()
{
	if (Node.IsValid())
	{
		Node->RemoveThisNode();
		Tree->RebuildNodes();
	}
	return FReply::Handled();
}