// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "UI/Wnds/SChooseMapWindow.h"
#include "Soda/SodaStatics.h"
#include "Soda/SodaSubsystem.h"
#include "SodaStyleSet.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Views/SListView.h"
#include "Kismet/GameplayStatics.h"

namespace soda
{

struct FLevelInfo
{
	FLevelInfo(const FString& InPath, const FString& InName) :
		Path(InPath),
		Name(InName) 
	{}
	FString Path;
	FString Name;
};

void SChooseMapWindow::Construct( const FArguments& InArgs )
{
	TArray<FString> MapPaths = USodaStatics::GetAllMapPaths();
	for (auto& Path : MapPaths)
	{
		Source.Add(MakeShared<FLevelInfo>(Path, FPaths::GetBaseFilename(Path)));
	}

	ChildSlot
	[
		SNew(SBorder)
		.BorderImage(FSodaStyle::GetBrush("MenuWindow.Content"))
		.Padding(5.0f)
		[
			SNew(SListView<TSharedPtr<FLevelInfo>>)
			.ListItemsSource(&Source)
			.SelectionMode(ESelectionMode::Single)
			.OnGenerateRow(this, &SChooseMapWindow::OnGenerateRow)
			.OnMouseButtonDoubleClick(this, &SChooseMapWindow::OnDoubleClick)
		]
	];
}

TSharedRef<ITableRow> SChooseMapWindow::OnGenerateRow(TSharedPtr<FLevelInfo> LevelInfo, const TSharedRef< STableViewBase >& OwnerTable)
{
	return
		SNew(STableRow<TSharedPtr<FLevelInfo>>, OwnerTable)
		[
			SNew(STextBlock)
			.Text(FText::FromString(*LevelInfo->Path))
		];
}

void SChooseMapWindow::OnDoubleClick(TSharedPtr<FLevelInfo> LevelInfo)
{
	UGameplayStatics::OpenLevel(USodaStatics::GetGameWorld(), FName(*LevelInfo->Path), true);
}

} // namespace soda

