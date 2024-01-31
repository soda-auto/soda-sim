// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Soda/UI/SMenuWindow.h"
#include "Soda/IToolActor.h"
#include "UObject/WeakInterfacePtr.h"
#include "Templates/SharedPointer.h"
#include "Widgets/Views/SListView.h"

class SEditableTextBox;
class ITableRow;
class STableViewBase;


namespace soda
{

class  SPinToolActorWindow : public SMenuWindowContent
{
public:
	SLATE_BEGIN_ARGS(SPinToolActorWindow)
	{}
	SLATE_END_ARGS()
	
	virtual ~SPinToolActorWindow() {}
	void Construct( const FArguments& InArgs, IToolActor* ToolActor);

protected:
	void UpdateSlots();

	TSharedRef<ITableRow> OnGenerateRow(TSharedPtr<FString> Slot, const TSharedRef< STableViewBase >& OwnerTable);
	void OnDoubleClickRow(TSharedPtr<FString> Slot);

	FReply OnNewSave();
	FReply OnDeleteSlot(TSharedPtr<FString> Slot);

	TArray<TSharedPtr<FString>> Source;
	TSharedPtr<SListView<TSharedPtr<FString>>> ListView;
	TSharedPtr<SEditableTextBox> EditableTextBox;
	TWeakInterfacePtr<IToolActor> ToolActor;
};

} // namespace