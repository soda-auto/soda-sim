// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "UObject/WeakInterfacePtr.h"
#include "Templates/SharedPointer.h"
#include "Widgets/Views/SListView.h"
#include "Templates/SharedPointer.h"

class AScenarioAction;
class UScenarioActionBlock;
class ITableRow;
class STableViewBase;
class SInlineEditableTextBlock;

class  SScenarioActionEditor : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SScenarioActionEditor)
	{ }
	SLATE_END_ARGS()

	void Construct( const FArguments& InArgs, TWeakObjectPtr<AScenarioAction> Scenario);
	void RefreshList();

	const TWeakObjectPtr<AScenarioAction>& GetScenarioAction() const { return Scenario; }

	void SetRequestedEditeTextBox(TWeakPtr<SInlineEditableTextBlock> InRequestedEditeTextBox) { RequestedEditeTextBox = InRequestedEditeTextBox; }

protected:
	virtual void Tick(const FGeometry& AllottedGeometry, const double CurrentTime, const float DeltaTime) override;

	FReply OnAddBlock();
	TSharedRef<ITableRow> OnGenerateRow(TWeakObjectPtr<UScenarioActionBlock> Block, const TSharedRef< STableViewBase >& OwnerTable);
	void OnSelectionChanged(TWeakObjectPtr<UScenarioActionBlock> Block, ESelectInfo::Type SelectInfo);

protected:
	TWeakObjectPtr<AScenarioAction> Scenario;
	TSharedPtr<SBox> BlockContent;
	TSharedPtr<SListView<TWeakObjectPtr<UScenarioActionBlock>>> ListView;
	TArray<TWeakObjectPtr<UScenarioActionBlock>> Source;
	TWeakPtr<SInlineEditableTextBlock> RequestedEditeTextBox;
};
