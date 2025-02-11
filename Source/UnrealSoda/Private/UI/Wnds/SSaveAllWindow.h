// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Soda/UI/SMenuWindow.h"
#include "Framework/Commands/UIAction.h"
#include "Templates/SharedPointer.h"

class STableViewBase;
class ITableRow;

namespace soda
{

enum class ESaveAllWindowMode
{
	Normal,
	Quit,
	Restart
};

struct FSaveAllWindowItem
{
	TAttribute<FString> Lable;
	FUIAction SaveAction;
	FUIAction ResaveAction;
	FName IconName;
	FName ClassName;
	TAttribute<bool> bIsDirty;
};

class  SSaveAllWindow : public SMenuWindowContent
{
public:
	SLATE_BEGIN_ARGS(SSaveAllWindow)
	{}
	SLATE_END_ARGS()
	
	virtual ~SSaveAllWindow() {}
	void Construct( const FArguments& InArgs, ESaveAllWindowMode Mode);

private:
	TSharedRef<ITableRow> OnGenerateRow(TSharedPtr<FSaveAllWindowItem> Item, const TSharedRef< STableViewBase >& OwnerTable);
	FReply OnQuit();
	FReply OnRestart();
	FReply OnCancel();
	FReply OnSaveAll();

	TArray<TSharedPtr<FSaveAllWindowItem>> Source;
	//SListView<TSharedPtr<FSaveAllWindowItem>> ListView;
};

} // namespace