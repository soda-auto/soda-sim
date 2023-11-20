// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/UI/SToolBox.h"
#include "SodaStyleSet.h"

namespace soda
{


void SToolBox::Construct( const FArguments& InArgs )
{
	ChildSlot
	.HAlign(HAlign_Fill)
	.VAlign(VAlign_Fill)
	[
		InArgs._Content.Widget
	];
	
	if (InArgs._Caption.IsSet())
	{
		Caption = InArgs._Caption.Get();
	}
}



} // namespace soda

