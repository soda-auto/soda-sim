// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Soda/UI/SMenuWindow.h"
#include "Templates/SharedPointer.h"
#include "Input/Reply.h"
#include "Widgets/Views/SListView.h"

class SEditableTextBox;
struct FLevelStateSlotDescription;
class ITableRow;
class STableViewBase;
class SButton;

namespace soda
{

enum class ELevelSaveLoadWindowMode
{
	Local,
	Remote
};

class  SLevelSaveLoadWindow : public SMenuWindowContent
{
public:
	SLATE_BEGIN_ARGS(SLevelSaveLoadWindow)
	{}
	SLATE_END_ARGS()
	
	virtual ~SLevelSaveLoadWindow() {}
	void Construct( const FArguments& InArgs );

	ELevelSaveLoadWindowMode GetMode() const { return Mode; }
	void SetMode(ELevelSaveLoadWindowMode InMode) { Mode = InMode; }

protected:
	void UpdateSlots();

	TSharedRef<ITableRow> OnGenerateRow(TSharedPtr<FLevelStateSlotDescription> Slot, const TSharedRef< STableViewBase >& OwnerTable);
	void OnSelectionChanged(TSharedPtr<FLevelStateSlotDescription> Slot, ESelectInfo::Type SelectInfo);
	TSharedRef<SWidget> GetComboMenuContent();

	FReply OnSave();
	FReply OnNewSave();
	FReply OnLoad();
	FReply OnDeleteSlot(TSharedPtr<FLevelStateSlotDescription> Slot);

	TArray<TSharedPtr<FLevelStateSlotDescription>> Source;
	TSharedPtr<SListView<TSharedPtr<FLevelStateSlotDescription>>> ListView;
	TSharedPtr<SEditableTextBox> EditableTextBoxDesc;
	TSharedPtr<SButton> SaveButton;
	TSharedPtr<SButton> LoadButton;

	ELevelSaveLoadWindowMode Mode = ELevelSaveLoadWindowMode::Local;
};


} // namespace