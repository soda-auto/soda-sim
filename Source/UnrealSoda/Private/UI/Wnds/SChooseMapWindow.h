// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Soda/UI/SMenuWindow.h"
#include "Templates/SharedPointer.h"

class ITableRow;
class STableViewBase;

namespace soda
{
struct FLevelInfo;

class  SChooseMapWindow : public SMenuWindowContent
{
public:
	SLATE_BEGIN_ARGS(SChooseMapWindow)
	{}
	SLATE_END_ARGS()
	
	virtual ~SChooseMapWindow() {}
	void Construct( const FArguments& InArgs );

protected:
	TSharedRef<ITableRow> OnGenerateRow(TSharedPtr<FLevelInfo> LevelInfo, const TSharedRef< STableViewBase >& OwnerTable);
	void OnDoubleClick(TSharedPtr<FLevelInfo> LevelInfo);

	TArray<TSharedPtr<FLevelInfo>> Source;
};

} // namespace soda