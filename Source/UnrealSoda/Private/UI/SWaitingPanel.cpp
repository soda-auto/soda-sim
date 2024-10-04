// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/UI/SWaitingPanel.h"
#include "SodaStyleSet.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SBackgroundBlur.h"
#include "Widgets/Images/SThrobber.h"
#include "Widgets/SBoxPanel.h"
#include "Soda/SodaSubsystem.h"

namespace soda
{


void SWaitingPanel::Construct( const FArguments& InArgs )
{
	ChildSlot
	[
		SNew(SBackgroundBlur)
		.VAlign(VAlign_Center)
		.HAlign(HAlign_Center)
		.BlurStrength(4)
		//.Padding(20.0f)
		.CornerRadius(FVector4(4.0f, 4.0f, 4.0f, 4.0f))
		.Cursor(EMouseCursor::Default)
		[
			SNew(SVerticalBox)
			+SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SCircularThrobber)
			]
			+SVerticalBox::Slot()
			.Padding(0, 5, 0, 0)
			.AutoHeight()
			[
				SNew(STextBlock)
				//.TextStyle(&FSodaStyle::Get().GetWidgetStyle<FTextBlockStyle>("SmallText"))
				.Text(InArgs._Caption)
			]
			+SVerticalBox::Slot()
			.Padding(0, 5, 0, 0)
			.AutoHeight()
			[
				SNew(STextBlock)
				//.TextStyle(&FSodaStyle::Get().GetWidgetStyle<FTextBlockStyle>("SmallText"))
				.Text(InArgs._SubCaption)
			]
		]
		
	];
}

bool SWaitingPanel::CloseWindow()
{
	if (USodaSubsystem* SodaSubsystem = USodaSubsystem::Get())
	{
		if (!SodaSubsystem->CloseWaitingPanel(this))
		{
			return false;
		}
		else
		{
			return true;
		}
	}
	return false;
}

} // namespace soda

