// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "SScenarioActionBlock.h"
#include "Soda/UnrealSoda.h"
#include "SodaStyleSet.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SSplitter.h"
#include "SPositiveActionButton.h"
#include "Kismet/GameplayStatics.h"
#include "Styling/StyleColors.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Framework/Application/SlateApplication.h"
//#include "Soda/ISodaActor.h"
//#include "RuntimeEditorModule.h"
//#include "RuntimePropertyEditor/IDetailsView.h"
//#include "Soda/SodaStatics.h"
//#include "Soda/SodaGameMode.h"
//#include "Soda/Vehicles/SodaVehicle.h"
//#include "Soda/UI/ToolBoxes/Common/SVehicleComponentClassCombo.h"
//#include "Soda/UI/ToolBoxes/Common/SPinWidget.h"
//#include "Soda/UI/SMessageBox.h"
//#include "Soda/SodaGameViewportClient.h"


#include "Soda/ScenarioAction/ScenarioAction.h"
#include "Soda/ScenarioAction/ScenarioActionBlock.h"
#include "Soda/ScenarioAction/ScenarioActionNode.h"
#include "Soda/ScenarioAction/ScenarioActionCondition.h"
#include "Soda/ScenarioAction/ScenarioActionUtils.h"
#include "Soda/ScenarioAction/ScenarioActionEvent.h"

#include "UI/Common/SActionButton.h"
#include "UI/ScenarioAction/SScenarioActionEditor.h"
#include "UI/ScenarioAction/SScenarioActionTree.h"
#include "Soda/UI/SActorList.h"

#include "Widgets/Layout/SExpandableArea.h"
#include "SPositiveActionButton.h"

#include "Soda/SodaGameViewportClient.h"



#define LOCTEXT_NAMESPACE "ScenarioBlock"


void SScenarioActionBlock::Construct(const FArguments& InArgs, UScenarioActionBlock* InBlock, TSharedRef<SScenarioActionEditor> InScenarioEditor)
{
	check(InBlock);
	Block = InBlock;
	ScenarioEditor = InScenarioEditor;

	ChildSlot
	[
		SNew(SScrollBox)
		.Orientation(Orient_Vertical) 
		+ SScrollBox::Slot()
		.HAlign(HAlign_Fill)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.Padding(0, 0, 0, 10)
			.AutoHeight()
			[
				SNew(SBorder)
				.BorderImage(FSodaStyle::GetBrush(TEXT("Brushes.Panel")))
				.HAlign(HAlign_Left)
				.Padding(5)
				[
					SNew(SBox)
					.MinDesiredHeight(30)
					.VAlign(VAlign_Center)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot()
						.VAlign(VAlign_Center)
						[
							SNew(STextBlock)
							.Text(FText::FromString(TEXT("Execution Mode:")))
						]
						+ SHorizontalBox::Slot()
						.VAlign(VAlign_Center)
						[
							SNew(SComboButton)
							.ContentPadding(3)
							.OnGetMenuContent(this, &SScenarioActionBlock::GetComboModeContent)
							.ButtonContent()
							[
								SNew(SHorizontalBox)
								+ SHorizontalBox::Slot()
								.VAlign(VAlign_Center)
								.HAlign(HAlign_Center)
								.Padding(0, 0, 5, 0)
								.AutoWidth()
								[
									SNew(SImage)
									.Image_Lambda([this]()
									{
										if(this->Block.IsValid())
										{
											if(this->Block->GetActionBlockMode() == EScenarioActionBlockMode::SingleExecute)
											{
												return FSodaStyle::Get().GetBrush("Icons.Step");
											}
											else
											{
												return FSodaStyle::Get().GetBrush("Icons.Loop");
											}
										}
										else
										{
											return FSodaStyle::Get().GetBrush("Icons.Error");
										}
									})
								]
								+ SHorizontalBox::Slot()
								.VAlign(VAlign_Center)
								.HAlign(HAlign_Center)
								.Padding(0, 0, 5, 0)
								[
									SNew(STextBlock)
									.MinDesiredWidth(50)
									.Text_Lambda([this]()
									{
										if(this->Block.IsValid())
										{
											if(this->Block->GetActionBlockMode() == EScenarioActionBlockMode::SingleExecute)
											{
												return FText::FromString("Single");
											}
											else
											{
												return FText::FromString("Multiple");
											}
										}
										else
										{
											return FText::FromString("Error");
										}
									})
								]
							]
						]
					]
				]
			]
			+SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SOverlay)
				+ SOverlay::Slot()
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("EVENT")))
					.ColorAndOpacity(FStyleColors::AccentRed)
					.RenderTransform(FTransform2D(TQuat2<float>(-PI / 2), {0, 50}))
				]
				+ SOverlay::Slot()
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(20, 0, 12, 0)
					[
						SNew(SBorder)
						.Padding(2, 2, 2, 90)
						.BorderImage(FSodaStyle::GetBrush(TEXT("ScenarioAction.EventMark")))
					]
					+ SHorizontalBox::Slot()
					.FillWidth(1)
					.VAlign(VAlign_Top)
					[
						SAssignNew(Events, SHorizontalBox)
					]
				]
			]
			+ SVerticalBox::Slot()
			.Padding(0, 12, 0, 12)
			.AutoHeight()
			[
				SNew(SBorder)
				.Padding(2)
				.BorderImage(FSodaStyle::GetBrush(TEXT("ScenarioAction.ConditionalSeparator")))
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SOverlay)
				+ SOverlay::Slot()
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("CONDTN")))
					.ColorAndOpacity(FStyleColors::AccentOrange)
					.RenderTransform(FTransform2D(TQuat2<float>(-PI / 2), {0, 65}))
				]
				+ SOverlay::Slot()
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(20, 0, 12, 0)
					[
						SNew(SBorder)
						.Padding(2, 2, 2, 90)
						.BorderImage(FSodaStyle::GetBrush(TEXT("ScenarioAction.ConditionalMark")))
					]
					+ SHorizontalBox::Slot()
					.FillWidth(1)
					[
						SAssignNew(ConditionRows, SVerticalBox)
					]
				]
			]
			+ SVerticalBox::Slot()
			.Padding(0, 12, 0, 12)
			.AutoHeight()
			[
				SNew(SBorder)
				.Padding(2)
				.BorderImage(FSodaStyle::GetBrush(TEXT("ScenarioAction.ConditionalSeparator")))
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SOverlay)
				+ SOverlay::Slot()
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("ACTION")))
					.ColorAndOpacity(FStyleColors::AccentGreen)
					.RenderTransform(FTransform2D(TQuat2<float>(-PI / 2), {0, 60}))
				]
				+ SOverlay::Slot()
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(20, 0, 12, 0)
					[
						SNew(SBorder)
						.Padding(2,2,2,90)
						.BorderImage(FSodaStyle::GetBrush(TEXT("ScenarioAction.ActionMark")))
					]
					+ SHorizontalBox::Slot()
					.FillWidth(1)
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot()
						.HAlign(HAlign_Left)
						.AutoHeight()
						.Padding(0, 0, 0, 5)
						[
							SNew(SActionButton)
							.ButtonStyle(FSodaStyle::Get(), "ScenarioAction.AddActionButton")
							.Text(FText::FromString("Add"))
							.MenuContent()
							[
								GenerateAddActionMenu()
							]
						]
						+ SVerticalBox::Slot()
						.FillHeight(1.0)
						[
							SAssignNew(ActionTree, SScenarioActionTree, Block)
						]
					]
				]
			]
		]
	];

	RebuildConditionalBlock();
	RebuildEventsBlock();
}

void SScenarioActionBlock::RebuildEventsBlock()
{
	Events->ClearChildren();

	for (int i = 0; i < Block->Events.Num(); ++i)
	{
		Events->AddSlot()
		.AutoWidth()
		[
			SNew(SBox)
			.MinDesiredWidth(200)
			.MinDesiredHeight(100)
			[
				SNew(SBorder)
				.BorderImage(FSodaStyle::GetBrush("ScenarioAction.EventNode"))
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					.Padding(4)
					.AutoHeight()
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot()
						.FillWidth(1)
						[
							SNew(STextBlock)
							.Text(Block->Events[i]->GetDisplayName())
						]
						+ SHorizontalBox::Slot()
						.AutoWidth()
						[
							SNew(SButton)
							.ButtonStyle(FSodaStyle::Get(), "MenuWindow.DeleteButton")
							.OnClicked(this, &SScenarioActionBlock::OnDeleteEvent, Block->Events[i])
						]
					]
					+ SVerticalBox::Slot()
					.Padding(4)
					.FillHeight(1)
					.VAlign(VAlign_Top)
					[
						Block->Events[i]->MakeWidget()
					]
				]
			]
		];

		// OR seporator
		Events->AddSlot()
		.AutoWidth()
		[
			SNew(SBox)
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			.Padding(4)
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("or")))
			]
		];
	}

	// Add ADD button
	Events->AddSlot()
	.AutoWidth()
	[
		SNew(SActionButton)
		.Text(FText::FromString(TEXT("Add")))
		.IconColor(FStyleColors::AccentRed)
		.ButtonStyle(&FSodaStyle::Get().GetWidgetStyle<FButtonStyle>("ScenarioAction.AddEventButton"))
		.MenuContent()
		[
			GenerateAddEventMenu()
		]
	];
}

void SScenarioActionBlock::RebuildConditionalBlock()
{
	ConditionRows->ClearChildren();

	for (int i = 0; i < Block->ConditionMatrix.Num() - 1; ++i)
	{
		if (Block->ConditionMatrix[i].ScenarioConditiones.Num() == 0)
		{
			Block->ConditionMatrix.RemoveAt(i);
			--i;
		}
	}

	for (int i = 0; i < Block->ConditionMatrix.Num(); ++i)
	{
		auto& Row = Block->ConditionMatrix[i];
		TSharedPtr<SHorizontalBox> ConditionColls;
		ConditionRows->AddSlot()
		.AutoHeight()
		[
			SNew(SScrollBox)
			.Orientation(Orient_Horizontal)
			+ SScrollBox::Slot()
			//.Padding(8)
			[
				SNew(SBox)
				.MinDesiredHeight(100)
				[
					SAssignNew(ConditionColls, SHorizontalBox)
				]
			]
		];

		for (auto & Col : Row.ScenarioConditiones)
		{
			ConditionColls->AddSlot()
			.AutoWidth()
			[
				SNew(SBox)
				.MinDesiredWidth(200)
				.MinDesiredHeight(100)
				[
					SNew(SBorder)
					.BorderImage(FSodaStyle::GetBrush("ScenarioAction.ConditionalNode"))
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot()
						.Padding(4)
						.AutoHeight()
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot()
							.FillWidth(1)
							[
								SNew(STextBlock)
								.Text(Col->GetDisplayName())
							]
							+ SHorizontalBox::Slot()
							.AutoWidth()
							[
								SNew(SButton)
								.ButtonStyle(FSodaStyle::Get(), "MenuWindow.DeleteButton")
								.OnClicked(this, &SScenarioActionBlock::OnDeleteConditional, Col, i)
							]
						]
						+ SVerticalBox::Slot()
						.Padding(4)
						.FillHeight(1)
						.VAlign(VAlign_Top)
						[
							Col->MakeWidget()
						]
					]
				]
			];

			// AND seporator
			ConditionColls->AddSlot()
			.AutoWidth()
			[
				SNew(SBox)
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				.Padding(4)
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("&")))
				]
			];
			
		}

		// Button "Add AND conditional block"
		ConditionColls->AddSlot()
		.AutoWidth()
		[
			SNew(SActionButton)
			.ButtonStyle(&FSodaStyle::Get().GetWidgetStyle<FButtonStyle>("ScenarioAction.AddConditionalButton"))
			.IconColor(FStyleColors::AccentOrange)
			.Text(FText::FromString(TEXT("Add")))
			.MenuContent()
			[
				GenerateAddConditionalMenu(i)
			]
		];

		// OR seporator
		ConditionRows->AddSlot()
		.Padding(0, 4, 0, 4)
		.AutoHeight()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.VAlign(VAlign_Center)
			.FillWidth(0.2)
			[
				SNew(SBorder)
				.Padding(0, 1, 0, 0)
				.BorderImage(FSodaStyle::GetBrush(TEXT("ScenarioAction.ConditionalSeparator")))
			]
			+ SHorizontalBox::Slot()
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			.Padding(10, 0, 10, 0)
			.AutoWidth()
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("or")))
			]
			+ SHorizontalBox::Slot()
			.VAlign(VAlign_Center)
			.FillWidth(0.2)
			[
				SNew(SBorder)
				.Padding(0, 1, 0, 0)
				.BorderImage(FSodaStyle::GetBrush(TEXT("ScenarioAction.ConditionalSeparator")))
			]
			+ SHorizontalBox::Slot()
			.FillWidth(0.5)
		];
	}

	// Button "Add OR conditional block"
	ConditionRows->AddSlot()
	.AutoHeight()
	[
		SNew(SBox)
		.HAlign(HAlign_Left)
		[
			SNew(SActionButton)
			.ButtonStyle(&FSodaStyle::Get().GetWidgetStyle<FButtonStyle>("ScenarioAction.AddConditionalButton"))
			.IconColor(FStyleColors::AccentOrange)
			.Text(FText::FromString(TEXT("Add")))
			.MenuContent()
			[
				GenerateAddConditionalMenu(-1)
			]
		]
	];
}

void SScenarioActionBlock::OnAddConditional(UFunction* Function, int RowIndex)
{
	if (Block.IsValid() && (RowIndex < 0 || (RowIndex >= 0 && Block->ConditionMatrix.IsValidIndex(RowIndex))))
	{
		UScenarioActionConditionFunction* ScenarioCondition = NewObject<UScenarioActionConditionFunction>(Block.Get());
		check(ScenarioCondition);
		ScenarioCondition->SetFunction(Function, Function->GetOuterUClassUnchecked());

		if (RowIndex >= 0)
		{
			Block->ConditionMatrix[RowIndex].ScenarioConditiones.Add(ScenarioCondition);
		}
		else
		{
			Block->ConditionMatrix.Add_GetRef({}).ScenarioConditiones.Add(ScenarioCondition);
		}
		RebuildConditionalBlock();
	}
}

FReply SScenarioActionBlock::OnDeleteConditional(UScenarioActionCondition* Condition, int RowIndex)
{
	if (IsValid(Condition) && Block.IsValid() && Block->ConditionMatrix.IsValidIndex(RowIndex))
	{
		Block->ConditionMatrix[RowIndex].ScenarioConditiones.Remove(Condition);
		

		if (Block->ConditionMatrix[RowIndex].ScenarioConditiones.Num() == 0)
		{
			Block->ConditionMatrix.RemoveAt(RowIndex);
		}

		RebuildConditionalBlock();
	}
	return FReply::Handled();
}

FReply SScenarioActionBlock::OnDeleteEvent(UScenarioActionEvent* Event)
{
	if (Block.IsValid())
	{
		Block->Events.Remove(Event);
		RebuildEventsBlock();
	}
	return FReply::Handled();
}

TSharedRef<SWidget> SScenarioActionBlock::GenerateAddEventMenu()
{
	if (Block.IsValid())
	{
		TMap<FString, TArray<UClass*>> EventMap;
		FScenarioActionUtils::GetScenarioActionEvents(EventMap);
		TArray<FString> Categories;
		EventMap.GetKeys(Categories);

		FMenuBuilder MenuBuilder(true, nullptr);

		for (auto& Category : Categories)
		{
			MenuBuilder.BeginSection(NAME_None, FText::FromString(Category));
			for (auto& Event : EventMap[Category])
			{
				MenuBuilder.AddMenuEntry(
					FText::FromString(Event->GetName()),
					FText::FromString(Event->GetName()),
					FSlateIcon(),
					FExecuteAction::CreateRaw(this, &SScenarioActionBlock::OnAddEvent, Event));
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

TSharedRef<SWidget> SScenarioActionBlock::GenerateAddConditionalMenu(int RowIndex)
{
	if (Block.IsValid())
	{
		TMap<FString, TArray<UFunction*>> FunctionMap;
		FScenarioActionUtils::GetScenarioConditionFunctionsByCategory(FunctionMap);
		TArray<FString> Categories;
		FunctionMap.GetKeys(Categories);

		FMenuBuilder MenuBuilder(true, nullptr);
		
		for (auto& Category : Categories)
		{
			MenuBuilder.BeginSection(NAME_None, FText::FromString(Category));
			for (auto& Function : FunctionMap[Category])
			{
				MenuBuilder.AddMenuEntry(
					FText::FromString(Function->GetName()),
					FText::FromString(Function->GetName()),
					FSlateIcon(),
					FExecuteAction::CreateRaw(this, &SScenarioActionBlock::OnAddConditional, Function, RowIndex));
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

TSharedRef<SWidget> SScenarioActionBlock::GenerateAddActionMenu()
{
	FMenuBuilder MenuBuilder(true, nullptr);

	MenuBuilder.BeginSection(NAME_None, FText::FromString("Actors"));

	MenuBuilder.AddWrapperSubMenu(
		FText::FromString(TEXT("Actors")), 
		FText::GetEmpty(), 
		FOnGetContent::CreateLambda([this]() 
		{
			return 
			SNew(SBorder)
			.BorderImage(FSodaStyle::GetBrush(TEXT("Brushes.Header")))
			.Padding(1)
			[
				SNew(SBox)
				.MinDesiredWidth(200)
				.MinDesiredHeight(200)
				[
					SNew(soda::SActorList, Cast<USodaGameViewportClient>(Block->GetWorld()->GetGameViewport()))
					.bInteractiveMode(false)
					.OnSelectionChanged(this, &SScenarioActionBlock::OnAddActorActionNode)
				]
			];
		}),
		FSlateIcon(FSodaStyle::Get().GetStyleSetName(), "ClassIcon.Actor")
	);

	MenuBuilder.EndSection();

	TMap<FString, TArray<UFunction*>> ActionMap;
	FScenarioActionUtils::GetScenarioActionFunctionsByCategory(ActionMap);
	TArray<FString> Categories;
	ActionMap.GetKeys(Categories);

	for (auto& Category : Categories)
	{
		MenuBuilder.BeginSection(NAME_None, FText::FromString(Category));
		for (auto& Action : ActionMap[Category])
		{
			MenuBuilder.AddMenuEntry(
				FText::FromString(Action->GetName()),
				FText::FromString(Action->GetName()),
				FSlateIcon(),
				FExecuteAction::CreateRaw(this, &SScenarioActionBlock::OnAddFunctionActionNode, Action));
		}
		MenuBuilder.EndSection();
	}

	return MenuBuilder.MakeWidget();
}

void SScenarioActionBlock::OnAddActorActionNode(TWeakObjectPtr<AActor> Actor, ESelectInfo::Type SelectInfo)
{
	if (Actor.IsValid() && Block.IsValid())
	{
		UScenarioActionActorNode* Node = NewObject<UScenarioActionActorNode>(Block.Get());
		check(Node);
		Node->InitializeNode(Actor.Get());
		Block->ActionRootNodes.Add(Node);
		ActionTree->RebuildNodes();
	}
	FSlateApplication::Get().DismissAllMenus();
}

void SScenarioActionBlock::OnAddFunctionActionNode(UFunction* Function)
{
	check(Function);
	if (Block.IsValid())
	{
		UScenarioActionFunctionNode* Node = NewObject<UScenarioActionFunctionNode>(Block.Get());
		check(Node);
		Node->InitializeNode(Function->GetOwnerClass(), Function);
		Block->ActionRootNodes.Add(Node);
		ActionTree->RebuildNodes();
	}
}

void SScenarioActionBlock::OnAddEvent(UClass* Class)
{
	if (Block.IsValid() && IsValid(Class))
	{
		UScenarioActionEvent* Event = NewObject<UScenarioActionEvent>(Block.Get(), Class);
		check(Event);
		Block->Events.Add(Event);
		RebuildEventsBlock();
	}
}


TSharedRef<SWidget> SScenarioActionBlock::GetComboModeContent()
{
	FMenuBuilder MenuBuilder(
		/*bShouldCloseWindowAfterMenuSelection*/ true, 
		/* InCommandList = */nullptr, 
		/* InExtender = */nullptr, 
		/*bCloseSelfOnly*/ false,
		&FSodaStyle::Get());

	MenuBuilder.BeginSection("Section");

	{
		FUIAction Action;
		Action.ExecuteAction.BindLambda([this]() 
		{
			Block->SetActionBlockMode(EScenarioActionBlockMode::SingleExecute);
		});
		MenuBuilder.AddMenuEntry(
			FText::FromString(TEXT("Single")),
			FText::FromString(TEXT("Single")),
			FSlateIcon(FSodaStyle::Get().GetStyleSetName(), "Icons.Step"),
			Action);
	}

	{
		FUIAction Action;
		Action.ExecuteAction.BindLambda([this]() 
		{
			Block->SetActionBlockMode(EScenarioActionBlockMode::MultipleExecute);
		});
		MenuBuilder.AddMenuEntry(
			FText::FromString(TEXT("Multiple")),
			FText::FromString(TEXT("Multiple")),
			FSlateIcon(FSodaStyle::Get().GetStyleSetName(), "Icons.Loop"),
			Action);
	}

	MenuBuilder.EndSection();

	return MenuBuilder.MakeWidget();
}


#undef LOCTEXT_NAMESPACE
