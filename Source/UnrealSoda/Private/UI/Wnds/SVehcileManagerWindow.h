// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Soda/UI/SMenuWindow.h"
#include "Templates/SharedPointer.h"
#include "Widgets/Views/SListView.h"
#include "UObject/WeakObjectPtrTemplates.h"

class SEditableTextBox;
class ASodaVehicle;
struct FVechicleSaveAddress;
class STableViewBase;

class ITableRow;

namespace soda
{

enum class EVehicleManagerSource
{
	Local,
	Remote
};

class SVehcileManagerWindow : public SMenuWindowContent
{
public:
	SLATE_BEGIN_ARGS(SVehcileManagerWindow)
	{}
	SLATE_END_ARGS()
	
	virtual ~SVehcileManagerWindow() {}
	void Construct( const FArguments& InArgs, ASodaVehicle* Vehicle);

	EVehicleManagerSource GetSource() const { return Source; }
	void SetSource(EVehicleManagerSource InSource) { Source = InSource; }

protected:
	void UpdateSlots();

	TSharedRef<ITableRow> OnGenerateRow(TSharedPtr<FVechicleSaveAddress> Address, const TSharedRef< STableViewBase >& OwnerTable);
	void OnSelectionChanged(TSharedPtr<FVechicleSaveAddress> Address, ESelectInfo::Type SelectInfo);

	FReply OnNewSave();
	FReply OnSave();
	FReply OnLoad();
	FReply OnDeleteSlot(TSharedPtr<FVechicleSaveAddress> Address);
	TSharedRef<SWidget> GetComboMenuContent();

	TSharedPtr<SListView<TSharedPtr<FVechicleSaveAddress>>> ListView;
	TSharedPtr<SEditableTextBox> EditableTextBox;
	TArray<TSharedPtr<FVechicleSaveAddress>> SavedVehicles;
	TWeakObjectPtr<ASodaVehicle> Vehicle;
	EVehicleManagerSource Source = EVehicleManagerSource::Local;
};

} // namespace