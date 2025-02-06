// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Soda/UI/SMenuWindow.h"
#include "Templates/SharedPointer.h"
#include "Widgets/Views/SListView.h"
#include "UObject/WeakObjectPtrTemplates.h"

class SEditableTextBox;
class ASodaVehicle;
class STableViewBase;

class ITableRow;

namespace soda
{

class SFileDatabaseManager;


class SVehcileManagerWindow : public SMenuWindowContent
{
public:
	SLATE_BEGIN_ARGS(SVehcileManagerWindow)
	{}
	SLATE_END_ARGS()
	
	virtual ~SVehcileManagerWindow() {}
	void Construct( const FArguments& InArgs, ASodaVehicle* SelectedVehicle = nullptr);

	ASodaVehicle* GetSelectedvehicle() const { return SelectedVehicle.Get(); }

protected:
	FReply OnNewSave();
	FReply OnSave();
	FReply OnReplaceSelectedVehicle();

	bool CanRespwn() const;
	bool CanSave() const;
	bool CanNewSlot() const;

	TSharedPtr<SFileDatabaseManager> FileDatabaseManager;
	TWeakObjectPtr<ASodaVehicle> SelectedVehicle;
};

} // namespace