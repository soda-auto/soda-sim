// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/Views/SListView.h"
#include "RuntimePropertyEditor/PropertyCustomizationHelpers.h"
#include "Templates/SharedPointer.h"
#include "UObject/WeakObjectPtrTemplates.h"

class USodaGameViewportClient;
class ASodaActorFactory;
class SSearchBox;
class SInlineEditableTextBlock;
class AActor;

namespace soda
{

class SActorListRow;

class SActorList: public SCompoundWidget
{
	friend SActorListRow;

public:

	SLATE_BEGIN_ARGS(SActorList)
		: _OnSelectionChanged()
		, _ActorFilter()
		, _bInteractiveMode(true)
	{}
	SLATE_EVENT(SListView<TWeakObjectPtr<AActor>>::FOnSelectionChanged, OnSelectionChanged)
	SLATE_ARGUMENT(FOnShouldFilterActor, ActorFilter)
	SLATE_ATTRIBUTE(bool, bInteractiveMode)
	SLATE_END_ARGS()
	

	void Construct(const FArguments& Args, USodaGameViewportClient* ViewportClient);
	virtual ~SActorList();

	void RebuilActorList();
	AActor* GetSelectedActor() const;

protected:
	virtual void Tick(const FGeometry& AllottedGeometry, const double CurrentTime, const float DeltaTime) override;

protected:
	void OnRenameActor(AActor* SelectedActor);
	void OnDeleteActor(AActor* SelectedActor);
	void OnPossesActor(APawn* SelectedActor);
	void OnSelectionChanged(TWeakObjectPtr<AActor> Actor, ESelectInfo::Type SelectInfo);

	TSharedRef< ITableRow > MakeRowWidget(TWeakObjectPtr<AActor> Actor, const TSharedRef< STableViewBase >& OwnerTable);
	TSharedPtr<SWidget> OnMenuOpening();
	FReply OnPinClicked(TWeakObjectPtr<AActor> Actor);

protected:
	bool bIsInteractiveMode = true;
	TSharedPtr<SListView<TWeakObjectPtr<AActor>>> TreeView;
	TSharedPtr<SSearchBox> SearchBox;
	TArray<TWeakObjectPtr<AActor>> ActorList;
	TWeakObjectPtr<USodaGameViewportClient> ViewportClient;
	TWeakObjectPtr<ASodaActorFactory> ActorFactory;
	TWeakObjectPtr<AActor> SelectedItem;
	TWeakPtr<SInlineEditableTextBlock> RequestedEditeTextBox;
	SListView<TWeakObjectPtr<AActor>>::FOnSelectionChanged OnSelectionChangedDelegate;
	FDelegateHandle ActorsMapChenagedHandle;
	FOnShouldFilterActor ActorFilter;
};

} // namespace soda