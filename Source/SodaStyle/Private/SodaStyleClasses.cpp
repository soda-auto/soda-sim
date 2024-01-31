// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "CoreMinimal.h"
#include "Framework/Application/SlateApplication.h"
#include "Classes/SodaStyleSettings.h"
#include "Misc/ConfigCacheIni.h"
#include "Modules/ModuleManager.h"

//#if WITH_EDITOR
	#include "UObject/UnrealType.h"
//#endif



/* USodaStyleSettings interface
 *****************************************************************************/

USodaStyleSettings::USodaStyleSettings( const FObjectInitializer& ObjectInitializer )
	: Super(ObjectInitializer)
{
	SelectionColor = FLinearColor(0.828f, 0.364f, 0.003f);
	EditorWindowBackgroundColor = FLinearColor::White;
	bCycleToOutputLogDrawer = true;
	bShowFriendlyNames = true;
}


FLinearColor USodaStyleSettings::GetSubduedSelectionColor() const
{
	FLinearColor SubduedSelectionColor = SelectionColor.LinearRGBToHSV();
	SubduedSelectionColor.G *= 0.55f;		// take the saturation 
	SubduedSelectionColor.B *= 0.8f;		// and brightness down

	return SubduedSelectionColor.HSVToLinearRGB();
}

