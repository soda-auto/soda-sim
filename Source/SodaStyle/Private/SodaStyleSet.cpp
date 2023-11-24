// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "SodaStyleSet.h"
#include "Styling/SlateStyleRegistry.h"
#include "Styling/CoreStyle.h"

TSharedPtr< ISlateStyle > FSodaStyle::Instance = NULL;

void FSodaStyle::ResetToDefault()
{
	SetStyle( FCoreStyle::Create("SodaStyle") );
}

void FSodaStyle::SetStyle( const TSharedRef< ISlateStyle >& NewStyle )
{
	if( Instance != NewStyle )
	{
		if ( Instance.IsValid() )
		{
			FSlateStyleRegistry::UnRegisterSlateStyle( *Instance.Get() );
		}

		Instance = NewStyle;

		if ( Instance.IsValid() )
		{
			FSlateStyleRegistry::RegisterSlateStyle( *Instance.Get() );
		}
		else
		{
			ResetToDefault();
		}
	}
}
