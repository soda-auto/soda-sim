// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Soda/UI/SMenuWindow.h"
#include "Soda/FileDatabaseManager.h"
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


class SSlotActorManagerWindow : public SMenuWindowContent
{
public:
	SLATE_BEGIN_ARGS(SSlotActorManagerWindow)
	{}
	SLATE_END_ARGS()
	
	virtual ~SSlotActorManagerWindow() {}
	void Construct( const FArguments& InArgs, EFileSlotType Type, AActor* SelectedActor = nullptr);

	AActor* GetSelectedActor() const { return SelectedActor.Get(); }

protected:
	FReply OnNewSave();
	FReply OnSave();
	FReply OnReplaceSelectedActor();

	bool CanRespwn() const;
	bool CanSave() const;
	bool CanNewSlot() const;

	TSharedPtr<SFileDatabaseManager> FileDatabaseManager;
	TWeakObjectPtr<AActor> SelectedActor;
};

} // namespace