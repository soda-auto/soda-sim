// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "UObject/Object.h"
#include "Styling/SlateBrush.h"
#include "Rendering/RenderingCommon.h"
#include "SodaStyleSettings.generated.h"

UCLASS(config= SodaStyleSettings)
class SODASTYLE_API USodaStyleSettings : public UObject
{
public:
	GENERATED_UCLASS_BODY()

public:

	/** The color used to represent selection */
	UPROPERTY(EditAnywhere, config, Category=Colors, meta=(DisplayName="Viewport Selection Color"))
	FLinearColor SelectionColor;

	UPROPERTY(config)
	bool bEnableEditorWindowBackgroundColor;

	/** The color used to tint the editor window backgrounds */
	UPROPERTY(EditAnywhere, config, Category=Colors, meta=(EditCondition="bEnableEditorWindowBackgroundColor"))
	FLinearColor EditorWindowBackgroundColor;

	/** When enabled, the C++ names for properties and functions will be displayed in a format that is easier to read */
	UPROPERTY(EditAnywhere, config, Category=UserInterface, meta=(DisplayName="Show Friendly Variable Names"))
	uint32 bShowFriendlyNames:1;

	/** When Playing or Simulating, shows all properties (even non-visible and non-editable properties), if the object belongs to a simulating world.  This is useful for debugging. */
	UPROPERTY(config)
	uint32 bShowHiddenPropertiesWhilePlaying : 1;

	/** The font size used in the output log */
	UPROPERTY(EditAnywhere, config, Category="Output Log", meta=(DisplayName="Log Font Size", ConfigRestartRequired=true))
	int32 LogFontSize;

	/**
	 * If checked pressing the console command shortcut will cycle between focusing the status bar console, opening the output log drawer, and back to the previous focus target. 
	 * If unchecked, the console command shortcut will only focus the status bar console
	 */
	UPROPERTY(EditAnywhere, config, Category = "Output Log", meta = (DisplayName = "Open Output Log Drawer with Console Command Shortcut"))
	bool bCycleToOutputLogDrawer;


public:

	/** @return A subdued version of the users selection color (for use with inactive selection)*/
	FLinearColor GetSubduedSelectionColor() const;
protected:

};
