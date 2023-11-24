// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Soda/UI/SMenuWindow.h"
#include "Templates/SharedPointer.h"
#include "Input/Reply.h"
#include "Widgets/Views/SListView.h"
#include "Soda/Vehicles/ISodaVehicleExporter.h"

class ASodaVehicle;

namespace soda
{

class  SExportVehicleWindow : public SMenuWindowContent
{
public:
	SLATE_BEGIN_ARGS(SExportVehicleWindow)
	{}
	SLATE_END_ARGS()
	
	virtual ~SExportVehicleWindow() {}
	void Construct( const FArguments& InArgs, TWeakObjectPtr<ASodaVehicle> Vehicle);

protected:
	void UpdateSlots();

	TSharedRef<ITableRow> OnGenerateRow(TSharedPtr<ISodaVehicleExporter> Exporter, const TSharedRef< STableViewBase >& OwnerTable);
	void OnSelectionChanged(TSharedPtr<ISodaVehicleExporter> Exporter, ESelectInfo::Type SelectInfo);

	FReply OnExportAs();

	TArray<TSharedPtr<ISodaVehicleExporter>> Source;
	TSharedPtr<SListView<TSharedPtr<ISodaVehicleExporter>>> ListView;
	TSharedPtr<SButton> ExportButton;
	TWeakObjectPtr<ASodaVehicle> Vehicle;
};


} // namespace