// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/UObjectGlobals.h"
#include "Modules/ModuleManager.h"
#include "Styling/SlateColor.h"
#include "Styling/SlateStyle.h"
#include "Styling/SlateTypes.h"
#include "SodaStyleSet.h"
#include "Classes/SodaStyleSettings.h"
//#include "ISettingsModule.h"
//#include "SodaStyleSettingsCustomization.h"
#include "SodaStyleModule.h"

struct FPropertyChangedEvent;

/**
 * Declares the Editor's visual style.
 */
class FStarshipSodaStyle
	: public FSodaStyle
{
public:

	static void Initialize();
	static void Shutdown();

	static void SyncCustomizations()
	{
		FStarshipSodaStyle::StyleInstance->SyncSettings();
	}

	class FStyle : public FSlateStyleSet
	{
	public:
		FStyle(const TWeakObjectPtr< USodaStyleSettings >& InSettings);

		void Initialize();
		void SetupGeneralStyles();
		void SetupViewportStyles();
		void SetupMenuBarStyles();
		void SetupGeneralIcons();
		void SetupWindowStyles();
		void SetupPropertySodaStyles();
		void SetupGraphSodaStyles();
		void SetupLevelSodaStyle();
		void SetupPersonaStyle();
		void SetupClassIconsAndThumbnails();
		void SetupColorPickerStyle();
		void SetupTutorialStyles();
		void SetupScenarioEditorStyle();
		void SetupPakWindowStyle();
		void SetupQuickStartWindowStyle();

		void SettingsChanged(UObject* ChangedObject, FPropertyChangedEvent& PropertyChangedEvent);
		void SyncSettings();
		void SyncParentStyles();

		static void SetColor(const TSharedRef<FLinearColor>& Source, const FLinearColor& Value);

		const FVector2D Icon7x16;
		const FVector2D Icon8x4;
		const FVector2D Icon16x4;
		const FVector2D Icon8x8;
		const FVector2D Icon10x10;
		const FVector2D Icon12x12;
		const FVector2D Icon12x16;
		const FVector2D Icon14x14;
		const FVector2D Icon16x16;
		const FVector2D Icon16x20;
		const FVector2D Icon20x20;
		const FVector2D Icon22x22;
		const FVector2D Icon24x24;
		const FVector2D Icon25x25;
		const FVector2D Icon32x32;
		const FVector2D Icon40x40;
		const FVector2D Icon48x48;
		const FVector2D Icon64x64;
		const FVector2D Icon36x24;
		const FVector2D Icon128x128;

		// These are the colors that are updated by the user style customizations
		const TSharedRef< FLinearColor > SelectionColor_Subdued_LinearRef;
		const TSharedRef< FLinearColor > HighlightColor_LinearRef;
		const TSharedRef< FLinearColor > WindowHighlightColor_LinearRef;

		// These are the Slate colors which reference those above; these are the colors to put into the style
		// Most of these are owned by our parent style
		FSlateColor DefaultForeground;
		FSlateColor InvertedForeground;
		FSlateColor SelectorColor;
		FSlateColor SelectionColor;
		FSlateColor SelectionColor_Inactive;
		FSlateColor SelectionColor_Pressed;

		const FSlateColor SelectionColor_Subdued;
		const FSlateColor HighlightColor;
		const FSlateColor WindowHighlightColor;

		const FSlateColor LogColor_SelectionBackground;
		const FSlateColor LogColor_Normal;
		const FSlateColor LogColor_Command;

		// These are common colors used thruout the editor in mutliple style elements
		const FSlateColor InheritedFromBlueprintTextColor;

		// Styles inherited from the parent style
		FTextBlockStyle NormalText;
		FEditableTextBoxStyle NormalEditableTextBoxStyle;
		FTableRowStyle NormalTableRowStyle;
		FButtonStyle Button;
		FButtonStyle HoverHintOnly;
		FButtonStyle NoBorder;
		FScrollBarStyle ScrollBar;
		FSlateFontInfo NormalFont;
		FSlateBrush EditorWindowHighlightBrush;

		TWeakObjectPtr< USodaStyleSettings > Settings;
	};

	static TSharedRef<class FStarshipSodaStyle::FStyle> Create(const TWeakObjectPtr< USodaStyleSettings >& InCustomization)
	{
		TSharedRef<class FStarshipSodaStyle::FStyle> NewStyle = MakeShareable(new FStarshipSodaStyle::FStyle(InCustomization));
		NewStyle->Initialize();

#if WITH_EDITOR
		FCoreUObjectDelegates::OnObjectPropertyChanged.AddSP(NewStyle, &FStarshipSodaStyle::FStyle::SettingsChanged);
#endif

		return NewStyle;
	}

	static TSharedPtr<FStarshipSodaStyle::FStyle> StyleInstance;
	static TWeakObjectPtr<USodaStyleSettings> Settings;
};
