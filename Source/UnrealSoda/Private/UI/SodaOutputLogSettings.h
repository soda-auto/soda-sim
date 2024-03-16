// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "UObject/ObjectMacros.h"
#include "UObject/Object.h"
#include "Styling/SlateBrush.h"
#include "Containers/EnumAsByte.h"
#include "Misc/OutputDevice.h"
#include "Delegates/DelegateCombinations.h"
#include "SodaOutputLogSettings.generated.h"

UENUM()
enum class ESodaLogCategoryColorizationMode : uint8
{
	/** Do not colorize based on log categories */
	None,

	/** Colorize the entire log line, but not warnings or errors */
	ColorizeWholeLine,

	/** Colorize only the category name (including on warnings and errors) */
	ColorizeCategoryOnly,

	/** Colorize the background of the category name (including on warnings and errors) */
	ColorizeCategoryAsBadge
};

/**
 * Implements the Editor style settings.
 */
UCLASS(config=Game)
class UNREALSODA_API USodaOutputLogSettings : public UObject
{
	GENERATED_BODY()

public:
	USodaOutputLogSettings()
	{
		bCycleToOutputLogDrawer = true;
		LogTimestampMode = ELogTimes::None;
	}

	/** The font size used in the output log */
	UPROPERTY(EditAnywhere, config, Category="Output Log", meta=(DisplayName="Log Font Size", ConfigRestartRequired=true))
	int32 LogFontSize;

	/** The display mode for timestamps in the output log window */
	UPROPERTY(EditAnywhere, config, Category="Output Log", meta=(DisplayName = "Output Log Window Timestamp Mode"))
	TEnumAsByte<ELogTimes::Type> LogTimestampMode;

	/** How should categories be colorized in the output log? */
	UPROPERTY(EditAnywhere, config, Category = "Output Log")
	ESodaLogCategoryColorizationMode CategoryColorizationMode;

	/**
	 * If checked pressing the console command shortcut will cycle between focusing the status bar console, opening the output log drawer, and back to the previous focus target. 
	 * If unchecked, the console command shortcut will only focus the status bar console
	 */
	UPROPERTY(EditAnywhere, config, Category = "Output Log", meta = (DisplayName = "Open Output Log Drawer with Console Command Shortcut"))
	bool bCycleToOutputLogDrawer;

	UPROPERTY(EditAnywhere, config, Category = "Output Log")
	bool bEnableOutputLogWordWrap;

#if WITH_EDITORONLY_DATA
	UPROPERTY(config)
	bool bEnableOutputLogClearOnPIE;
#endif

public:

	/**
	 * Returns an event delegate that is executed when a setting has changed.
	 *
	 * @return The delegate.
	 */
	DECLARE_EVENT_OneParam(USodaOutputLogSettings, FSettingChangedEvent, FName /*PropertyName*/);
	FSettingChangedEvent& OnSettingChanged() { return SettingChangedEvent; }

protected:
#if WITH_EDITOR
	// UObject overrides
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override
	{
		Super::PostEditChangeProperty(PropertyChangedEvent);

		SaveConfig();
		const FName PropertyName = (PropertyChangedEvent.Property != nullptr) ? PropertyChangedEvent.Property->GetFName() : NAME_None;
		SettingChangedEvent.Broadcast(PropertyName);
	}
#endif

private:

	// Holds an event delegate that is executed when a setting has changed.
	FSettingChangedEvent SettingChangedEvent;
};
