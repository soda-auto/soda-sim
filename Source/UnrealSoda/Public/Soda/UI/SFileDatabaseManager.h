// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Soda/FileDatabaseManager.h"
#include "Templates/SharedPointer.h"
#include "Widgets/Views/SListView.h"

class SEditableTextBox;
class STableViewBase;

class ITableRow;

namespace soda
{

class SFileDatabaseManager : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SFileDatabaseManager)
	{}
	SLATE_END_ARGS()
	
	virtual ~SFileDatabaseManager() {}
	void Construct( const FArguments& InArgs, EFileSlotType SlotType, const FGuid & TargetGuid);
	void UpdateSlots();
	FName GetSelectedSource() const { return Source; }

	TSharedPtr<FFileDatabaseSlotInfo> GetSelectedSlot();
	FString GetLableText() const;
	FString GetDescriptionText() const;

	TSharedPtr<SListView<TSharedPtr<FFileDatabaseSlotInfo>>> ListView;

protected:

	TSharedRef<ITableRow> OnGenerateRow(TSharedPtr<FFileDatabaseSlotInfo> Address, const TSharedRef< STableViewBase >& OwnerTable);
	void OnSelectionChanged(TSharedPtr<FFileDatabaseSlotInfo> Address, ESelectInfo::Type SelectInfo);
	TSharedRef<SWidget> GetComboMenuContent();


	TSharedPtr<SEditableTextBox> LableTextBox;
	TSharedPtr<SEditableTextBox> DescriptionTextBox;
	TArray<TSharedPtr<FFileDatabaseSlotInfo>> Slots;
	FName Source = NAME_None;
	EFileSlotType SlotType;
	FGuid TargetGuid;
};

} // namespace