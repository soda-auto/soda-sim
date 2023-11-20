// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "SScenarioActionEditor.h"
#include "Soda/UnrealSoda.h"
#include "SodaStyleSet.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Text/SInlineEditableTextBlock.h"
#include "Widgets/Layout/SSplitter.h"
#include "SPositiveActionButton.h"
#include "Kismet/GameplayStatics.h"
#include "Soda/ScenarioAction/ScenarioAction.h"
#include "Soda/ScenarioAction/ScenarioActionBlock.h"
#include "Soda/ScenarioAction/ScenarioActionNode.h"
#include "Soda/ScenarioAction/ScenarioActionCondition.h"
#include "UI/ScenarioAction/SScenarioActionTree.h"
#include "UI/ScenarioAction/SScenarioActionBlock.h"


#define LOCTEXT_NAMESPACE "ScenarioEditor"

class SScenarioActionBlockRow : public STableRow<TWeakObjectPtr<UScenarioActionBlock>>
{
public:
	SLATE_BEGIN_ARGS(SScenarioActionBlockRow)
	{}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTable, TWeakObjectPtr<UScenarioActionBlock> InBlock, const TSharedPtr<SScenarioActionEditor>& InActionEditor)
	{
		check(InActionEditor);
		check(InBlock.IsValid());

		Block = InBlock;
		ActionEditor = InActionEditor;

		STableRow<TWeakObjectPtr<UScenarioActionBlock>>::FArguments Args;
		STableRow<TWeakObjectPtr<UScenarioActionBlock>>::Construct(Args, InOwnerTable);

		ChildSlot
		.Padding(3)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.Padding(FMargin(0, 0, 5, 0))
			.FillWidth(1.0)
			[
				SAssignNew(InlineTextBlock, SInlineEditableTextBlock)
				.Text_Lambda([this]()
				{
					if(Block.IsValid())
					{
						return FText::FromName(Block->GetDisplayName());
					}
					return FText::FromString("None");
				})
				.OnTextCommitted(this, &SScenarioActionBlockRow::OnRenameBlockCommited)
				.IsSelected(this, &SScenarioActionBlockRow::IsSelectedExclusively)

			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SButton)
				.ButtonStyle(FSodaStyle::Get(), "MenuWindow.DeleteButton")
				.OnClicked(this, &SScenarioActionBlockRow::OnDeleteBlock)
			]
		];
	}

	FReply OnDeleteBlock()
	{
		if (ActionEditor.IsValid() && ActionEditor->GetScenarioAction().IsValid() && Block.IsValid())
		{
			ActionEditor->GetScenarioAction()->RemoveBlock(Block.Get());
			ActionEditor->RefreshList();
		}
		return FReply::Handled();
	}

	void OnRenameBlockCommited(const FText& NewName, ETextCommit::Type Type)
	{
		if (ActionEditor.IsValid() && ActionEditor->GetScenarioAction().IsValid() && Block.IsValid())
		{
			Block->SetDisplayName(FName(NewName.ToString()));
			ActionEditor->RefreshList();
		}
	}
protected:
	TWeakObjectPtr<UScenarioActionBlock> Block;
	TSharedPtr<SInlineEditableTextBlock> InlineTextBlock;
	TSharedPtr<SScenarioActionEditor> ActionEditor;
};


void SScenarioActionEditor::Construct( const FArguments& InArgs, TWeakObjectPtr<AScenarioAction> InScenario)
{
	Scenario = InScenario;

	ChildSlot
	[
		SNew(SSplitter)
		.Orientation(EOrientation::Orient_Horizontal)
		+ SSplitter::Slot()
		.Value(0.3)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SBorder)
				.BorderImage(FSodaStyle::GetBrush(TEXT("Brushes.Recessed")))
				[
					SNew(SBorder)
					.HAlign(HAlign_Left)
					.BorderImage(FSodaStyle::GetBrush(TEXT("Brushes.Panel")))
					.Padding(5)
					[
						SNew(SBox)
						.MinDesiredHeight(30)
						.VAlign(VAlign_Center)
						[
							SNew(SPositiveActionButton)
							.Icon(FAppStyle::Get().GetBrush("Icons.Plus"))
							.Text(LOCTEXT("Add", "Add Block"))
							.OnClicked(this, &SScenarioActionEditor::OnAddBlock)
						]
					]
				]
			]
			+ SVerticalBox::Slot()
			.FillHeight(1)
			.Padding(5)
			[
				SAssignNew(ListView, SListView<TWeakObjectPtr<UScenarioActionBlock>>)
				.ListItemsSource(&Source)
				.SelectionMode(ESelectionMode::Single)
				.OnGenerateRow(this, &SScenarioActionEditor::OnGenerateRow)
				.OnSelectionChanged(this, &SScenarioActionEditor::OnSelectionChanged)
			]
		]
		+ SSplitter::Slot()
		.Value(0.7)
		[
			SNew(SBorder)
			.BorderImage(FSodaStyle::GetBrush(TEXT("Brushes.Recessed")))
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Fill)
			[
				SAssignNew(BlockContent, SBox)
			]
		]
	];
	
	RefreshList();

	if (Source.Num())
	{
		ListView->SetSelection(Source.Last());
	}
	else
	{
		ListView->SetSelection(nullptr);
	}
}

void SScenarioActionEditor::RefreshList()
{
	BlockContent->SetContent(SNullWidget::NullWidget);

	Source.Empty();

	for (auto& It : Scenario->ScenarioBlocks)
	{
		if (IsValid(It))
		{
			Source.Add(It);
		}
	}

	ListView->RequestListRefresh();
}

FReply SScenarioActionEditor::OnAddBlock()
{
	if (Scenario.IsValid())
	{
		Scenario->CreateNewBlock()->SetDisplayName(TEXT("New Action Block"));
		RefreshList();
	}
	return FReply::Handled();
}

TSharedRef<ITableRow> SScenarioActionEditor::OnGenerateRow(TWeakObjectPtr<UScenarioActionBlock> Block, const TSharedRef< STableViewBase >& OwnerTable)
{
	return SNew(SScenarioActionBlockRow, OwnerTable, Block, SharedThis(this));
}

void SScenarioActionEditor::OnSelectionChanged(TWeakObjectPtr<UScenarioActionBlock> Block, ESelectInfo::Type SelectInfo)
{
	if (Block.IsValid())
	{
		BlockContent->SetContent(SNew(SScenarioActionBlock, Block.Get(), SharedThis(this)));
	}
	else
	{
		BlockContent->SetContent(
			SNew(SBox)
			.HAlign(HAlign_Center)
			.Padding(0, 80, 0, 0)
			[
				SNew(STextBlock)
				.TextStyle(FAppStyle::Get(), "HintText")
				.Text(FText::FromString(TEXT("Select any action block to start editing")))
			]
		);
	}
}

void SScenarioActionEditor::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);

	if (RequestedEditeTextBox.IsValid())
	{
		RequestedEditeTextBox.Pin()->EnterEditingMode();
		RequestedEditeTextBox.Reset();
	}
}

#undef LOCTEXT_NAMESPACE
