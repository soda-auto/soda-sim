// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "UI/Wnds/PakWindow/SPakWindow.h"
#include "Soda/UnrealSodaVersion.h"
#include "SodaStyleSet.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/SBoxPanel.h"
#include "Soda/SodaSubsystem.h"
#include "Soda/LevelState.h"
#include "SodaStyleSet.h"
#include "SPakItemList.h"

namespace soda
{


void SPakWindow::Construct(const FArguments& InArgs)
{
	


	ChildSlot
	[
		SNew(SBox)
		.Padding(5)
		.MinDesiredWidth(1200)
		.MinDesiredHeight(800)
		[
			SNew(SBorder)
			.BorderImage(FAppStyle::Get().GetBrush("Brushes.Panel"))
			[
				SNew(SPakItemList)
			]
		]
	];
}

} // namespace soda

