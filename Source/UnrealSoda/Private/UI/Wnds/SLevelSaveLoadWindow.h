// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Soda/UI/SMenuWindow.h"
#include "Templates/SharedPointer.h"
#include "Input/Reply.h"
#include "Widgets/Views/SListView.h"

class SEditableTextBox;
class ITableRow;
class STableViewBase;
class SButton;

namespace soda
{

class SFileDatabaseManager;

class  SLevelSaveLoadWindow : public SMenuWindowContent
{
public:
	SLATE_BEGIN_ARGS(SLevelSaveLoadWindow)
	{}
	SLATE_END_ARGS()
	
	virtual ~SLevelSaveLoadWindow() {}
	void Construct( const FArguments& InArgs );


protected:
	FReply OnSave();
	FReply OnNewSave();
	FReply OnLoad();

	TSharedPtr<SButton> SaveButton;
	TSharedPtr<SButton> LoadButton;

	TSharedPtr<SFileDatabaseManager> FileDatabaseManager;
};


} // namespace