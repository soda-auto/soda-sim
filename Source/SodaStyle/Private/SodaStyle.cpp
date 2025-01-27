// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "SodaStyle.h"
#include "Misc/CommandLine.h"
#include "Styling/StarshipCoreStyle.h"
#include "Classes/SodaStyleSettings.h"
#include "SlateOptMacros.h"
#include "Styling/SlateStyleMacros.h"
#include "Styling/ToolBarStyle.h"
#include "Styling/SegmentedControlStyle.h"
#include "Styling/StyleColors.h"
#include "Framework/Application/SlateApplication.h"
#include "Interfaces/IPluginManager.h"

#define ICON_FONT(...) FSlateFontInfo(RootToContentDir("Fonts/FontAwesome", TEXT(".ttf")), __VA_ARGS__)

#define LOCTEXT_NAMESPACE "SodaStyle"

void FStarshipSodaStyle::Initialize()
{
	LLM_SCOPE_BYNAME(TEXT("FStarshipSodaStyle"));
	Settings = NULL;

	FSlateApplication::InitializeCoreStyle();

	const FString ThemesSubDir = TEXT("Slate/Themes");

	StyleInstance = Create(Settings);
	SetStyle(StyleInstance.ToSharedRef());
}

void FStarshipSodaStyle::Shutdown()
{
	StyleInstance.Reset();
}

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION


TSharedPtr< FStarshipSodaStyle::FStyle > FStarshipSodaStyle::StyleInstance = NULL;
TWeakObjectPtr< USodaStyleSettings > FStarshipSodaStyle::Settings = NULL;

void FStarshipSodaStyle::FStyle::SetColor(const TSharedRef< FLinearColor >& Source, const FLinearColor& Value)
{
	Source->R = Value.R;
	Source->G = Value.G;
	Source->B = Value.B;
	Source->A = Value.A;
}

/* FStarshipSodaStyle interface
 *****************************************************************************/

FStarshipSodaStyle::FStyle::FStyle( const TWeakObjectPtr< USodaStyleSettings >& InSettings )
	: FSlateStyleSet("SodaStyle")

	// Note, these sizes are in Slate Units.
	// Slate Units do NOT have to map to pixels.
	, Icon7x16(7.0f, 16.0f)
	, Icon8x4(8.0f, 4.0f)
	, Icon16x4(16.0f, 4.0f)
	, Icon8x8(8.0f, 8.0f)
	, Icon10x10(10.0f, 10.0f)
	, Icon12x12(12.0f, 12.0f)
	, Icon12x16(12.0f, 16.0f)
	, Icon14x14(14.0f, 14.0f)
	, Icon16x16(16.0f, 16.0f)
	, Icon16x20(16.0f, 20.0f)
	, Icon20x20(20.0f, 20.0f)
	, Icon22x22(22.0f, 22.0f)
	, Icon24x24(24.0f, 24.0f)
	, Icon25x25(25.0f, 25.0f)
	, Icon32x32(32.0f, 32.0f)
	, Icon40x40(40.0f, 40.0f)
	, Icon48x48(48.0f, 48.0f)
	, Icon64x64(64.0f, 64.0f)
	, Icon36x24(36.0f, 24.0f)
	, Icon128x128(128.0f, 128.0f)

	// These are the colors that are updated by the user style customizations
	, SelectionColor_Subdued_LinearRef(MakeShareable(new FLinearColor(0.807f, 0.596f, 0.388f)))
	, HighlightColor_LinearRef( MakeShareable( new FLinearColor(0.068f, 0.068f, 0.068f) ) ) 
	, WindowHighlightColor_LinearRef(MakeShareable(new FLinearColor(0,0,0,0)))



	// These are the Slate colors which reference those above; these are the colors to put into the style
	, SelectionColor_Subdued( SelectionColor_Subdued_LinearRef )
	, HighlightColor( HighlightColor_LinearRef )
	, WindowHighlightColor(WindowHighlightColor_LinearRef)
	, LogColor_SelectionBackground(EStyleColor::User2)
	, LogColor_Normal(EStyleColor::User3)
	, LogColor_Command(EStyleColor::User4)

	, InheritedFromBlueprintTextColor(FLinearColor(0.25f, 0.5f, 1.0f))

	, Settings( InSettings )
{
}


void FStarshipSodaStyle::FStyle::SettingsChanged(UObject* ChangedObject, FPropertyChangedEvent& PropertyChangedEvent)
{
	if ( ChangedObject == Settings.Get() )
	{
		SyncSettings();
	}
}

void FStarshipSodaStyle::FStyle::SyncSettings()
{
	if (Settings.IsValid())
	{
		// The subdued selection color is derived from the selection color
		auto SubduedSelectionColor = Settings->GetSubduedSelectionColor();
		SetColor(SelectionColor_Subdued_LinearRef, SubduedSelectionColor);

		// Also sync the colors used by FStarshipCoreStyle, as FSodaStyle isn't yet being used as an override everywhere
	/*	FStarshipCoreStyle::SetSelectorColor(Settings->KeyboardFocusColor);
		FStarshipCoreStyle::SetSelectionColor(Settings->SelectionColor);
		FStarshipCoreStyle::SetInactiveSelectionColor(Settings->InactiveSelectionColor);
		FStarshipCoreStyle::SetPressedSelectionColor(Settings->PressedSelectionColor);
*/

		// Sync the window background settings
		FWindowStyle& WindowStyle = const_cast<FWindowStyle&>(FStarshipCoreStyle::GetCoreStyle().GetWidgetStyle<FWindowStyle>("Window"));

		if (Settings->bEnableEditorWindowBackgroundColor)
		{
			SetColor(WindowHighlightColor_LinearRef, Settings->EditorWindowBackgroundColor);
		}
		else
		{
			SetColor(WindowHighlightColor_LinearRef, FLinearColor(0, 0, 0, 0));
		}
	}
}

void FStarshipSodaStyle::FStyle::SyncParentStyles()
{
	const ISlateStyle* ParentStyle = GetParentStyle();

	// Get the scrollbar style from the core style as it is referenced by the editor style
	ScrollBar = ParentStyle->GetWidgetStyle<FScrollBarStyle>("ScrollBar");
	NoBorder = ParentStyle->GetWidgetStyle<FButtonStyle>("NoBorder");
	NormalFont = ParentStyle->GetFontStyle("NormalFont");
	NormalText = ParentStyle->GetWidgetStyle<FTextBlockStyle>("NormalText");
	Button = ParentStyle->GetWidgetStyle<FButtonStyle>("Button");
	NormalEditableTextBoxStyle = ParentStyle->GetWidgetStyle<FEditableTextBoxStyle>("NormalEditableTextBox");
	NormalTableRowStyle = ParentStyle->GetWidgetStyle<FTableRowStyle>("TableView.Row");

	DefaultForeground = ParentStyle->GetSlateColor("DefaultForeground");
	InvertedForeground = ParentStyle->GetSlateColor("InvertedForeground");

	SelectorColor = ParentStyle->GetSlateColor("SelectorColor");
	SelectionColor = ParentStyle->GetSlateColor("SelectionColor");
	SelectionColor_Inactive = ParentStyle->GetSlateColor("SelectionColor_Inactive");
	SelectionColor_Pressed = ParentStyle->GetSlateColor("SelectionColor_Pressed");
}

static void AuditDuplicatedCoreStyles(const ISlateStyle& SodaStyle)
{
	const ISlateStyle& CoreStyle = FStarshipCoreStyle::GetCoreStyle();
	TSet<FName> CoreStyleKeys = CoreStyle.GetStyleKeys();

	TSet<FName> SodaStyleKeys = SodaStyle.GetStyleKeys();

	TSet<FName> DuplicatedNames = CoreStyleKeys.Intersect(SodaStyleKeys);

	DuplicatedNames.Sort(FNameLexicalLess());
	for (FName& Name : DuplicatedNames)
	{
		UE_LOG(LogSlate, Log, TEXT("%s"), *Name.ToString());
	}
}

void FStarshipSodaStyle::FStyle::Initialize()
{
	SetParentStyleName("CoreStyle");

	// Sync styles from the parent style that will be used as templates for styles defined here
	SyncParentStyles();
	SetContentRoot(IPluginManager::Get().FindPlugin(TEXT("SodaSim"))->GetContentDir() / TEXT("Slate"));
	SetCoreContentRoot(FPaths::EngineContentDir() / TEXT("Slate"));

	SetupGeneralStyles();
	SetupViewportStyles();
	SetupMenuBarStyles();
	SetupGeneralIcons();
	SetupWindowStyles();
	SetupPropertySodaStyles();
	SetupGraphSodaStyles();
	SetupLevelSodaStyle();
	SetupPersonaStyle();
	SetupClassIconsAndThumbnails();
	SetupColorPickerStyle();
	SetupTutorialStyles();
	SetupScenarioEditorStyle();
	SetupPakWindowStyle();
	SetupQuickStartWindowStyle();

	AuditDuplicatedCoreStyles(*this);
	
	SyncSettings();
}

void FStarshipSodaStyle::FStyle::SetupGeneralStyles()
{
	Set("MenuWindow.Caption", new FSlateRoundedBoxBrush(FStyleColors::Recessed, 12.f, FLinearColor(0, 0, 0, .8), 1.0));
	Set("MenuWindow.Border", new FSlateRoundedBoxBrush(FStyleColors::Dropdown, 12.f, FLinearColor(0, 0, 0, .8), 1.0));
	Set("MenuWindow.Content", new FSlateRoundedBoxBrush(FStyleColors::Recessed, 12.f, FLinearColor(0, 0, 0, .8), 1.0));

	FButtonStyle CloseMenuWindowButton = FButtonStyle()
		.SetNormal(IMAGE_BRUSH("Common/X", Icon16x16, FLinearColor(1, 1, 1, 0.4)))
		.SetHovered(IMAGE_BRUSH("Common/X", Icon16x16, FLinearColor(1, 1, 1, 1.0)))
		.SetPressed(IMAGE_BRUSH("Common/X", Icon16x16, FLinearColor(1, 1, 1, 0.8)));
		//.SetNormalPadding(FMargin(0, 0, 0, 1))
		//.SetPressedPadding(FMargin(0, 1, 0, 0));
	Set("MenuWindow.CloseButton", CloseMenuWindowButton);

	FButtonStyle DeleteButton = FButtonStyle()
		.SetNormal(CORE_IMAGE_BRUSH_SVG("Starship/Common/Delete", Icon16x16, FLinearColor(1, 1, 1, 0.4)))
		.SetHovered(CORE_IMAGE_BRUSH_SVG("Starship/Common/Delete", Icon16x16, FLinearColor(1, 1, 1, 1.0)))
		.SetPressed(CORE_IMAGE_BRUSH_SVG("Starship/Common/Delete", Icon16x16, FLinearColor(1, 1, 1, 0.8)));
	//.SetNormalPadding(FMargin(0, 0, 0, 1))
	//.SetPressedPadding(FMargin(0, 1, 0, 0));
	Set("MenuWindow.DeleteButton", DeleteButton);

	FButtonStyle AddButton = FButtonStyle()
		.SetNormal(CORE_IMAGE_BRUSH_SVG("Starship/Common/plus", Icon16x16, FLinearColor(1, 1, 1, 0.4)))
		.SetHovered(CORE_IMAGE_BRUSH_SVG("Starship/Common/plus", Icon16x16, FLinearColor(1, 1, 1, 1.0)))
		.SetPressed(CORE_IMAGE_BRUSH_SVG("Starship/Common/plus", Icon16x16, FLinearColor(1, 1, 1, 0.8)));
	//.SetNormalPadding(FMargin(0, 0, 0, 1))
	//.SetPressedPadding(FMargin(0, 1, 0, 0));
	Set("MenuWindow.AddButton", AddButton);

	FButtonStyle SaveButton = FButtonStyle()
		.SetNormal(CORE_IMAGE_BRUSH_SVG("Starship/Common/save", Icon16x16, FLinearColor(1, 1, 1, 0.4)))
		.SetHovered(CORE_IMAGE_BRUSH_SVG("Starship/Common/save", Icon16x16, FLinearColor(1, 1, 1, 1.0)))
		.SetPressed(CORE_IMAGE_BRUSH_SVG("Starship/Common/save", Icon16x16, FLinearColor(1, 1, 1, 0.8)));
	//.SetNormalPadding(FMargin(0, 0, 0, 1))
	//.SetPressedPadding(FMargin(0, 1, 0, 0));
	Set("MenuWindow.SaveButton", SaveButton);

	Set("RowButton", FButtonStyle(Button)
		.SetNormal(FSlateNoResource())
		.SetHovered(BOX_BRUSH("Common/RoundedSelection_16x", 4.0f / 16.0f, SelectionColor))
		.SetPressed(BOX_BRUSH("Common/RoundedSelection_16x", 4.0f / 16.0f, SelectionColor))
	);

	Set("RowButtonSelected", FButtonStyle(Button)
		.SetNormal(BOX_BRUSH("Common/RoundedSelection_16x", 4.0f / 16.0f, SelectionColor_Pressed))
		.SetHovered(FSlateNoResource())
		.SetPressed(FSlateNoResource())
	);

	// Normal Text
	{
		Set( "RichTextBlock.TextHighlight", FTextBlockStyle(NormalText)
			.SetColorAndOpacity( FLinearColor( 1.0f, 1.0f, 1.0f ) ) );
		Set( "RichTextBlock.Bold", FTextBlockStyle(NormalText)
			.SetFont( DEFAULT_FONT("Bold", FStarshipCoreStyle::RegularTextSize )) );
		Set( "RichTextBlock.BoldHighlight", FTextBlockStyle(NormalText)
			.SetFont( DEFAULT_FONT("Bold", FStarshipCoreStyle::RegularTextSize ))
			.SetColorAndOpacity( FLinearColor( 1.0f, 1.0f, 1.0f ) ) );
		Set("RichTextBlock.Italic", FTextBlockStyle(NormalText)
			.SetFont(DEFAULT_FONT("Italic", FStarshipCoreStyle::RegularTextSize)));
		Set("RichTextBlock.ItalicHighlight", FTextBlockStyle(NormalText)
			.SetFont(DEFAULT_FONT("Italic", FStarshipCoreStyle::RegularTextSize))
			.SetColorAndOpacity(FLinearColor(1.0f, 1.0f, 1.0f)));

		Set( "TextBlock.HighlightShape",  new BOX_BRUSH( "Common/TextBlockHighlightShape", FMargin(3.f/8.f) ));
		Set( "TextBlock.HighlighColor", FLinearColor( 0.02f, 0.3f, 0.0f ) );

		Set("TextBlock.ShadowedText", FTextBlockStyle(NormalText)
			.SetShadowOffset(FVector2D(1.0f, 1.0f))
			.SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f)));

		Set("TextBlock.ShadowedTextWarning", FTextBlockStyle(NormalText)
			.SetColorAndOpacity(FStyleColors::Warning)
			.SetShadowOffset(FVector2D(1.0f, 1.0f))
			.SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f)));

		Set("NormalText.Subdued", FTextBlockStyle(NormalText)
			.SetColorAndOpacity(FSlateColor::UseSubduedForeground()));

		Set("NormalText.Important", FTextBlockStyle(NormalText)
			.SetFont(DEFAULT_FONT("Bold", FStarshipCoreStyle::RegularTextSize))
			.SetColorAndOpacity(FLinearColor(1.0f, 1.0f, 1.0f))
			.SetHighlightColor(FLinearColor(1.0f, 1.0f, 1.0f))
			.SetShadowOffset(FVector2D(1, 1))
			.SetShadowColorAndOpacity(FLinearColor(0, 0, 0, 0.9f)));

		Set("SmallText.Subdued", FTextBlockStyle(NormalText)
			.SetFont(DEFAULT_FONT("Regular", FStarshipCoreStyle::SmallTextSize))
			.SetColorAndOpacity(FSlateColor::UseSubduedForeground()));

		Set("TinyText", FTextBlockStyle(NormalText)
			.SetFont(DEFAULT_FONT("Regular", FStarshipCoreStyle::SmallTextSize)));

		Set("TinyText.Subdued", FTextBlockStyle(NormalText)
			.SetFont(DEFAULT_FONT("Regular", FStarshipCoreStyle::SmallTextSize))
			.SetColorAndOpacity(FSlateColor::UseSubduedForeground()));

		Set("LargeText", FTextBlockStyle(NormalText)
			.SetFont(DEFAULT_FONT("Bold", 11))
			.SetColorAndOpacity(FLinearColor(1.0f, 1.0f, 1.0f))
			.SetHighlightColor(FLinearColor(1.0f, 1.0f, 1.0f))
			.SetShadowOffset(FVector2D(1, 1))
			.SetShadowColorAndOpacity(FLinearColor(0, 0, 0, 0.9f)));
	}

	// Rendering resources that never change
	{
		Set( "None", new FSlateNoResource() );
	}



	Set("PlainBorder", new BORDER_BRUSH("Common/PlainBorder", 2.f / 8.f));

	Set( "WideDash.Horizontal", new CORE_IMAGE_BRUSH("Starship/Common/Dash_Horizontal", FVector2D(10, 1), FLinearColor::White, ESlateBrushTileType::Horizontal));
	Set( "WideDash.Vertical", new CORE_IMAGE_BRUSH("Starship/Common/Dash_Vertical", FVector2D(1, 10), FLinearColor::White, ESlateBrushTileType::Vertical));

	Set("DropTarget.Background", new CORE_BOX_BRUSH("Starship/Common/DropTargetBackground", FMargin(6.0f / 64.0f)));

	Set("ThinLine.Horizontal", new IMAGE_BRUSH("Common/ThinLine_Horizontal", FVector2D(11, 2), FLinearColor::White, ESlateBrushTileType::Horizontal));


	// Buttons that only provide a hover hint.
	HoverHintOnly = FButtonStyle()
			.SetNormal( FSlateNoResource() )
			.SetHovered( BOX_BRUSH( "Common/ButtonHoverHint", FMargin(4/16.0f), FLinearColor(1,1,1,0.15f) ) )
			.SetPressed( BOX_BRUSH( "Common/ButtonHoverHint", FMargin(4/16.0f), FLinearColor(1,1,1,0.25f) ) )
			.SetNormalPadding( FMargin(0,0,0,1) )
			.SetPressedPadding( FMargin(0,1,0,0) );
	Set( "HoverHintOnly", HoverHintOnly );


	FButtonStyle SimpleSharpButton = FButtonStyle()
		.SetNormal(BOX_BRUSH("Common/Button/simple_sharp_normal", FMargin(4 / 16.0f), FLinearColor(1, 1, 1, 1)))
		.SetHovered(BOX_BRUSH("Common/Button/simple_sharp_hovered", FMargin(4 / 16.0f), FLinearColor(1, 1, 1, 1)))
		.SetPressed(BOX_BRUSH("Common/Button/simple_sharp_hovered", FMargin(4 / 16.0f), FLinearColor(1, 1, 1, 1)))
		.SetNormalPadding(FMargin(0, 0, 0, 1))
		.SetPressedPadding(FMargin(0, 1, 0, 0));
	Set("SimpleSharpButton", SimpleSharpButton);

	FButtonStyle SimpleRoundButton = FButtonStyle()
		.SetNormal(BOX_BRUSH("Common/Button/simple_round_normal", FMargin(4 / 16.0f), FLinearColor(1, 1, 1, 1)))
		.SetHovered(BOX_BRUSH("Common/Button/simple_round_hovered", FMargin(4 / 16.0f), FLinearColor(1, 1, 1, 1)))
		.SetPressed(BOX_BRUSH("Common/Button/simple_round_hovered", FMargin(4 / 16.0f), FLinearColor(1, 1, 1, 1)))
		.SetNormalPadding(FMargin(0, 0, 0, 1))
		.SetPressedPadding(FMargin(0, 1, 0, 0));
	Set("SimpleRoundButton", SimpleRoundButton);

	// Common glyphs
	{
		Set( "Symbols.SearchGlass", new IMAGE_BRUSH( "Common/SearchGlass", Icon16x16 ) );
		Set( "Symbols.X", new IMAGE_BRUSH( "Common/X", Icon16x16 ) );
		Set( "Symbols.VerticalPipe", new BOX_BRUSH( "Common/VerticalPipe", FMargin(0) ) );
		Set( "Symbols.UpArrow", new IMAGE_BRUSH( "Common/UpArrow", Icon8x8 ) );
		Set( "Symbols.DoubleUpArrow", new IMAGE_BRUSH( "Common/UpArrow2", Icon8x8 ) );
		Set( "Symbols.DownArrow", new IMAGE_BRUSH( "Common/DownArrow", Icon8x8 ) );
		Set( "Symbols.DoubleDownArrow", new IMAGE_BRUSH( "Common/DownArrow2", Icon8x8 ) );
		Set( "Symbols.RightArrow", new IMAGE_BRUSH("Common/SubmenuArrow", Icon8x8));
		Set( "Symbols.Check", new IMAGE_BRUSH( "Common/Check", Icon16x16 ) );
	}

	// Common icons
	{
		Set("Icons.Crop", new IMAGE_BRUSH_SVG("Starship/Common/Crop", Icon16x16));
		Set("Icons.Fullscreen", new IMAGE_BRUSH_SVG("Starship/Common/EnableFullscreen", Icon16x16));
		Set("Icons.Save", new IMAGE_BRUSH_SVG( "Starship/Common/SaveCurrent", Icon16x16 ) );
		Set("Icons.SaveChanged", new IMAGE_BRUSH_SVG( "Starship/Common/SaveChanged", Icon16x16 ) );

		Set("Icons.DirtyBadge", new IMAGE_BRUSH_SVG("Starship/Common/DirtyBadge", Icon12x12));
		Set("Icons.MakeStaticMesh", new IMAGE_BRUSH_SVG("Starship/Common/MakeStaticMesh", Icon16x16));
		Set("Icons.Documentation", new IMAGE_BRUSH_SVG("Starship/Common/Documentation", Icon16x16));
		Set("Icons.Support", new IMAGE_BRUSH_SVG("Starship/Common/Support", Icon16x16));
		Set("Icons.Package", new IMAGE_BRUSH_SVG("Starship/Common/ProjectPackage", Icon16x16));
		Set("Icons.Comment", new IMAGE_BRUSH_SVG("Starship/Common/Comment", Icon16x16));
		Set("Icons.SelectInViewport", new IMAGE_BRUSH_SVG("Starship/Common/SelectInViewport", Icon16x16));
		Set("Icons.BrowseContent", new IMAGE_BRUSH_SVG("Starship/Common/BrowseContent", Icon16x16));
		Set("Icons.Use", new IMAGE_BRUSH_SVG("Starship/Common/use-circle", Icon16x16));
		Set("Icons.Advanced", new IMAGE_BRUSH_SVG("Starship/Common/Advanced", Icon16x16));
		Set("Icons.Launch", new IMAGE_BRUSH_SVG("Starship/Common/ProjectLauncher", Icon16x16));
		Set("Icons.Next", new IMAGE_BRUSH_SVG("Starship/Common/NextArrow", Icon16x16));
		Set("Icons.Previous", new IMAGE_BRUSH_SVG("Starship/Common/PreviousArrow", Icon16x16));
		Set("Icons.Visibility", new IMAGE_BRUSH_SVG("Starship/Common/Visibility", Icon20x20));
		Set("Icons.World", new IMAGE_BRUSH_SVG("Starship/Common/World", Icon20x20));
		Set("Icons.Details", new IMAGE_BRUSH_SVG("Starship/Common/Details", Icon16x16));
		Set("Icons.Convert", new IMAGE_BRUSH_SVG("Starship/Common/convert", Icon20x20));
		Set("Icons.Adjust", new IMAGE_BRUSH_SVG("Starship/Common/Adjust", Icon16x16));
		Set("Icons.PlaceActors", new IMAGE_BRUSH_SVG("Starship/Common/PlaceActors", Icon16x16));
		Set("Icons.ReplaceActor", new IMAGE_BRUSH_SVG("Starship/Common/ReplaceActors", Icon16x16));
		Set("Icons.GroupActors", new IMAGE_BRUSH_SVG("Starship/Common/GroupActors", Icon16x16));
		Set("Icons.Transform", new IMAGE_BRUSH_SVG("Starship/Common/transform-local", Icon16x16));
		Set("Icons.SetShowPivot", new IMAGE_BRUSH_SVG("Starship/Common/SetShowPivot", Icon16x16));
		Set("Icons.Snap", new IMAGE_BRUSH_SVG("Starship/Common/Snap", Icon16x16));
		Set("Icons.Event", new IMAGE_BRUSH_SVG("Starship/Common/Event", Icon16x16));
		Set("Icons.JumpToEvent", new IMAGE_BRUSH_SVG("Starship/Common/JumpToEvent", Icon16x16));
		Set("Icons.Merge", new IMAGE_BRUSH_SVG("Starship/Common/Merge", Icon16x16));
		Set("Icons.Level", new IMAGE_BRUSH_SVG("Starship/Common/Levels", Icon16x16));
		Set("Icons.Play", new IMAGE_BRUSH_SVG("Starship/Common/play", Icon16x16));
		Set("Icons.Localization", new IMAGE_BRUSH_SVG("Starship/Common/LocalizationDashboard", Icon16x16));
		Set("Icons.Audit", new IMAGE_BRUSH_SVG("Starship/Common/AssetAudit", Icon16x16));
		Set("Icons.Blueprint", new IMAGE_BRUSH_SVG("Starship/Common/blueprint", Icon16x16));
		Set("Icons.Color", new IMAGE_BRUSH_SVG("Starship/Common/color", Icon16x16));
		Set("Icons.LOD", new IMAGE_BRUSH_SVG("Starship/Common/LOD", Icon16x16));
		Set("Icons.SkeletalMesh", new IMAGE_BRUSH_SVG("Starship/Common/SkeletalMesh", Icon16x16));
		Set("Icons.OpenInExternalEditor", new IMAGE_BRUSH_SVG("Starship/Common/OpenInExternalEditor", Icon16x16));
		Set("Icons.OpenSourceLocation", new IMAGE_BRUSH_SVG("Starship/Common/OpenSourceLocation", Icon16x16));
		Set("Icons.OpenInBrowser", new IMAGE_BRUSH_SVG("Starship/Common/WebBrowser", Icon16x16));
		Set("Icons.Find", new IMAGE_BRUSH_SVG("Starship/Common/Find", Icon16x16));
		Set("Icons.Validate", new IMAGE_BRUSH_SVG("Starship/Common/validate", Icon16x16));
		Set("Icons.Pinned", new IMAGE_BRUSH_SVG("Starship/Common/Pinned", Icon16x16));
		Set("Icons.Unpinned", new IMAGE_BRUSH_SVG("Starship/Common/Unpinned", Icon16x16));

		Set("Icons.Tree", new IMAGE_BRUSH_SVG("Starship/Common/WorldOutliner", Icon16x16));
		Set("Icons.Reset", new IMAGE_BRUSH_SVG("Starship/Common/Update", Icon16x16));
		Set("Icons.Box", new IMAGE_BRUSH_SVG("Starship/Common/ViewPerspective", Icon16x16));
		Set("Icons.Animation", new IMAGE_BRUSH_SVG("Starship/Common/Animation", Icon16x16));
		Set("Icons.Loop", new IMAGE_BRUSH_SVG("Starship/Common/Loop", Icon16x16));
		Set("Icons.Step", new IMAGE_BRUSH_SVG("Starship/MainToolbar/SingleFrameAdvance", Icon16x16));
		Set("Icons.Sequence", new IMAGE_BRUSH_SVG("Starship/AssetIcons/LevelSequenceActor_16", Icon16x16));

		Set("Icons.Function", new IMAGE_BRUSH_SVG("Starship/Blueprints/icon_Blueprint_Function", Icon16x16));
		Set("Icons.AddFunction", new IMAGE_BRUSH_SVG("Starship/Blueprints/icon_Blueprint_AddFunction", Icon16x16));
		Set("Icons.Pillset", new IMAGE_BRUSH_SVG("Starship/Blueprints/pillset", Icon16x16));

		Set("Icons.Log", new IMAGE_BRUSH_SVG("Starship/Common/Log", Icon16x16)); 

		Set("Icons.Toolbar.Play", new IMAGE_BRUSH_SVG("Starship/Common/play", Icon16x16, FStyleColors::AccentGreen));
		Set("Icons.Toolbar.Pause", new IMAGE_BRUSH_SVG("Starship/MainToolbar/pause", Icon16x16));
		Set("Icons.Toolbar.Stop", new IMAGE_BRUSH_SVG("Starship/MainToolbar/stop", Icon16x16, FStyleColors::AccentRed));
		Set("Icons.Toolbar.Settings", new CORE_IMAGE_BRUSH_SVG( "Starship/Common/Settings", Icon16x16));
		Set("Icons.Toolbar.Details", new IMAGE_BRUSH_SVG("Starship/Common/Details", Icon16x16));
		Set("Icons.Toolbar.EditorMode", new IMAGE_BRUSH_SVG("Starship/Common/EditorModes", Icon16x16));
		Set("Icons.Toolbar.FreeMode", new IMAGE_BRUSH_SVG("SodaIcons/car", Icon16x16));
	}

	// Soda Icons
	{
		Set("SodaIcons.AI", new IMAGE_BRUSH_SVG("SodaIcons/ai", Icon16x16));
		Set("SodaIcons.Car", new IMAGE_BRUSH_SVG("SodaIcons/car", Icon16x16));
		Set("SodaIcons.Keyboard", new IMAGE_BRUSH_SVG("SodaIcons/keyboard", Icon16x16));
		Set("SodaIcons.Point", new IMAGE_BRUSH_SVG("SodaIcons/point", Icon16x16));
		Set("SodaIcons.Terminal", new IMAGE_BRUSH_SVG("SodaIcons/terminal", Icon16x16));
		Set("SodaIcons.Action", new IMAGE_BRUSH_SVG("SodaIcons/action", Icon16x16));
		Set("SodaIcons.Compass", new IMAGE_BRUSH_SVG("SodaIcons/compass", Icon16x16));
		Set("SodaIcons.LapCount", new IMAGE_BRUSH_SVG("SodaIcons/lap-count", Icon16x16));
		Set("SodaIcons.RaceTrack", new IMAGE_BRUSH_SVG("SodaIcons/race-track", Icon16x16));
		Set("SodaIcons.Tool", new IMAGE_BRUSH_SVG("SodaIcons/tool", Icon16x16));
		Set("SodaIcons.Arrival", new IMAGE_BRUSH_SVG("SodaIcons/arrival", Icon16x16));
		Set("SodaIcons.Soda", new IMAGE_BRUSH_SVG("SodaIcons/soda", Icon16x16));
		Set("SodaIcons.Device", new IMAGE_BRUSH_SVG("SodaIcons/device", Icon16x16));
		Set("SodaIcons.Lidar", new IMAGE_BRUSH_SVG("SodaIcons/lidar", Icon16x16));
		Set("SodaIcons.Radar", new IMAGE_BRUSH_SVG("SodaIcons/radar", Icon16x16));
		Set("SodaIcons.Ultrasonic", new IMAGE_BRUSH_SVG("SodaIcons/ultrasonic", Icon16x16));
		Set("SodaIcons.Braking", new IMAGE_BRUSH_SVG("SodaIcons/braking", Icon16x16));
		Set("SodaIcons.External", new IMAGE_BRUSH_SVG("SodaIcons/external", Icon16x16));
		Set("SodaIcons.Mechanics", new IMAGE_BRUSH_SVG("SodaIcons/mechanics", Icon16x16));
		Set("SodaIcons.Road", new IMAGE_BRUSH_SVG("SodaIcons/road", Icon16x16));
		Set("SodaIcons.Van", new IMAGE_BRUSH_SVG("SodaIcons/van", Icon16x16));
		Set("SodaIcons.Bus", new IMAGE_BRUSH_SVG("SodaIcons/bus", Icon16x16));
		Set("SodaIcons.GPS", new IMAGE_BRUSH_SVG("SodaIcons/gps", Icon16x16));
		Set("SodaIcons.Modem", new IMAGE_BRUSH_SVG("SodaIcons/modem", Icon16x16));
		Set("SodaIcons.Route", new IMAGE_BRUSH_SVG("SodaIcons/route", Icon16x16));
		Set("SodaIcons.Weather", new IMAGE_BRUSH_SVG("SodaIcons/weather", Icon16x16));
		Set("SodaIcons.CAN", new IMAGE_BRUSH_SVG("SodaIcons/can", Icon16x16));
		Set("SodaIcons.Net", new IMAGE_BRUSH_SVG("SodaIcons/net", Icon16x16));
		Set("SodaIcons.IMU", new IMAGE_BRUSH_SVG("SodaIcons/imu", Icon16x16));
		Set("SodaIcons.Motor", new IMAGE_BRUSH_SVG("SodaIcons/motor", Icon16x16));
		Set("SodaIcons.Sensor", new IMAGE_BRUSH_SVG("SodaIcons/sensor", Icon16x16));
		Set("SodaIcons.Record", new IMAGE_BRUSH_SVG("SodaIcons/record", Icon16x16));
		Set("SodaIcons.Camera", new IMAGE_BRUSH_SVG("SodaIcons/camera", Icon16x16));
		Set("SodaIcons.Joystick", new IMAGE_BRUSH_SVG("SodaIcons/joystick", Icon16x16));
		Set("SodaIcons.Path", new IMAGE_BRUSH_SVG("SodaIcons/path", Icon16x16));
		Set("SodaIcons.Steering", new IMAGE_BRUSH_SVG("SodaIcons/steering", Icon16x16));
		Set("SodaIcons.CircleBoard", new IMAGE_BRUSH_SVG("SodaIcons/circle-board", Icon16x16));
		Set("SodaIcons.ArUco", new IMAGE_BRUSH_SVG("SodaIcons/aruco", Icon16x16));
		Set("SodaIcons.ChessBoard", new IMAGE_BRUSH_SVG("SodaIcons/chess-board", Icon16x16));
		Set("SodaIcons.Walking", new IMAGE_BRUSH_SVG("SodaIcons/walking", Icon16x16));
		Set("SodaIcons.Region", new IMAGE_BRUSH_SVG("SodaIcons/region", Icon16x16));
		Set("SodaIcons.Differential", new IMAGE_BRUSH_SVG("SodaIcons/differential", Icon16x16));
		Set("SodaIcons.Critical", new IMAGE_BRUSH_SVG("SodaIcons/critical", Icon16x16));
		Set("SodaIcons.Hourglass", new IMAGE_BRUSH_SVG("SodaIcons/hourglass", Icon16x16));
		Set("SodaIcons.WTF", new IMAGE_BRUSH_SVG("SodaIcons/wtf", Icon16x16));
		Set("SodaIcons.Bike", new IMAGE_BRUSH_SVG("SodaIcons/bike", Icon16x16));
		Set("SodaIcons.Motorbike", new IMAGE_BRUSH_SVG("SodaIcons/motorbike", Icon16x16));
		Set("SodaIcons.Tire", new IMAGE_BRUSH_SVG("SodaIcons/tire", Icon16x16));
		Set("SodaIcons.RoadSign", new IMAGE_BRUSH_SVG("SodaIcons/road-sign", Icon16x16));
		Set("SodaIcons.Forever", new IMAGE_BRUSH_SVG("SodaIcons/forever", Icon16x16));
		Set("SodaIcons.DownStream", new IMAGE_BRUSH_SVG("SodaIcons/down-stream", Icon16x16));
		Set("SodaIcons.DownStream32", new IMAGE_BRUSH_SVG("SodaIcons/down-stream", Icon32x32));
		Set("SodaIcons.GearBox", new IMAGE_BRUSH_SVG("SodaIcons/gear-box", Icon16x16));
		Set("SodaIcons.Hive", new IMAGE_BRUSH_SVG("SodaIcons/hive", Icon16x16));
		Set("SodaIcons.Python", new IMAGE_BRUSH_SVG("SodaIcons/python", Icon16x16));
		Set("SodaIcons.MongoDB", new IMAGE_BRUSH_SVG("SodaIcons/mongodb", Icon16x16));
		

		Set("SodaIcons.DB.DownloadBG", new IMAGE_BRUSH_SVG("Starship/DerivedData/DD_DownloadBG", Icon16x16));
		Set("SodaIcons.DB.Download", new IMAGE_BRUSH_SVG("Starship/DerivedData/DD_Download", Icon16x16, FLinearColor(0.3, 0.3, 0.3, 1.0)));
		Set("SodaIcons.DB.UploadBG", new IMAGE_BRUSH_SVG("Starship/DerivedData/DD_UploadBG", Icon16x16));
		Set("SodaIcons.DB.Upload", new IMAGE_BRUSH_SVG("Starship/DerivedData/DD_Upload", Icon16x16, FLinearColor(0.3, 0.3, 0.3, 1.0)));
		Set("SodaIcons.DB.UpDownBG", new IMAGE_BRUSH_SVG("Starship/DerivedData/DD_RemoteCache_UpDownBG", Icon16x16));
		Set("SodaIcons.DB.UpDown", new IMAGE_BRUSH_SVG("Starship/DerivedData/DD_RemoteCache_UpDown", Icon16x16, FLinearColor(0.3, 0.3, 0.3, 1.0)));
		Set("SodaIcons.DB.UpDownFull", new IMAGE_BRUSH_SVG("Starship/DerivedData/DD_RemoteCache_UpDownFull", Icon16x16));
		Set("SodaIcons.DB.IdleBG", new IMAGE_BRUSH_SVG("Starship/DerivedData/DD_RemoteCache_IdleBG", Icon16x16));
		Set("SodaIcons.DB.Idle", new IMAGE_BRUSH_SVG("Starship/DerivedData/DD_RemoteCache_Idle", Icon16x16, EStyleColor::AccentGreen));
		Set("SodaIcons.DB.WarningBG", new IMAGE_BRUSH_SVG("Starship/DerivedData/DD_RemoteCache_WarningBG", Icon16x16));
		Set("SodaIcons.DB.Warning", new IMAGE_BRUSH_SVG("Starship/DerivedData/DD_RemoteCache_Warning", Icon16x16, EStyleColor::Warning));
		Set("SodaIcons.DB.DisabledBG", new IMAGE_BRUSH_SVG("Starship/DerivedData/DD_RemoteCache_DisabledBG", Icon16x16));
		Set("SodaIcons.DB.Disabled", new IMAGE_BRUSH_SVG("Starship/DerivedData/DD_RemoteCache_Disabled", Icon16x16, FLinearColor(0.3, 0.3, 0.3, 1.0)));
		Set("SodaIcons.DB.DisabledFull", new IMAGE_BRUSH_SVG("Starship/DerivedData/DD_RemoteCache_DisabledFull", Icon16x16));
		Set("SodaIcons.DB.DD_Unavailable", new IMAGE_BRUSH_SVG("Starship/DerivedData/DD_RemoteCache_Unavailable", Icon16x16));
	}

	Set("UnrealDefaultThumbnail", new IMAGE_BRUSH("Starship/Common/Unreal_DefaultThumbnail", FVector2D(256, 256)));

	Set( "WarningStripe", new IMAGE_BRUSH( "Common/WarningStripe", FVector2D(20,6), FLinearColor::White, ESlateBrushTileType::Horizontal ) );
	
	Set("RoundedWarning", new FSlateRoundedBoxBrush(FStyleColors::Transparent, 4.0f, FStyleColors::Warning, 1.0f));
	Set("RoundedError", new FSlateRoundedBoxBrush(FStyleColors::Transparent, 4.0f, FStyleColors::Error, 1.0f));

	Set( "Button.Disabled", new BOX_BRUSH( "Common/Button_Disabled", 8.0f/32.0f ) );
	

	// Toggle button
	{
		Set( "ToggleButton", FButtonStyle(Button)
			.SetNormal(FSlateNoResource())
			.SetHovered(BOX_BRUSH( "Common/RoundedSelection_16x", 4.0f/16.0f, SelectionColor ))
			.SetPressed(BOX_BRUSH( "Common/RoundedSelection_16x", 4.0f/16.0f, SelectionColor_Pressed ))
		);

		//FSlateColorBrush(FLinearColor::White)

		Set("RoundButton", FButtonStyle(Button)
			.SetNormal(BOX_BRUSH("Common/RoundedSelection_16x", 4.0f / 16.0f, FLinearColor(1, 1, 1, 0.1f)))
			.SetHovered(BOX_BRUSH("Common/RoundedSelection_16x", 4.0f / 16.0f, SelectionColor))
			.SetPressed(BOX_BRUSH("Common/RoundedSelection_16x", 4.0f / 16.0f, SelectionColor_Pressed))
			);

		Set("FlatButton", FButtonStyle(Button)
			.SetNormal(FSlateNoResource())
			.SetHovered(BOX_BRUSH("Common/FlatButton", 2.0f / 8.0f, SelectionColor))
			.SetPressed(BOX_BRUSH("Common/FlatButton", 2.0f / 8.0f, SelectionColor_Pressed))
			);

		Set("FlatButton.Dark", FButtonStyle(Button)
			.SetNormal(BOX_BRUSH("Common/FlatButton", 2.0f / 8.0f, FLinearColor(0.125f, 0.125f, 0.125f, 0.8f)))
			.SetHovered(BOX_BRUSH("Common/FlatButton", 2.0f / 8.0f, SelectionColor))
			.SetPressed(BOX_BRUSH("Common/FlatButton", 2.0f / 8.0f, SelectionColor_Pressed))
			);

		Set("FlatButton.DarkGrey", FButtonStyle(Button)
			.SetNormal(BOX_BRUSH("Common/FlatButton", 2.0f / 8.0f, FLinearColor(0.05f, 0.05f, 0.05f, 0.8f)))
			.SetHovered(BOX_BRUSH("Common/FlatButton", 2.0f / 8.0f, SelectionColor))
			.SetPressed(BOX_BRUSH("Common/FlatButton", 2.0f / 8.0f, SelectionColor_Pressed))
		);


		Set("FlatButton.Default", GetWidgetStyle<FButtonStyle>("FlatButton.Dark"));

		Set("FlatButton.DefaultTextStyle", FTextBlockStyle(NormalText)
			.SetFont(DEFAULT_FONT("Bold", 10))
			.SetColorAndOpacity(FLinearColor(1.0f, 1.0f, 1.0f))
			.SetHighlightColor(FLinearColor(1.0f, 1.0f, 1.0f))
			.SetShadowOffset(FVector2D(1, 1))
			.SetShadowColorAndOpacity(FLinearColor(0, 0, 0, 0.9f)));

		struct ButtonColor
		{
		public:
			FName Name;
			FLinearColor Normal;
			FLinearColor Hovered;
			FLinearColor Pressed;

			ButtonColor(const FName& InName, const FLinearColor& Color) : Name(InName)
			{
				Normal = Color * 0.8f;
				Normal.A = Color.A;
				Hovered = Color * 1.0f;
				Hovered.A = Color.A;
				Pressed = Color * 0.6f;
				Pressed.A = Color.A;
			}
		};

		TArray< ButtonColor > FlatButtons;
		FlatButtons.Add(ButtonColor("FlatButton.Primary", FLinearColor(0.02899, 0.19752, 0.48195)));
		FlatButtons.Add(ButtonColor("FlatButton.Success", FLinearColor(0.10616, 0.48777, 0.10616)));
		FlatButtons.Add(ButtonColor("FlatButton.Info", FLinearColor(0.10363, 0.53564, 0.7372)));
		FlatButtons.Add(ButtonColor("FlatButton.Warning", FLinearColor(0.87514, 0.42591, 0.07383)));
		FlatButtons.Add(ButtonColor("FlatButton.Danger", FLinearColor(0.70117, 0.08464, 0.07593)));

		for ( const ButtonColor& Entry : FlatButtons )
		{
			Set(Entry.Name, FButtonStyle(Button)
				.SetNormal(BOX_BRUSH("Common/FlatButton", 2.0f / 8.0f, Entry.Normal))
				.SetHovered(BOX_BRUSH("Common/FlatButton", 2.0f / 8.0f, Entry.Hovered))
				.SetPressed(BOX_BRUSH("Common/FlatButton", 2.0f / 8.0f, Entry.Pressed))
				);
		}

		Set("FontAwesome.7", ICON_FONT(7));
		Set("FontAwesome.8", ICON_FONT(8));
		Set("FontAwesome.9", ICON_FONT(9));
		Set("FontAwesome.10", ICON_FONT(10));
		Set("FontAwesome.11", ICON_FONT(11));
		Set("FontAwesome.12", ICON_FONT(12));
		Set("FontAwesome.14", ICON_FONT(14));
		Set("FontAwesome.16", ICON_FONT(16));
		Set("FontAwesome.18", ICON_FONT(18));

		/* Create a checkbox style for "ToggleButton" but with the images used by a normal checkbox (see "Checkbox" below) ... */
		const FCheckBoxStyle CheckboxLookingToggleButtonStyle = FCheckBoxStyle()
			.SetCheckBoxType(ESlateCheckBoxType::ToggleButton)
			.SetUncheckedImage( IMAGE_BRUSH( "Common/CheckBox", Icon16x16 ) )
			.SetUncheckedHoveredImage( IMAGE_BRUSH( "Common/CheckBox", Icon16x16 ) )
			.SetUncheckedPressedImage( IMAGE_BRUSH( "Common/CheckBox_Hovered", Icon16x16, FLinearColor( 0.5f, 0.5f, 0.5f ) ) )
			.SetCheckedImage( IMAGE_BRUSH( "Common/CheckBox_Checked_Hovered", Icon16x16 ) )
			.SetCheckedHoveredImage( IMAGE_BRUSH( "Common/CheckBox_Checked_Hovered", Icon16x16, FLinearColor( 0.5f, 0.5f, 0.5f ) ) )
			.SetCheckedPressedImage( IMAGE_BRUSH( "Common/CheckBox_Checked", Icon16x16 ) )
			.SetUndeterminedImage( IMAGE_BRUSH( "Common/CheckBox_Undetermined", Icon16x16 ) )
			.SetUndeterminedHoveredImage( IMAGE_BRUSH( "Common/CheckBox_Undetermined_Hovered", Icon16x16 ) )
			.SetUndeterminedPressedImage( IMAGE_BRUSH( "Common/CheckBox_Undetermined_Hovered", Icon16x16, FLinearColor( 0.5f, 0.5f, 0.5f ) ) )
			.SetPadding(1.0f);
		/* ... and set new style */
		Set( "CheckboxLookToggleButtonCheckbox", CheckboxLookingToggleButtonStyle );


		Set( "ToggleButton.LabelFont", DEFAULT_FONT( "Regular", 9 ) );
		Set( "ToggleButtonCheckbox.LabelFont", DEFAULT_FONT( "Regular", 9 ) );
	}

	// Combo Button, Combo Box
	{
		// Legacy style; still being used by some editor widgets
		Set( "ComboButton.Arrow", new IMAGE_BRUSH("Common/ComboArrow", Icon8x8 ) );


		FComboButtonStyle ToolbarComboButton = FComboButtonStyle()
			.SetButtonStyle( GetWidgetStyle<FButtonStyle>( "ToggleButton" ) )
			.SetDownArrowImage( IMAGE_BRUSH( "Common/ShadowComboArrow", Icon8x8 ) )
			.SetMenuBorderBrush(FSlateNoResource())
			.SetMenuBorderPadding( FMargin( 0.0f ) );
		Set( "ToolbarComboButton", ToolbarComboButton );

		Set("GenericFilters.ComboButtonStyle", ToolbarComboButton);

		Set("GenericFilters.TextStyle", FTextBlockStyle(NormalText)
			.SetFont(DEFAULT_FONT("Bold", 9))
			.SetColorAndOpacity(FLinearColor(1.0f, 1.0f, 1.0f, 0.9f))
			.SetShadowOffset(FVector2D(1, 1))
			.SetShadowColorAndOpacity(FLinearColor(0, 0, 0, 0.9f)));

	}

	// Error Reporting
	{
		Set( "InfoReporting.BackgroundColor", FLinearColor(0.1f, 0.33f, 1.0f));

	}

	// EditableTextBox
	{
		Set( "EditableTextBox.Background.Normal", new BOX_BRUSH( "Common/TextBox", FMargin(4.0f/16.0f) ) );
		Set( "EditableTextBox.Background.Hovered", new BOX_BRUSH( "Common/TextBox_Hovered", FMargin(4.0f/16.0f) ) );
		Set( "EditableTextBox.Background.Focused", new BOX_BRUSH( "Common/TextBox_Hovered", FMargin(4.0f/16.0f) ) );
		Set( "EditableTextBox.Background.ReadOnly", new BOX_BRUSH( "Common/TextBox_ReadOnly", FMargin(4.0f/16.0f) ) );
		Set( "EditableTextBox.BorderPadding", FMargin(4.0f, 2.0f) );
	}

	// EditableTextBox Special
	{
		FSlateBrush* SpecialEditableTextImageNormal = new BOX_BRUSH( "Common/TextBox_Special", FMargin(8.0f/32.0f) );
		Set( "SpecialEditableTextImageNormal", SpecialEditableTextImageNormal );

		const FEditableTextBoxStyle SpecialEditableTextBoxStyle = FEditableTextBoxStyle()
			.SetBackgroundImageNormal( *SpecialEditableTextImageNormal )
			.SetBackgroundImageHovered( BOX_BRUSH( "Common/TextBox_Special_Hovered", FMargin(8.0f/32.0f) ) )
			.SetBackgroundImageFocused( BOX_BRUSH( "Common/TextBox_Special_Hovered", FMargin(8.0f/32.0f) ) )
			.SetBackgroundImageReadOnly( BOX_BRUSH( "Common/TextBox_ReadOnly", FMargin(4.0f/16.0f) ) )
			.SetScrollBarStyle( ScrollBar );
		Set( "SpecialEditableTextBox", SpecialEditableTextBoxStyle );

		Set( "SearchBox.ActiveBorder", new BOX_BRUSH( "Common/TextBox_Special_Active", FMargin(8.0f/32.0f) ) );
	}

	// Filtering/Searching feedback
	{
		const FLinearColor ActiveFilterColor = FLinearColor(1.0f,0.55f,0.0f,1.0f);
		Set("Searching.SearchActiveTab",    new FSlateNoResource());
		Set("Searching.SearchActiveBorder", new FSlateRoundedBoxBrush(FLinearColor::Transparent, 0.0, FStyleColors::Primary, 1.f));
	}

	// Images sizes are specified in Slate Screen Units. These do not necessarily correspond to pixels!
	// An IMAGE_BRUSH( "SomeImage", FVector2D(32,32)) will have a desired size of 16x16 Slate Screen Units.
	// This allows the original resource to be scaled up or down as needed.

	Set( "WhiteTexture", new IMAGE_BRUSH( "Old/White", Icon16x16 ) );

	Set( "BoldFont", DEFAULT_FONT( "Bold", FStarshipCoreStyle::RegularTextSize ) );

	Set( "MarqueeSelection", new BORDER_BRUSH( "Old/DashedBorder", FMargin(6.0f/32.0f) ) );

	Set( "Border", new BOX_BRUSH( "Old/Border", 4.0f/16.0f ) );

	Set( "FilledBorder", new BOX_BRUSH( "Old/FilledBorder", 4.0f/16.0f ) );

	Set("GenericLink", new IMAGE_BRUSH("Common/link", Icon16x16));


	{
		// Dark Hyperlink - for use on light backgrounds
		FButtonStyle DarkHyperlinkButton = FButtonStyle()
			.SetNormal ( BORDER_BRUSH( "Old/HyperlinkDotted", FMargin(0,0,0,3/16.0f), FLinearColor::Black ) )
			.SetPressed( FSlateNoResource() )
			.SetHovered( BORDER_BRUSH( "Old/HyperlinkUnderline", FMargin(0,0,0,3/16.0f), FLinearColor::Black ) );
		FHyperlinkStyle DarkHyperlink = FHyperlinkStyle()
			.SetUnderlineStyle(DarkHyperlinkButton)
			.SetTextStyle(NormalText)
			.SetPadding(FMargin(0.0f));
		Set("DarkHyperlink", DarkHyperlink);

		// Visible on hover hyper link
		FButtonStyle HoverOnlyHyperlinkButton = FButtonStyle()
			.SetNormal(FSlateNoResource() )
			.SetPressed(FSlateNoResource() )
			.SetHovered(BORDER_BRUSH( "Old/HyperlinkUnderline", FMargin(0,0,0,3/16.0f) ) );
		Set("HoverOnlyHyperlinkButton", HoverOnlyHyperlinkButton);

		FHyperlinkStyle HoverOnlyHyperlink = FHyperlinkStyle()
			.SetUnderlineStyle(HoverOnlyHyperlinkButton)
			.SetTextStyle(NormalText)
			.SetPadding(FMargin(0.0f));
		Set("HoverOnlyHyperlink", HoverOnlyHyperlink);
	}

	Set( "DashedBorder", new BORDER_BRUSH( "Old/DashedBorder", FMargin(6.0f/32.0f) ) );

	Set( "UniformShadow", new BORDER_BRUSH( "Common/UniformShadow", FMargin( 16.0f / 64.0f ) ) );
	Set( "UniformShadow_Tint", new BORDER_BRUSH( "Common/UniformShadow_Tint", FMargin( 16.0f / 64.0f ) ) );

	{
		Set ("SplitterDark", FSplitterStyle()
			.SetHandleNormalBrush( FSlateColorBrush( FLinearColor(FColor( 32, 32, 32) ) ) )
			.SetHandleHighlightBrush( FSlateColorBrush( FLinearColor(FColor( 96, 96, 96) ) ) ) 
			);
	}

	// Lists, Trees
	{

		const FTableViewStyle DefaultTreeViewStyle = FTableViewStyle()
			.SetBackgroundBrush(FSlateColorBrush(FStyleColors::Recessed));
		Set("ListView", DefaultTreeViewStyle);

		const FTableViewStyle DefaultTableViewStyle = FTableViewStyle()
			.SetBackgroundBrush(FSlateColorBrush(FStyleColors::Recessed));
		Set("TreeView", DefaultTableViewStyle);

		Set( "TableView.Row", FTableRowStyle( NormalTableRowStyle) );
		Set( "TableView.DarkRow",FTableRowStyle( NormalTableRowStyle)
			.SetEvenRowBackgroundBrush(IMAGE_BRUSH("PropertyView/DetailCategoryMiddle", FVector2D(16, 16)))
			.SetEvenRowBackgroundHoveredBrush(IMAGE_BRUSH("PropertyView/DetailCategoryMiddle_Hovered", FVector2D(16, 16)))
			.SetOddRowBackgroundBrush(IMAGE_BRUSH("PropertyView/DetailCategoryMiddle", FVector2D(16, 16)))
			.SetOddRowBackgroundHoveredBrush(IMAGE_BRUSH("PropertyView/DetailCategoryMiddle_Hovered", FVector2D(16, 16)))
			.SetSelectorFocusedBrush(BORDER_BRUSH("Common/Selector", FMargin(4.f / 16.f), SelectorColor))
			.SetActiveBrush(IMAGE_BRUSH("Common/Selection", Icon8x8, SelectionColor))
			.SetActiveHoveredBrush(IMAGE_BRUSH("Common/Selection", Icon8x8, SelectionColor))
			.SetInactiveBrush(IMAGE_BRUSH("Common/Selection", Icon8x8, SelectionColor_Inactive))
			.SetInactiveHoveredBrush(IMAGE_BRUSH("Common/Selection", Icon8x8, SelectionColor_Inactive))
		);
		Set("TableView.NoHoverTableRow", FTableRowStyle(NormalTableRowStyle)
			.SetEvenRowBackgroundHoveredBrush(FSlateNoResource())
			.SetOddRowBackgroundHoveredBrush(FSlateNoResource())
			.SetActiveHoveredBrush(FSlateNoResource())
			.SetInactiveHoveredBrush(FSlateNoResource())
			);
	}
	
	// Spinboxes
	{

		// Legacy styles; used by other editor widgets
		Set( "SpinBox.Background", new BOX_BRUSH( "Common/Spinbox", FMargin(4.0f/16.0f) ) );
		Set( "SpinBox.Background.Hovered", new BOX_BRUSH( "Common/Spinbox_Hovered", FMargin(4.0f/16.0f) ) );
		Set( "SpinBox.Fill", new BOX_BRUSH( "Common/Spinbox_Fill", FMargin(4.0f/16.0f, 4.0f/16.0f, 8.0f/16.0f, 4.0f/16.0f) ) );
		Set( "SpinBox.Fill.Hovered", new BOX_BRUSH( "Common/Spinbox_Fill_Hovered", FMargin(4.0f/16.0f) ) );
		Set( "SpinBox.Arrows", new IMAGE_BRUSH( "Common/SpinArrows", Icon12x12 ) );
		Set( "SpinBox.TextMargin", FMargin(1.0f,2.0f) );
	}	

	// Message Log
	{
		Set( "MessageLog.Action", new IMAGE_BRUSH( "Icons/icon_file_choosepackages_16px", Icon16x16) );
		Set( "MessageLog.Docs", new IMAGE_BRUSH( "Icons/icon_Docs_16x", Icon16x16) );
		Set( "MessageLog.Tutorial", new IMAGE_BRUSH( "Icons/icon_Blueprint_Enum_16x", Icon16x16 ) );
		Set( "MessageLog.Url", new IMAGE_BRUSH( "Icons/icon_world_16x", Icon16x16 ) );
		Set( "MessageLog.TabIcon", new IMAGE_BRUSH_SVG( "Starship/Common/MessageLog", Icon16x16 ) );
		Set( "MessageLog.ListBorder", new BOX_BRUSH( "/Docking/AppTabContentArea", FMargin(4/16.0f) ) );
	}

	// Output Log Window
	{
		const int32 LogFontSize = Settings.IsValid() ? Settings->LogFontSize : 9;

		const FTextBlockStyle NormalLogText = FTextBlockStyle(NormalText)
			.SetFont(DEFAULT_FONT("Mono", LogFontSize))
			.SetColorAndOpacity(LogColor_Normal)
			.SetSelectedBackgroundColor(LogColor_SelectionBackground)
			.SetHighlightColor(FStyleColors::Black);

		Set("Log.Normal", NormalLogText );

		Set("Log.Command", FTextBlockStyle(NormalLogText)
			.SetColorAndOpacity( LogColor_Command )
			);

		Set("Log.Warning", FTextBlockStyle(NormalLogText)
			.SetColorAndOpacity(FStyleColors::Warning)
			);

		Set("Log.Error", FTextBlockStyle(NormalLogText)
			.SetColorAndOpacity(FStyleColors::Error)
			);

		Set("Log.TabIcon", new IMAGE_BRUSH_SVG( "Starship/Common/OutputLog", Icon16x16 ) );

		Set("Log.TextBox", FEditableTextBoxStyle(NormalEditableTextBoxStyle)
			.SetBackgroundImageNormal( BOX_BRUSH( "Common/WhiteGroupBorder", FMargin(4.0f/16.0f) ) )
			.SetBackgroundImageHovered( BOX_BRUSH( "Common/WhiteGroupBorder", FMargin(4.0f/16.0f) ) )
			.SetBackgroundImageFocused( BOX_BRUSH( "Common/WhiteGroupBorder", FMargin(4.0f/16.0f) ) )
			.SetBackgroundImageReadOnly( BOX_BRUSH( "Common/WhiteGroupBorder", FMargin(4.0f/16.0f) ) )
			.SetBackgroundColor(FStyleColors::Recessed)
			);

		Set("DebugConsole.Background", new FSlateNoResource());

		const FButtonStyle DebugConsoleButton = FButtonStyle(FStarshipCoreStyle::GetCoreStyle().GetWidgetStyle<FButtonStyle>("NoBorder"))
			.SetNormalForeground(FStyleColors::Foreground)
			.SetNormalPadding(FMargin(2, 2, 2, 2))
			.SetPressedPadding(FMargin(2, 3, 2, 1));

		const FComboButtonStyle DebugConsoleComboButton = FComboButtonStyle(FStarshipCoreStyle::GetCoreStyle().GetWidgetStyle<FComboButtonStyle>("ComboButton"))
			.SetDownArrowImage(FSlateNoResource())
			.SetButtonStyle(DebugConsoleButton);

		Set("DebugConsole.ComboButton", DebugConsoleComboButton);

		Set("DebugConsole.Icon", new IMAGE_BRUSH_SVG("Starship/Common/Console", Icon16x16));
	}

	// Output Log Window (override)
	{
		
		const int32 LogFontSize = Settings.IsValid() ? Settings->LogFontSize : 9;

		const FTextBlockStyle NormalLogText = FTextBlockStyle(NormalText)
			.SetFont(DEFAULT_FONT("Mono", LogFontSize))
			.SetColorAndOpacity(FStyleColors::Foreground)
			.SetSelectedBackgroundColor(FStyleColors::Highlight)
			.SetHighlightColor(FStyleColors::Black);

		Set("Log.Normal", NormalLogText);

		Set("Log.Command", FTextBlockStyle(NormalLogText)
			.SetColorAndOpacity(FStyleColors::AccentGreen)
		);

		Set("Log.Warning", FTextBlockStyle(NormalLogText)
			.SetColorAndOpacity(FStyleColors::Warning)
		);

		Set("Log.Error", FTextBlockStyle(NormalLogText)
			.SetColorAndOpacity(FStyleColors::Error)
		);

		Set("DebugConsole.Icon", new IMAGE_BRUSH_SVG("Starship/Common/Console", Icon16x16));
		
	}

	// Main frame
	{
		Set( "MainFrame.AutoSaveImage", 	       new IMAGE_BRUSH_SVG( "Starship/Common/SaveCurrent", Icon16x16 ) );
		Set( "MainFrame.SaveAll",                  new IMAGE_BRUSH_SVG( "Starship/Common/SaveAll", Icon16x16 ) );
		Set( "MainFrame.ChoosePackagesToSave",     new IMAGE_BRUSH_SVG( "Starship/Common/SaveChoose", Icon16x16 ) );
		Set( "MainFrame.NewProject",               new IMAGE_BRUSH_SVG( "Starship/Common/ProjectNew", Icon16x16 ) );
		Set( "MainFrame.OpenProject",              new IMAGE_BRUSH_SVG( "Starship/Common/ProjectOpen", Icon16x16 ) );
		Set( "MainFrame.AddCodeToProject",         new IMAGE_BRUSH_SVG( "Starship/Common/ProjectC++", Icon16x16 ) );
		Set( "MainFrame.Exit",                     new IMAGE_BRUSH_SVG( "Starship/Common/Exit", Icon16x16 ) );
		Set( "MainFrame.CookContent",              new IMAGE_BRUSH_SVG( "Starship/Common/CookContent", Icon16x16 ) );
		Set( "MainFrame.OpenVisualStudio",         new IMAGE_BRUSH_SVG( "Starship/Common/VisualStudio", Icon16x16 ) );
		Set( "MainFrame.RefreshVisualStudio",      new IMAGE_BRUSH_SVG( "Starship/Common/RefreshVisualStudio", Icon16x16 ) );
		Set( "MainFrame.OpenSourceCodeEditor",     new IMAGE_BRUSH_SVG( "Starship/Common/SourceCodeEditor", Icon16x16));
		Set( "MainFrame.RefreshSourceCodeEditor",  new IMAGE_BRUSH_SVG( "Starship/Common/RefreshSourceCodeEditor", Icon16x16));
		Set( "MainFrame.PackageProject",           new IMAGE_BRUSH_SVG( "Starship/Common/ProjectPackage", Icon16x16 ) );
		Set( "MainFrame.RecentProjects",           new IMAGE_BRUSH_SVG( "Starship/Common/ProjectsRecent", Icon16x16 ) );
		Set( "MainFrame.RecentLevels",             new IMAGE_BRUSH_SVG( "Starship/Common/LevelRecent", Icon16x16 ) );
		Set( "MainFrame.FavoriteLevels",           new IMAGE_BRUSH_SVG( "Starship/Common/LevelFavorite", Icon16x16 ) );
		Set( "MainFrame.ZipUpProject", 		       new IMAGE_BRUSH_SVG( "Starship/Common/ZipProject", Icon16x16 ) );

		Set( "MainFrame.ChooseFilesToSave",       new IMAGE_BRUSH_SVG( "Starship/Common/SaveChoose", Icon16x16 ) );
		Set( "MainFrame.ConnectToSourceControl",  new CORE_IMAGE_BRUSH_SVG("Starship/SourceControl/SourceControl", Icon16x16) );
		Set( "MainFrame.OpenMarketplace",			new IMAGE_BRUSH_SVG("Starship/MainToolbar/marketplace", Icon16x16));

		Set( "MainFrame.DebugTools.SmallFont", DEFAULT_FONT( "Regular", 8 ) );
		Set( "MainFrame.DebugTools.NormalFont", DEFAULT_FONT( "Regular", 9 ) );
		Set( "MainFrame.DebugTools.LabelFont", DEFAULT_FONT( "Regular", 8 ) );
	}

	// Main frame
	{
		Set("MainFrame.StatusInfoButton", FButtonStyle(Button)
			.SetNormal( IMAGE_BRUSH( "Icons/StatusInfo_16x", Icon16x16 ) )
			.SetHovered( IMAGE_BRUSH( "Icons/StatusInfo_16x", Icon16x16 ) )
			.SetPressed( IMAGE_BRUSH( "Icons/StatusInfo_16x", Icon16x16 ) )
			.SetNormalPadding(0)
			.SetPressedPadding(0)
		);
	}

	{
		Set("Level.VisibleIcon16x", new CORE_IMAGE_BRUSH_SVG("Starship/Common/visible", Icon16x16));
		Set("Level.VisibleHighlightIcon16x", new CORE_IMAGE_BRUSH_SVG("Starship/Common/visible", Icon16x16));
		Set("Level.NotVisibleIcon16x", new CORE_IMAGE_BRUSH_SVG("Starship/Common/hidden", Icon16x16));
		Set("Level.NotVisibleHighlightIcon16x", new CORE_IMAGE_BRUSH_SVG("Starship/Common/hidden", Icon16x16));
	}

	// Mode ToolPalette 
	{

		FToolBarStyle PaletteToolBarStyle = FStarshipCoreStyle::GetCoreStyle().GetWidgetStyle<FToolBarStyle>("SlimToolBar");

		FTextBlockStyle PaletteToolbarLabelStyle = FTextBlockStyle(GetParentStyle()->GetWidgetStyle<FTextBlockStyle>("SmallText"));
		PaletteToolbarLabelStyle.SetOverflowPolicy(ETextOverflowPolicy::Ellipsis);

		PaletteToolBarStyle.SetLabelStyle(PaletteToolbarLabelStyle);

		PaletteToolBarStyle.SetBackground(FSlateColorBrush(FStyleColors::Recessed));

		PaletteToolBarStyle.SetLabelPadding(FMargin(0.0f, 1.0f, 0.0f, 0.0f));

		PaletteToolBarStyle.SetButtonPadding(FMargin(0.0f, 0.0f));
		PaletteToolBarStyle.SetCheckBoxPadding(FMargin(0.0f, 0.0f));
		PaletteToolBarStyle.SetComboButtonPadding(FMargin(0.0f, 0.0f));
		PaletteToolBarStyle.SetIndentedBlockPadding(FMargin(0.0f, 0.0f));
		PaletteToolBarStyle.SetBlockPadding(FMargin(0.0f, 0.0f));
		PaletteToolBarStyle.ToggleButton.SetPadding(FMargin(0.0f, 6.0f));
		PaletteToolBarStyle.ButtonStyle.SetNormalPadding(FMargin(2.0f, 6.0f));
		PaletteToolBarStyle.ButtonStyle.SetPressedPadding(FMargin(2.0f, 6.0f));

		Set("PaletteToolBar.Tab", FCheckBoxStyle()
			.SetCheckBoxType(ESlateCheckBoxType::ToggleButton)

			.SetCheckedImage(FSlateRoundedBoxBrush(FStyleColors::Primary, 2.0f))
			.SetCheckedHoveredImage(FSlateRoundedBoxBrush(FStyleColors::PrimaryHover, 2.0f))
			.SetCheckedPressedImage(FSlateRoundedBoxBrush(FStyleColors::Dropdown, 2.0f))

			.SetUncheckedImage(FSlateRoundedBoxBrush(FStyleColors::Secondary, 2.0f))
			.SetUncheckedHoveredImage(FSlateRoundedBoxBrush(FStyleColors::Hover, 2.0f))
			.SetUncheckedPressedImage(FSlateRoundedBoxBrush(FStyleColors::Secondary, 2.0f))

			.SetForegroundColor(FStyleColors::Foreground)
			.SetHoveredForegroundColor(FStyleColors::ForegroundHover)
			.SetPressedForegroundColor(FStyleColors::ForegroundHover)
			.SetCheckedForegroundColor(FStyleColors::ForegroundHover)
			.SetCheckedHoveredForegroundColor(FStyleColors::ForegroundHover)
			.SetCheckedPressedForegroundColor(FStyleColors::ForegroundHover)
			.SetPadding(FMargin(2.f, 6.f))
		);

		Set("PaletteToolBar.MaxUniformToolbarSize", 48.f);
		Set("PaletteToolBar.MinUniformToolbarSize", 48.f);

		Set("PaletteToolBar.ExpandableAreaHeader", new FSlateRoundedBoxBrush(FStyleColors::Dropdown, FVector4(4.0, 4.0, 0.0, 0.0)));
		Set("PaletteToolBar.ExpandableAreaBody", new FSlateRoundedBoxBrush(FStyleColors::Recessed, FVector4(0.0, 0.0, 4.0, 4.0)));


		Set("PaletteToolBar", PaletteToolBarStyle);

		Set("EditorModesPanel.CategoryFontStyle", DEFAULT_FONT("Bold", 10));
		Set("EditorModesPanel.ToolDescriptionFont", DEFAULT_FONT("Italic", 10));

	}

	// Vertical ToolPalette 
	{
		FToolBarStyle VerticalToolBarStyle = FStarshipCoreStyle::GetCoreStyle().GetWidgetStyle<FToolBarStyle>("SlimToolBar");

		FTextBlockStyle VerticalToolBarLabelStyle = FTextBlockStyle(GetParentStyle()->GetWidgetStyle<FTextBlockStyle>("SmallText"));
		VerticalToolBarLabelStyle.SetOverflowPolicy(ETextOverflowPolicy::Ellipsis);

		VerticalToolBarStyle.SetLabelStyle(VerticalToolBarLabelStyle);

		VerticalToolBarStyle.SetBackground(FSlateColorBrush(FStyleColors::Recessed));

		VerticalToolBarStyle.SetLabelPadding(FMargin(0.0f, 1.0f, 0.0f, 0.0f));

		VerticalToolBarStyle.SetButtonPadding(FMargin(0.0f, 0.0f));
		VerticalToolBarStyle.SetCheckBoxPadding(FMargin(0.0f, 0.0f));
		VerticalToolBarStyle.SetComboButtonPadding(FMargin(0.0f, 0.0f));
		VerticalToolBarStyle.SetIndentedBlockPadding(FMargin(0.0f, 0.0f));
		VerticalToolBarStyle.SetBlockPadding(FMargin(0.0f, 0.0f));
		VerticalToolBarStyle.SetBackgroundPadding(FMargin(4.0f, 2.0f));
		VerticalToolBarStyle.ToggleButton.SetPadding(FMargin(0.0f, 6.0f));
		VerticalToolBarStyle.ButtonStyle.SetNormalPadding(FMargin(2.0f, 6.0f));
		VerticalToolBarStyle.ButtonStyle.SetPressedPadding(FMargin(2.0f, 6.0f));

		Set("VerticalToolBar.Tab", FCheckBoxStyle()
			.SetCheckBoxType(ESlateCheckBoxType::ToggleButton)

			.SetCheckedImage(FSlateRoundedBoxBrush(FStyleColors::Input, 2.0f))
			.SetCheckedHoveredImage(FSlateRoundedBoxBrush(FStyleColors::Input, 2.0f))
			.SetCheckedPressedImage(FSlateRoundedBoxBrush(FStyleColors::Input, 2.0f))

			.SetUncheckedImage(FSlateRoundedBoxBrush(FStyleColors::Secondary, 2.0f))
			.SetUncheckedHoveredImage(FSlateRoundedBoxBrush(FStyleColors::Hover, 2.0f))
			.SetUncheckedPressedImage(FSlateRoundedBoxBrush(FStyleColors::Secondary, 2.0f))

			.SetForegroundColor(FStyleColors::Foreground)
			.SetHoveredForegroundColor(FStyleColors::ForegroundHover)
			.SetPressedForegroundColor(FStyleColors::ForegroundHover)
			.SetCheckedForegroundColor(FStyleColors::Primary)
			.SetCheckedHoveredForegroundColor(FStyleColors::PrimaryHover)
			.SetPadding(FMargin(2.f, 6.f))
		);

		Set("VerticalToolBar.MaxUniformToolbarSize", 48.f);
		Set("VerticalToolBar.MinUniformToolbarSize", 48.f);

		Set("VerticalToolBar.ExpandableAreaHeader", new FSlateRoundedBoxBrush(FStyleColors::Dropdown, FVector4(4.0, 4.0, 0.0, 0.0)));
		Set("VerticalToolBar.ExpandableAreaBody", new FSlateRoundedBoxBrush(FStyleColors::Recessed, FVector4(0.0, 0.0, 4.0, 4.0)));

		Set("VerticalToolBar", VerticalToolBarStyle);
	}
}

void FStarshipSodaStyle::FStyle::SetupViewportStyles()
{
	{
		FToolBarStyle ViewportToolbarStyle = FStarshipCoreStyle::GetCoreStyle().GetWidgetStyle<FToolBarStyle>("SlimToolBar");

		FMargin ViewportMarginLeft(6.f, 4.f, 3.f, 4.f);
		FMargin ViewportMarginCenter(6.f, 4.f, 3.f, 4.f);
		FMargin ViewportMarginRight(4.f, 4.f, 5.f, 4.f);

		const FCheckBoxStyle ViewportToggleButton = FCheckBoxStyle()
			.SetCheckBoxType(ESlateCheckBoxType::ToggleButton)
			.SetCheckedImage(FSlateNoResource())
			.SetCheckedHoveredImage(FSlateNoResource())
			.SetCheckedPressedImage(FSlateNoResource())
			.SetUncheckedImage(FSlateNoResource())
			.SetUncheckedHoveredImage(FSlateNoResource())
			.SetUncheckedPressedImage(FSlateNoResource())
			.SetForegroundColor(FStyleColors::Foreground)
			.SetHoveredForegroundColor(FStyleColors::ForegroundHover)
			.SetPressedForegroundColor(FStyleColors::ForegroundHover)
			.SetCheckedForegroundColor(FStyleColors::ForegroundHover)
			.SetCheckedHoveredForegroundColor(FStyleColors::ForegroundHover)
			.SetPadding(0);


		FLinearColor ToolbarBackgroundColor = FStyleColors::Dropdown.GetSpecifiedColor();
		ToolbarBackgroundColor.A = .80f;

		FLinearColor ToolbarPressedColor = FStyleColors::Recessed.GetSpecifiedColor();
		ToolbarPressedColor.A = .80f;

		FSlateRoundedBoxBrush* ViewportGroupBrush = new FSlateRoundedBoxBrush(ToolbarBackgroundColor, 12.f, FLinearColor(0,0,0,.8), 1.0);
		Set("EditorViewportToolBar.Group", ViewportGroupBrush);

		FSlateRoundedBoxBrush* ViewportGroupPressedBrush = new FSlateRoundedBoxBrush(ToolbarPressedColor, 12.f, FLinearColor(0, 0, 0, .8), 1.0);
		Set("EditorViewportToolBar.Group.Pressed", ViewportGroupPressedBrush);

		FButtonStyle ViewportMenuButton = FButtonStyle()
			.SetNormal(*ViewportGroupBrush)
			.SetHovered(*ViewportGroupBrush)
			.SetPressed(*ViewportGroupPressedBrush)
			.SetNormalForeground(FStyleColors::Foreground)
			.SetHoveredForeground(FStyleColors::ForegroundHover)
			.SetPressedForeground(FStyleColors::ForegroundHover)
			.SetDisabledForeground(FStyleColors::Foreground)
			.SetNormalPadding(FMargin(4.0f, 4.0f, 3.0f, 4.0f))
			.SetPressedPadding(FMargin(4.0f, 4.0f, 3.0f, 4.0f));
		Set("EditorViewportToolBar.Button", ViewportMenuButton);

		const FCheckBoxStyle ViewportMenuToggleLeftButtonStyle = FCheckBoxStyle(ViewportToggleButton)
			.SetCheckBoxType(ESlateCheckBoxType::ToggleButton)
			.SetUncheckedImage(		  BOX_BRUSH("Starship/EditorViewport/ToolBarLeftGroup", 12.f/25.f, FStyleColors::Dropdown))
			.SetUncheckedPressedImage(BOX_BRUSH("Starship/EditorViewport/ToolBarLeftGroup", 12.f/25.f,FStyleColors::Recessed))
			.SetUncheckedHoveredImage(BOX_BRUSH("Starship/EditorViewport/ToolBarLeftGroup", 12.f/25.f, FStyleColors::Hover))
			.SetCheckedHoveredImage(  BOX_BRUSH("Starship/EditorViewport/ToolBarLeftGroup", 12.f/25.f, FStyleColors::PrimaryHover))
			.SetCheckedPressedImage(  BOX_BRUSH("Starship/EditorViewport/ToolBarLeftGroup", 12.f/25.f, FStyleColors::PrimaryPress))
			.SetCheckedImage(         BOX_BRUSH("Starship/EditorViewport/ToolBarLeftGroup", 12.f/25.f, FStyleColors::Primary))
			.SetPadding(ViewportMarginLeft);
		Set("EditorViewportToolBar.ToggleButton.Start", ViewportMenuToggleLeftButtonStyle);

		const FCheckBoxStyle ViewportMenuToggleMiddleButtonStyle = FCheckBoxStyle(ViewportToggleButton)
			.SetCheckBoxType(ESlateCheckBoxType::ToggleButton)
			.SetUncheckedImage(		  BOX_BRUSH("Starship/EditorViewport/ToolBarMiddleGroup", 12.f/25.f, FStyleColors::Dropdown))
			.SetUncheckedPressedImage(BOX_BRUSH("Starship/EditorViewport/ToolBarMiddleGroup", 12.f/25.f, FStyleColors::Recessed))
			.SetUncheckedHoveredImage(BOX_BRUSH("Starship/EditorViewport/ToolBarMiddleGroup", 12.f/25.f, FStyleColors::Hover))
			.SetCheckedHoveredImage(  BOX_BRUSH("Starship/EditorViewport/ToolBarMiddleGroup", 12.f/25.f, FStyleColors::PrimaryHover))
			.SetCheckedPressedImage(  BOX_BRUSH("Starship/EditorViewport/ToolBarMiddleGroup", 12.f/25.f, FStyleColors::PrimaryPress))
			.SetCheckedImage(         BOX_BRUSH("Starship/EditorViewport/ToolBarMiddleGroup", 12.f/25.f, FStyleColors::Primary))
			.SetPadding(ViewportMarginCenter);
		Set("EditorViewportToolBar.ToggleButton.Middle", ViewportMenuToggleMiddleButtonStyle);

		const FCheckBoxStyle ViewportMenuToggleRightButtonStyle = FCheckBoxStyle(ViewportToggleButton)
			.SetCheckBoxType(ESlateCheckBoxType::ToggleButton)
			.SetUncheckedImage(		  BOX_BRUSH("Starship/EditorViewport/ToolBarRightGroup", 12.f/25.f, FStyleColors::Dropdown))
			.SetUncheckedPressedImage(BOX_BRUSH("Starship/EditorViewport/ToolBarRightGroup", 12.f/25.f, FStyleColors::Recessed))
			.SetUncheckedHoveredImage(BOX_BRUSH("Starship/EditorViewport/ToolBarRightGroup", 12.f/25.f, FStyleColors::Hover))
			.SetCheckedHoveredImage(  BOX_BRUSH("Starship/EditorViewport/ToolBarRightGroup", 12.f/25.f, FStyleColors::PrimaryHover))
			.SetCheckedPressedImage(  BOX_BRUSH("Starship/EditorViewport/ToolBarRightGroup", 12.f/25.f, FStyleColors::PrimaryPress))
			.SetCheckedImage(         BOX_BRUSH("Starship/EditorViewport/ToolBarRightGroup", 12.f/25.f, FStyleColors::Primary))
			.SetPadding(ViewportMarginRight);
		Set("EditorViewportToolBar.ToggleButton.End", ViewportMenuToggleRightButtonStyle);

		// We want a background-less version as the ComboMenu has its own unified background
		const FToolBarStyle& SlimCoreToolBarStyle = FStarshipCoreStyle::GetCoreStyle().GetWidgetStyle<FToolBarStyle>("SlimToolBar");

		FButtonStyle ComboMenuButtonStyle = FButtonStyle(SlimCoreToolBarStyle.ButtonStyle)
			.SetNormal(BOX_BRUSH("Starship/EditorViewport/ToolBarRightGroup", 12.f/25.f, FStyleColors::Dropdown))
			.SetPressed(BOX_BRUSH("Starship/EditorViewport/ToolBarRightGroup", 12.f/25.f, FStyleColors::Recessed))
			.SetHovered(BOX_BRUSH("Starship/EditorViewport/ToolBarRightGroup", 12.f/25.f, FStyleColors::Hover))
			.SetNormalPadding(0.0)
			.SetPressedPadding(0.0);

		Set("EditorViewportToolBar.ComboMenu.ButtonStyle", ComboMenuButtonStyle);
		Set("EditorViewportToolBar.ComboMenu.ToggleButton", ViewportToggleButton);
		Set("EditorViewportToolBar.ComboMenu.LabelStyle", SlimCoreToolBarStyle.LabelStyle);

		FCheckBoxStyle MaximizeRestoreButton = FCheckBoxStyle(ViewportToolbarStyle.ToggleButton)
			.SetUncheckedImage(*ViewportGroupBrush)
			.SetUncheckedPressedImage(*ViewportGroupPressedBrush)
			.SetUncheckedHoveredImage(*ViewportGroupBrush)
			.SetCheckedImage(*ViewportGroupBrush)
			.SetCheckedHoveredImage(*ViewportGroupBrush)
			.SetCheckedPressedImage(*ViewportGroupPressedBrush)
			.SetForegroundColor(FStyleColors::Foreground)
			.SetPressedForegroundColor(FStyleColors::ForegroundHover)
			.SetHoveredForegroundColor(FStyleColors::ForegroundHover)
			.SetCheckedForegroundColor(FStyleColors::Foreground)
			.SetCheckedPressedForegroundColor(FStyleColors::ForegroundHover)
			.SetCheckedHoveredForegroundColor(FStyleColors::ForegroundHover)
			.SetPadding(FMargin(4.0f, 4.0f, 3.0f, 4.0f));
		Set("EditorViewportToolBar.MaximizeRestoreButton", MaximizeRestoreButton);

		Set("EditorViewportToolBar.Heading.Padding", FMargin(4.f));


		// SComboBox 
		FComboButtonStyle ViewportComboButton = FComboButtonStyle()
			.SetButtonStyle(ViewportMenuButton)
			.SetContentPadding(ViewportMarginCenter);

		// Non-grouped Toggle Button
		FCheckBoxStyle SoloToggleButton = FCheckBoxStyle(ViewportToolbarStyle.ToggleButton)
			.SetUncheckedImage(*ViewportGroupBrush)
			.SetUncheckedPressedImage(*ViewportGroupPressedBrush)
			.SetUncheckedHoveredImage(*ViewportGroupBrush)
			.SetCheckedImage(FSlateRoundedBoxBrush(FStyleColors::Primary, 12.f, FLinearColor(0, 0, 0, .8), 1.0))
			.SetCheckedHoveredImage(FSlateRoundedBoxBrush(FStyleColors::PrimaryHover, 12.f, FLinearColor(0, 0, 0, .8), 1.0))
			.SetCheckedPressedImage(FSlateRoundedBoxBrush(FStyleColors::PrimaryPress, 12.f, FLinearColor(0, 0, 0, .8), 1.0))
			.SetForegroundColor(FStyleColors::Foreground)
			.SetPressedForegroundColor(FStyleColors::ForegroundHover)
			.SetHoveredForegroundColor(FStyleColors::ForegroundHover)
			.SetCheckedForegroundColor(FStyleColors::Foreground)
			.SetCheckedPressedForegroundColor(FStyleColors::ForegroundHover)
			.SetCheckedHoveredForegroundColor(FStyleColors::ForegroundHover)
			.SetPadding(FMargin(6.0f, 4.0f, 6.0f, 4.0f));


		ViewportToolbarStyle
			.SetBackground(FSlateNoResource())
			.SetIconSize(Icon16x16)
			.SetBackgroundPadding(FMargin(0))
			.SetLabelPadding(FMargin(0))
			.SetComboButtonPadding(FMargin(4.f, 0.0f))
			.SetBlockPadding(FMargin(0.0f,0.0f))
			.SetIndentedBlockPadding(FMargin(0))
			.SetButtonPadding(FMargin(0))
			.SetCheckBoxPadding(FMargin(4.0f, 0.0f))
			//.SetComboButtonStyle(ViewportComboButton)
			.SetToggleButtonStyle(SoloToggleButton)
			.SetButtonStyle(ViewportMenuButton)
			.SetSeparatorBrush(FSlateNoResource())
			.SetSeparatorPadding(FMargin(2.0f, 0.0f))
			.SetExpandBrush(IMAGE_BRUSH("Icons/toolbar_expand_16x", Icon8x8));
		Set("EditorViewportToolBar", ViewportToolbarStyle);

		FButtonStyle ViewportMenuWarningButton = FButtonStyle(ViewportMenuButton)
			.SetNormalForeground(FStyleColors::AccentYellow)
			.SetHoveredForeground(FStyleColors::ForegroundHover)
			.SetPressedForeground(FStyleColors::ForegroundHover)
			.SetDisabledForeground(FStyleColors::AccentYellow);
		Set("EditorViewportToolBar.WarningButton", ViewportMenuWarningButton);

		Set("EditorViewportToolBar.Background", new FSlateNoResource());
		Set("EditorViewportToolBar.OptionsDropdown", new IMAGE_BRUSH_SVG("Starship/EditorViewport/menu", Icon16x16));

		Set("EditorViewportToolBar.Font", FStyleFonts::Get().Normal);

		Set("EditorViewportToolBar.MenuButton", FButtonStyle(Button)
			.SetNormal(BOX_BRUSH("Common/SmallRoundedButton", FMargin(7.f / 16.f), FLinearColor(1, 1, 1, 0.75f)))
			.SetHovered(BOX_BRUSH("Common/SmallRoundedButton", FMargin(7.f / 16.f), FLinearColor(1, 1, 1, 1.0f)))
			.SetPressed(BOX_BRUSH("Common/SmallRoundedButton", FMargin(7.f / 16.f)))
		);

		
		Set("EditorViewportToolBar.MenuDropdown", new IMAGE_BRUSH("Common/ComboArrow", Icon8x8));
		Set("EditorViewportToolBar.Maximize.Normal", new IMAGE_BRUSH_SVG("Starship/EditorViewport/square", Icon16x16));
		Set("EditorViewportToolBar.Maximize.Checked", new IMAGE_BRUSH_SVG("Starship/EditorViewport/quad", Icon16x16));
		Set("EditorViewportToolBar.RestoreFromImmersive.Normal", new IMAGE_BRUSH("Icons/icon_RestoreFromImmersive_16px", Icon16x16));

		FLinearColor ViewportOverlayColor = FStyleColors::Input.GetSpecifiedColor();
		ViewportOverlayColor.A = 0.75f;

		Set("SodaViewport.OverlayBrush", new FSlateRoundedBoxBrush(ViewportOverlayColor, 8.0, FStyleColors::Dropdown, 1.0));

		Set("LevelEditor.OpenAddContent.Background", new IMAGE_BRUSH_SVG("Starship/MainToolbar/PlaceActorsBase", Icon20x20));
		Set("LevelEditor.OpenAddContent.Overlay", new IMAGE_BRUSH_SVG("Starship/MainToolbar/ToolBadgePlus", Icon20x20, FStyleColors::AccentGreen));

	}
}

void FStarshipSodaStyle::FStyle::SetupMenuBarStyles()
{
	// MenuBar
	{
		Set("Menu.Label.Padding", FMargin(0.0f, 0.0f, 0.0f, 0.0f));
		Set("Menu.Label.ContentPadding", FMargin(10.0f, 2.0f));
	}
}

void FStarshipSodaStyle::FStyle::SetupGeneralIcons()
{
	Set("Plus", new IMAGE_BRUSH("Icons/PlusSymbol_12x", Icon12x12));
	Set("Cross", new IMAGE_BRUSH("Icons/Cross_12x", Icon12x12));
	Set("ArrowUp", new IMAGE_BRUSH("Icons/ArrowUp_12x", Icon12x12));
	Set("ArrowDown", new IMAGE_BRUSH("Icons/ArrowDown_12x", Icon12x12));
	Set("AssetEditor.SaveThumbnail", new IMAGE_BRUSH_SVG("Starship/AssetEditors/SaveThumbnail", Icon20x20));
	Set("AssetEditor.ToggleShowBounds", new IMAGE_BRUSH_SVG("Starship/Common/SetShowBounds", Icon20x20));
	Set("AssetEditor.Apply", new IMAGE_BRUSH_SVG("Starship/Common/Apply", Icon20x20));
	Set("AssetEditor.Simulate", new IMAGE_BRUSH_SVG("Starship/MainToolbar/simulate", Icon20x20));
	Set("AssetEditor.ToggleStats", new IMAGE_BRUSH_SVG("Starship/Common/Statistics", Icon20x20));
	Set("AssetEditor.CompileStatus.Background", new IMAGE_BRUSH_SVG("Starship/Blueprints/CompileStatus_Background", Icon20x20));
	Set("AssetEditor.CompileStatus.Overlay.Unknown", new IMAGE_BRUSH_SVG("Starship/Blueprints/CompileStatus_Unknown_Badge", Icon20x20, FStyleColors::AccentYellow));
	Set("AssetEditor.CompileStatus.Overlay.Warning", new IMAGE_BRUSH_SVG("Starship/Blueprints/CompileStatus_Warning_Badge", Icon20x20, FStyleColors::Warning));
	Set("AssetEditor.CompileStatus.Overlay.Good", new IMAGE_BRUSH_SVG("Starship/Blueprints/CompileStatus_Good_Badge", Icon20x20, FStyleColors::AccentGreen));
	Set("AssetEditor.CompileStatus.Overlay.Error", new IMAGE_BRUSH_SVG("Starship/Blueprints/CompileStatus_Fail_Badge", Icon20x20, FStyleColors::Error));
	
	Set("Debug", new IMAGE_BRUSH_SVG( "Starship/Common/Debug", Icon16x16 ) );
	Set("Modules", new IMAGE_BRUSH_SVG( "Starship/Common/Modules", Icon16x16 ) );
	Set("Versions", new IMAGE_BRUSH_SVG("Starship/Common/Versions", Icon20x20));
}

void FStarshipSodaStyle::FStyle::SetupWindowStyles()
{
	// Window styling
	{
		EditorWindowHighlightBrush = CORE_IMAGE_BRUSH("Common/Window/WindowTitle", FVector2D(74, 74), FLinearColor::White, ESlateBrushTileType::Horizontal);
	}
}

void FStarshipSodaStyle::FStyle::SetupPropertySodaStyles()
	{
	// Property / details Window / PropertyTable 
	{
		Set( "PropertyEditor.Grid.TabIcon", new IMAGE_BRUSH( "Icons/icon_PropertyMatrix_16px", Icon16x16 ) );
		Set( "PropertyEditor.Properties.TabIcon", new IMAGE_BRUSH( "Icons/icon_tab_SelectionDetails_16x", Icon16x16 ) );

		Set( "PropertyEditor.RemoveColumn", new IMAGE_BRUSH( "Common/PushPin_Down", Icon16x16, FColor( 96, 194, 253, 255 ).ReinterpretAsLinear() ) );
		Set( "PropertyEditor.AddColumn", new IMAGE_BRUSH( "Common/PushPin_Up", Icon16x16, FColor( 96, 194, 253, 255 ).ReinterpretAsLinear() ) );

		Set( "PropertyEditor.AddColumnOverlay",	new IMAGE_BRUSH( "Common/TinyChalkArrow", FVector2D( 71, 20 ), FColor( 96, 194, 253, 255 ).ReinterpretAsLinear() ) );
		Set( "PropertyEditor.AddColumnMessage", FTextBlockStyle(NormalText)
			.SetFont( DEFAULT_FONT( "BoldCondensedItalic", 10 ) )
			.SetColorAndOpacity(FColor( 96, 194, 253, 255 ).ReinterpretAsLinear())
		);
	

		Set( "PropertyEditor.AssetName.ColorAndOpacity", FLinearColor::White );

		Set("PropertyEditor.AssetThumbnailBorder", new FSlateRoundedBoxBrush(FStyleColors::Transparent, 4.0f, FStyleColors::InputOutline, 1.0f));
		Set("PropertyEditor.AssetThumbnailBorderHovered", new FSlateRoundedBoxBrush(FStyleColors::Transparent, 4.0f, FStyleColors::Hover2, 1.0f));
		Set("PropertyEditor.AssetTileItem.DropShadow", new BOX_BRUSH("Starship/ContentBrowser/drop-shadow", FMargin(4.0f / 64.0f)));

		Set( "PropertyEditor.AssetClass", FTextBlockStyle(NormalText)
			.SetFont( DEFAULT_FONT( "Regular", 10 ) )
			.SetColorAndOpacity( FLinearColor::White )
			.SetShadowOffset( FVector2D(1,1) )
			.SetShadowColorAndOpacity( FLinearColor::Black )
		);

		const FButtonStyle AssetComboStyle = FButtonStyle()
			.SetNormal( BOX_BRUSH( "Common/ButtonHoverHint", FMargin(4/16.0f), FLinearColor(1,1,1,0.15f) ) )
			.SetHovered( BOX_BRUSH( "Common/ButtonHoverHint", FMargin(4/16.0f), FLinearColor(1,1,1,0.25f) ) )
			.SetPressed( BOX_BRUSH( "Common/ButtonHoverHint", FMargin(4/16.0f), FLinearColor(1,1,1,0.30f) ) )
			.SetNormalPadding( FMargin(0,0,0,1) )
			.SetPressedPadding( FMargin(0,1,0,0) );
		Set( "PropertyEditor.AssetComboStyle", AssetComboStyle );

		Set( "PropertyEditor.HorizontalDottedLine",		new IMAGE_BRUSH( "Common/HorizontalDottedLine_16x1px", FVector2D(16.0f, 1.0f), FLinearColor::White, ESlateBrushTileType::Horizontal ) );
		Set( "PropertyEditor.VerticalDottedLine",		new IMAGE_BRUSH( "Common/VerticalDottedLine_1x16px", FVector2D(1.0f, 16.0f), FLinearColor::White, ESlateBrushTileType::Vertical ) );
		Set( "PropertyEditor.SlateBrushPreview",		new BOX_BRUSH( "PropertyView/SlateBrushPreview_32px", Icon32x32, FMargin(3.f/32.f, 3.f/32.f, 15.f/32.f, 13.f/32.f) ) );

		Set( "PropertyTable.TableRow", FTableRowStyle()
			.SetEvenRowBackgroundBrush( FSlateColorBrush( FLinearColor( 0.70f, 0.70f, 0.70f, 255 ) ) )
			.SetEvenRowBackgroundHoveredBrush( IMAGE_BRUSH( "Common/Selection", Icon8x8, SelectionColor_Inactive ) )
			.SetOddRowBackgroundBrush( FSlateColorBrush( FLinearColor( 0.80f, 0.80f, 0.80f, 255 ) ) )
			.SetOddRowBackgroundHoveredBrush( IMAGE_BRUSH( "Common/Selection", Icon8x8, SelectionColor_Inactive ) )
			.SetSelectorFocusedBrush( BORDER_BRUSH( "Common/Selector", FMargin(4.f/16.f), SelectorColor ) )
			.SetActiveBrush( IMAGE_BRUSH( "Common/Selection", Icon8x8, SelectionColor ) )
			.SetActiveHoveredBrush( IMAGE_BRUSH( "Common/Selection", Icon8x8, SelectionColor ) )
			.SetInactiveBrush( IMAGE_BRUSH( "Common/Selection", Icon8x8, SelectionColor_Inactive ) )
			.SetInactiveHoveredBrush( IMAGE_BRUSH( "Common/Selection", Icon8x8, SelectionColor_Inactive ) )
			.SetTextColor( DefaultForeground )
			.SetSelectedTextColor( InvertedForeground )
			);

		const FTableColumnHeaderStyle PropertyTableColumnHeaderStyle = FTableColumnHeaderStyle()
			.SetSortPrimaryAscendingImage(IMAGE_BRUSH("Common/SortUpArrow", Icon8x4))
			.SetSortPrimaryDescendingImage(IMAGE_BRUSH("Common/SortDownArrow", Icon8x4))
			.SetSortSecondaryAscendingImage(IMAGE_BRUSH("Common/SortUpArrows", Icon16x4))
			.SetSortSecondaryDescendingImage(IMAGE_BRUSH("Common/SortDownArrows", Icon16x4))
			.SetNormalBrush( BOX_BRUSH( "Common/ColumnHeader", 4.f/32.f ) )
			.SetHoveredBrush( BOX_BRUSH( "Common/ColumnHeader_Hovered", 4.f/32.f ) )
			.SetMenuDropdownImage( IMAGE_BRUSH( "Common/ColumnHeader_Arrow", Icon8x8 ) )
			.SetMenuDropdownNormalBorderBrush( BOX_BRUSH( "Common/ColumnHeaderMenuButton_Normal", 4.f/32.f ) )
			.SetMenuDropdownHoveredBorderBrush( BOX_BRUSH( "Common/ColumnHeaderMenuButton_Hovered", 4.f/32.f ) );

		const FTableColumnHeaderStyle PropertyTableLastColumnHeaderStyle = FTableColumnHeaderStyle()
			.SetSortPrimaryAscendingImage(IMAGE_BRUSH("Common/SortUpArrow", Icon8x4))
			.SetSortPrimaryDescendingImage(IMAGE_BRUSH("Common/SortDownArrow", Icon8x4))
			.SetSortSecondaryAscendingImage(IMAGE_BRUSH("Common/SortUpArrows", Icon16x4))
			.SetSortSecondaryDescendingImage(IMAGE_BRUSH("Common/SortDownArrows", Icon16x4))
			.SetNormalBrush( FSlateNoResource() )
			.SetHoveredBrush( BOX_BRUSH( "Common/LastColumnHeader_Hovered", 4.f/32.f ) )
			.SetMenuDropdownImage( IMAGE_BRUSH( "Common/ColumnHeader_Arrow", Icon8x8 ) )
			.SetMenuDropdownNormalBorderBrush( BOX_BRUSH( "Common/ColumnHeaderMenuButton_Normal", 4.f/32.f ) )
			.SetMenuDropdownHoveredBorderBrush( BOX_BRUSH( "Common/ColumnHeaderMenuButton_Hovered", 4.f/32.f ) );

		const FSplitterStyle PropertyTableHeaderSplitterStyle = FSplitterStyle()
			.SetHandleNormalBrush( FSlateNoResource() )
			.SetHandleHighlightBrush( IMAGE_BRUSH( "Common/HeaderSplitterGrip", Icon8x8 ) );

		Set( "PropertyTable.HeaderRow", FHeaderRowStyle()
			.SetColumnStyle( PropertyTableColumnHeaderStyle )
			.SetLastColumnStyle( PropertyTableLastColumnHeaderStyle )
			.SetColumnSplitterStyle( PropertyTableHeaderSplitterStyle )
			.SetBackgroundBrush( BOX_BRUSH( "Common/TableViewHeader", 4.f/32.f ) )
			.SetForegroundColor( DefaultForeground )
			);

		FWindowStyle InViewportDecoratorWindow = FStarshipCoreStyle::GetCoreStyle().GetWidgetStyle<FWindowStyle>("Window");
		InViewportDecoratorWindow.SetCornerRadius(4);

		Set("InViewportDecoratorWindow", InViewportDecoratorWindow);
		FLinearColor TransparentBackground = FStyleColors::Background.GetSpecifiedColor();
		TransparentBackground.A = 0.8f;
		Set("PropertyTable.InViewport.Header", new FSlateRoundedBoxBrush(FStyleColors::Title, FVector4(4.0f, 4.0f, 0.0f, 0.0f)));
		Set("PropertyTable.InViewport.Background", new FSlateRoundedBoxBrush(FSlateColor(TransparentBackground), 4.0f));
		// InViewportToolbar
		{
			FToolBarStyle InViewportToolbar = FStarshipCoreStyle::GetCoreStyle().GetWidgetStyle<FToolBarStyle>("SlimToolBar");
			InViewportToolbar.SetBackground(FSlateColorBrush(FStyleColors::Panel));
			InViewportToolbar.SetBackgroundPadding(FMargin(4.0f, 0.0f));
			InViewportToolbar.SetButtonPadding(0.0f);
			InViewportToolbar.SetIconSize(Icon16x16);
			InViewportToolbar.ButtonStyle.SetNormalPadding(FMargin(4, 4, 4, 4));
			InViewportToolbar.ButtonStyle.SetPressedPadding(FMargin(4, 5, 4, 3));
			Set("InViewportToolbar", InViewportToolbar);
		}
		const FTableViewStyle InViewportViewStyle = FTableViewStyle()
			.SetBackgroundBrush(FSlateNoResource());
		Set("PropertyTable.InViewport.ListView", InViewportViewStyle);

		Set("PropertyTable.InViewport.Row", FTableRowStyle(NormalTableRowStyle)
			.SetEvenRowBackgroundBrush(FSlateNoResource())
			.SetEvenRowBackgroundHoveredBrush(FSlateNoResource())
			.SetOddRowBackgroundBrush(FSlateNoResource())
			.SetOddRowBackgroundHoveredBrush(FSlateNoResource())
			.SetSelectorFocusedBrush(FSlateNoResource())
			.SetActiveBrush(FSlateNoResource())
			.SetActiveHoveredBrush(FSlateNoResource())
			.SetInactiveBrush(FSlateNoResource())
			.SetInactiveHoveredBrush(FSlateNoResource())
		);

		const FSplitterStyle TransparentSplitterStyle = FSplitterStyle()
			.SetHandleNormalBrush(FSlateNoResource())
			.SetHandleHighlightBrush(FSlateNoResource());
		Set("PropertyTable.InViewport.Splitter", TransparentSplitterStyle);

		Set( "PropertyTable.Selection.Active",						new IMAGE_BRUSH( "Common/Selection", Icon8x8, SelectionColor ) );

		Set( "PropertyTable.HeaderRow.Column.PathDelimiter",		new IMAGE_BRUSH( "Common/SmallArrowRight", Icon10x10 ) );

		Set( "PropertyTable.RowHeader.Background",					new BOX_BRUSH( "Old/Menu_Background", FMargin(4.f/64.f) ) );
		Set( "PropertyTable.RowHeader.BackgroundActive",			new BOX_BRUSH( "Old/Menu_Background", FMargin(4.f/64.f), SelectionColor_Inactive ) );
		Set( "PropertyTable.ColumnBorder",							new BOX_BRUSH( "Common/ColumnBorder", FMargin(4.f/16.f), FLinearColor(0.1f, 0.1f, 0.1f, 0.5f) ) );
		Set( "PropertyTable.CellBorder",							new BOX_BRUSH( "Common/CellBorder", FMargin(4.f/16.f), FLinearColor(0.1f, 0.1f, 0.1f, 0.5f) ) );
		Set( "PropertyTable.ReadOnlyEditModeCellBorder",			new BORDER_BRUSH( "Common/ReadOnlyEditModeCellBorder", FMargin(6.f/32.f), SelectionColor ) );
		Set( "PropertyTable.ReadOnlyCellBorder",					new BOX_BRUSH( "Common/ReadOnlyCellBorder", FMargin(4.f/16.f), FLinearColor(0.1f, 0.1f, 0.1f, 0.5f) ) );
		Set( "PropertyTable.CurrentCellBorder",						new BOX_BRUSH( "Common/CurrentCellBorder", FMargin(4.f/16.f), FLinearColor(0.0f, 0.0f, 0.0f, 1.0f) ) );
		Set( "PropertyTable.ReadOnlySelectedCellBorder",			new BOX_BRUSH( "Common/ReadOnlySelectedCellBorder", FMargin(4.f/16.f), FLinearColor(0.0f, 0.0f, 0.0f, 1.0f) ) );
		Set( "PropertyTable.ReadOnlyCurrentCellBorder",				new BOX_BRUSH( "Common/ReadOnlyCurrentCellBorder", FMargin(4.f/16.f), FLinearColor(0.0f, 0.0f, 0.0f, 1.0f) ) );
		Set( "PropertyTable.Cell.DropDown.Background",				new BOX_BRUSH( "Common/GroupBorder", FMargin(4.f/16.f) ) );
		Set( "PropertyTable.ContentBorder",							new BOX_BRUSH( "Common/GroupBorder", FMargin(4.0f/16.0f) ) );	
		Set( "PropertyTable.NormalFont",							DEFAULT_FONT( "Regular", 9 ) );
		Set( "PropertyTable.BoldFont",								DEFAULT_FONT( "Bold", 9 ) );
		Set( "PropertyTable.FilterFont",							DEFAULT_FONT( "Regular", 10 ) );

		Set( "PropertyWindow.FilterSearch", new IMAGE_BRUSH( "Old/FilterSearch", Icon16x16 ) );
		Set( "PropertyWindow.FilterCancel", new IMAGE_BRUSH( "Old/FilterCancel", Icon16x16 ) );
		Set( "PropertyWindow.Favorites_Disabled", new IMAGE_BRUSH( "Icons/EmptyStar_16x", Icon16x16 ) );
		Set( "PropertyWindow.Locked", new CORE_IMAGE_BRUSH_SVG( "Starship/Common/lock", Icon16x16 ) );
		Set( "PropertyWindow.Unlocked", new CORE_IMAGE_BRUSH_SVG( "Starship/Common/lock-unlocked", Icon16x16 ) );
		Set( "PropertyWindow.DiffersFromDefault", new IMAGE_BRUSH_SVG( "Starship/Common/ResetToDefault", Icon16x16) ) ;
		
		Set( "PropertyWindow.NormalFont", FStyleFonts::Get().Small);
		Set( "PropertyWindow.BoldFont",FStyleFonts::Get().SmallBold);
		Set( "PropertyWindow.ItalicFont", DEFAULT_FONT( "Italic", 8 ) );
		Set( "PropertyWindow.FilterFont", DEFAULT_FONT( "Regular", 10 ) );

		FSlateFontInfo MobilityFont = FStyleFonts::Get().Small;
		MobilityFont.LetterSpacing = 100;

		Set("PropertyWindow.MobilityFont", MobilityFont );
		Set("PropertyWindow.MobilityStatic", new IMAGE_BRUSH_SVG("Starship/Common/MobilityStatic", Icon16x16));
		Set("PropertyWindow.MobilityStationary", new IMAGE_BRUSH_SVG("Starship/Common/MobilityStationary", Icon16x16));
		Set("PropertyWindow.MobilityMoveable", new IMAGE_BRUSH_SVG("Starship/Common/MobilityMoveable", Icon16x16));

		Set( "PropertyWindow.NoOverlayColor", new FSlateNoResource() );
		Set( "PropertyWindow.EditConstColor", new FSlateColorBrush( FColor( 152, 152, 152, 80 ) ) );
		Set( "PropertyWindow.FilteredColor", new FSlateColorBrush( FColor( 0, 255, 0, 80 ) ) );
		Set( "PropertyWindow.FilteredEditConstColor", new FSlateColorBrush( FColor( 152, 152, 152, 80 ).ReinterpretAsLinear() * FColor(0,255,0,255).ReinterpretAsLinear() ) );
		Set( "PropertyWindow.CategoryBackground", new BOX_BRUSH( "/PropertyView/CategoryBackground", FMargin(4.f/16.f) ) );
		Set( "PropertyWindow.CategoryForeground", FLinearColor::Black );
		Set( "PropertyWindow.Button_Clear", new IMAGE_BRUSH( "Icons/Cross_12x", Icon12x12 ) );
		Set( "PropertyWindow.Button_Ellipsis", new IMAGE_BRUSH( "Icons/ellipsis_12x", Icon12x12 ) );
		Set( "PropertyWindow.Button_PickAsset", new IMAGE_BRUSH( "Icons/pillarray_12x", Icon12x12 ) );
		Set( "PropertyWindow.Button_PickActor", new IMAGE_BRUSH( "Icons/levels_16x", Icon12x12 ) );

		Set( "PropertyWindow.WindowBorder", new BOX_BRUSH( "Common/GroupBorder", FMargin(4.0f/16.0f) ) );

		FInlineEditableTextBlockStyle NameStyle(FCoreStyle::Get().GetWidgetStyle<FInlineEditableTextBlockStyle>("InlineEditableTextBlockStyle"));
		NameStyle.EditableTextBoxStyle.SetFont(DEFAULT_FONT("Regular", 11))
			.SetForegroundColor(FSlateColor(EStyleColor::White));
		NameStyle.TextStyle.SetFont(DEFAULT_FONT("Regular", 11))
			.SetColorAndOpacity(FSlateColor(EStyleColor::White));
		Set( "DetailsView.ConstantTextBlockStyle", NameStyle.TextStyle);
		Set( "DetailsView.NameTextBlockStyle",  NameStyle );

		Set( "DetailsView.NameChangeCommitted", new BOX_BRUSH( "Common/EditableTextSelectionBackground", FMargin(4.f/16.f) ) );
		Set( "DetailsView.HyperlinkStyle", FTextBlockStyle(NormalText) .SetFont( DEFAULT_FONT( "Regular", 8 ) ) );

		FTextBlockStyle BPWarningMessageTextStyle = FTextBlockStyle(NormalText) .SetFont(DEFAULT_FONT("Regular", 8));
		FTextBlockStyle BPWarningMessageHyperlinkTextStyle = FTextBlockStyle(BPWarningMessageTextStyle).SetColorAndOpacity(FLinearColor(0.25f, 0.5f, 1.0f));

		FButtonStyle EditBPHyperlinkButton = FButtonStyle()
			.SetNormal(BORDER_BRUSH("Old/HyperlinkDotted", FMargin(0, 0, 0, 3 / 16.0f), FLinearColor(0.25f, 0.5f, 1.0f)))
			.SetPressed(FSlateNoResource())
			.SetHovered(BORDER_BRUSH("Old/HyperlinkUnderline", FMargin(0, 0, 0, 3 / 16.0f), FLinearColor(0.25f, 0.5f, 1.0f)));

		FHyperlinkStyle BPWarningMessageHyperlinkStyle = FHyperlinkStyle()
			.SetUnderlineStyle(EditBPHyperlinkButton)
			.SetTextStyle(BPWarningMessageHyperlinkTextStyle)
			.SetPadding(FMargin(0.0f));

		Set( "DetailsView.BPMessageHyperlinkStyle", BPWarningMessageHyperlinkStyle );
		Set( "DetailsView.BPMessageTextStyle", BPWarningMessageTextStyle );

		Set( "DetailsView.GroupSection",              new FSlateNoResource());

		Set( "DetailsView.PulldownArrow.Down",        new CORE_IMAGE_BRUSH_SVG("Starship/Common/chevron-down", Icon16x16, FStyleColors::Foreground)); 
		Set( "DetailsView.PulldownArrow.Down.Hovered",new CORE_IMAGE_BRUSH_SVG("Starship/Common/chevron-down", Icon16x16, FStyleColors::ForegroundHover)); 
		Set( "DetailsView.PulldownArrow.Up",          new CORE_IMAGE_BRUSH_SVG("Starship/Common/chevron-up",   Icon16x16, FStyleColors::Foreground)); 
		Set( "DetailsView.PulldownArrow.Up.Hovered",  new CORE_IMAGE_BRUSH_SVG("Starship/Common/chevron-up",   Icon16x16, FStyleColors::ForegroundHover)); 

		Set( "DetailsView.EditRawProperties",         new CORE_IMAGE_BRUSH_SVG("Starship/Common/layout-spreadsheet",  Icon16x16, FLinearColor::White) );
		Set( "DetailsView.ViewOptions",         	  new CORE_IMAGE_BRUSH_SVG("Starship/Common/settings",  Icon16x16, FLinearColor::White) );
		Set( "DetailsView.EditConfigProperties",      new IMAGE_BRUSH("Icons/icon_PropertyMatrix_16px",  Icon16x16, FLinearColor::White ) );

		Set( "DetailsView.CollapsedCategory",         new FSlateColorBrush(FStyleColors::Header));
		Set( "DetailsView.CollapsedCategory_Hovered", new FSlateColorBrush(FStyleColors::Hover));
		Set( "DetailsView.CategoryTop",               new FSlateColorBrush(FStyleColors::Header));
		Set( "DetailsView.CategoryTop_Hovered",       new FSlateColorBrush(FStyleColors::Hover));
		Set( "DetailsView.CategoryBottom",            new FSlateColorBrush(FStyleColors::Recessed));
		
		// these are not actually displayed as white, see PropertyEditorConstants::GetRowBackgroundColor
		Set( "DetailsView.CategoryMiddle",            new FSlateColorBrush(FStyleColors::White));
		Set( "DetailsView.Highlight",				  new FSlateRoundedBoxBrush(FStyleColors::Transparent, 0.0f, FStyleColors::AccentBlue, 1.0f));

		Set( "DetailsView.PropertyIsFavorite", new IMAGE_BRUSH("PropertyView/Favorites_Enabled", Icon12x12));
		Set( "DetailsView.PropertyIsNotFavorite", new IMAGE_BRUSH("PropertyView/Favorites_Disabled", Icon12x12));
		Set( "DetailsView.NoFavoritesSystem", new IMAGE_BRUSH("PropertyView/NoFavoritesSystem", Icon12x12));

		Set( "DetailsView.Splitter", FSplitterStyle()
			.SetHandleNormalBrush(FSlateColorBrush(FStyleColors::Recessed))                   
			.SetHandleHighlightBrush(FSlateColorBrush(FStyleColors::Recessed))
		);

		Set( "DetailsView.GridLine", new FSlateColorBrush(FStyleColors::Recessed) );
		Set( "DetailsView.SectionButton", FCheckBoxStyle( FStarshipCoreStyle::GetCoreStyle().GetWidgetStyle<FCheckBoxStyle>("ToggleButtonCheckbox"))
			.SetUncheckedImage(FSlateRoundedBoxBrush(FStyleColors::Header, 4.0f, FStyleColors::Input, 1.0f))
			.SetUncheckedHoveredImage(FSlateRoundedBoxBrush(FStyleColors::Hover, 4.0f, FStyleColors::Input, 1.0f))
			.SetUncheckedPressedImage(FSlateRoundedBoxBrush(FStyleColors::Hover, 4.0f, FStyleColors::Input, 1.0f))
			.SetCheckedImage(FSlateRoundedBoxBrush(FStyleColors::Primary, 4.0f, FStyleColors::Input, 1.0f))
			.SetCheckedHoveredImage(FSlateRoundedBoxBrush(FStyleColors::PrimaryHover, 4.0f, FStyleColors::Input, 1.0f))
			.SetCheckedPressedImage(FSlateRoundedBoxBrush(FStyleColors::PrimaryHover, 4.0f, FStyleColors::Input, 1.0f))
			.SetPadding(FMargin(16, 4))
		);

		Set( "DetailsView.CategoryFontStyle", FStyleFonts::Get().SmallBold);
		Set( "DetailsView.CategoryTextStyle", FTextBlockStyle(NormalText)
			.SetFont(GetFontStyle("DetailsView.CategoryFontStyle"))
			.SetColorAndOpacity(FStyleColors::ForegroundHeader)
		);

		Set("DetailsView.CategoryTextStyleUpdate", FTextBlockStyle(NormalText)
			.SetFont(FStyleFonts::Get().Small)
			.SetColorAndOpacity(FStyleColors::ForegroundHeader)
			.SetTransformPolicy(ETextTransformPolicy::ToUpper)
		);

		FButtonStyle DetailsExtensionMenuButton = FButtonStyle(FStarshipCoreStyle::GetCoreStyle().GetWidgetStyle<FButtonStyle>("NoBorder"))
			.SetNormalForeground(FStyleColors::Foreground)
			.SetHoveredForeground(FStyleColors::ForegroundHover)
			.SetPressedForeground(FStyleColors::ForegroundHover)
			.SetDisabledForeground(FStyleColors::Foreground)
			.SetNormalPadding(FMargin(2, 2, 2, 2))
			.SetPressedPadding(FMargin(2, 3, 2, 1));

		Set("DetailsView.ExtensionToolBar.Button", DetailsExtensionMenuButton);

		FToolBarStyle DetailsExtensionToolBarStyle = FToolBarStyle(FStarshipCoreStyle::GetCoreStyle().GetWidgetStyle<FToolBarStyle>("SlimToolBar"))
			.SetButtonStyle(DetailsExtensionMenuButton)
			.SetExpandBrush(CORE_IMAGE_BRUSH_SVG("Starship/Common/ellipsis-vertical-narrow", FVector2D(4, 16)))
			.SetIconSize(Icon16x16)
			.SetBackground(FSlateNoResource())
			.SetLabelPadding(FMargin(0))
			.SetComboButtonPadding(FMargin(0))
			.SetBlockPadding(FMargin(0))
			.SetIndentedBlockPadding(FMargin(0))
			.SetBackgroundPadding(FMargin(0))
			.SetButtonPadding(FMargin(0))
			.SetCheckBoxPadding(FMargin(0))
			.SetSeparatorBrush(FSlateNoResource())
			.SetSeparatorPadding(FMargin(0));

		Set("DetailsView.ExtensionToolBar", DetailsExtensionToolBarStyle);

		Set("DetailsView.ArrayDropShadow", new IMAGE_BRUSH("Common/ArrayDropShadow", FVector2D(32,2)));

		Set( "DetailsView.TreeView.TableRow", FTableRowStyle()
			.SetEvenRowBackgroundBrush( FSlateNoResource() )
			.SetEvenRowBackgroundHoveredBrush( FSlateNoResource() )
			.SetOddRowBackgroundBrush( FSlateNoResource() )
			.SetOddRowBackgroundHoveredBrush( FSlateNoResource() )
			.SetSelectorFocusedBrush( FSlateNoResource() )
			.SetActiveBrush( FSlateNoResource() )
			.SetActiveHoveredBrush( FSlateNoResource() )
			.SetInactiveBrush( FSlateNoResource() )
			.SetInactiveHoveredBrush( FSlateNoResource() )
			.SetTextColor( DefaultForeground )
			.SetSelectedTextColor( InvertedForeground )
			.SetDropIndicator_Above(BOX_BRUSH("Common/DropZoneIndicator_Above", FMargin(10.0f / 16.0f, 10.0f / 16.0f, 0.f, 0.f), SelectionColor))
			.SetDropIndicator_Onto(BOX_BRUSH("Common/DropZoneIndicator_Onto", FMargin(4.0f / 16.0f), SelectionColor))
			.SetDropIndicator_Below(BOX_BRUSH("Common/DropZoneIndicator_Below", FMargin(10.0f / 16.0f, 0.f, 0.f, 10.0f / 16.0f), SelectionColor))
			);

		Set("DetailsView.DropZone.Below", new BOX_BRUSH("Common/VerticalBoxDropZoneIndicator_Below", FMargin(10.0f / 16.0f, 0, 0, 10.0f / 16.0f), SelectionColor_Subdued));

		FButtonStyle NameAreaButton = FButtonStyle(Button)
			.SetNormalPadding(FMargin(6, 3))
			.SetPressedPadding(FMargin(6, 3));
		Set("DetailsView.NameAreaButton", NameAreaButton);

		Set("DetailsView.NameAreaComboButton", FComboButtonStyle(GetWidgetStyle<FComboButtonStyle>("ComboButton"))
			.SetButtonStyle(NameAreaButton)
			.SetDownArrowPadding(FMargin(4, 0, 0, 0))
			.SetContentPadding(FMargin(4, 0, 0, 0))
		);

	}
	}
	
void FStarshipSodaStyle::FStyle::SetupGraphSodaStyles()
{
	Set("Graph.ConnectorFeedback.Border", new BOX_BRUSH("Old/Menu_Background", FMargin(8.0f / 64.0f)));
	Set("Graph.ConnectorFeedback.OK", new IMAGE_BRUSH("Old/Graph/Feedback_OK", Icon16x16));
	Set("Graph.ConnectorFeedback.OKWarn", new IMAGE_BRUSH("Old/Graph/Feedback_OKWarn", Icon16x16));
	Set("Graph.ConnectorFeedback.Error", new IMAGE_BRUSH("Old/Graph/Feedback_Error", Icon16x16));
	Set("Graph.ConnectorFeedback.NewNode", new IMAGE_BRUSH("Old/Graph/Feedback_NewNode", Icon16x16));
	Set("Graph.ConnectorFeedback.ViaCast", new IMAGE_BRUSH("Old/Graph/Feedback_ConnectViaCast", Icon16x16));
	Set("Graph.ConnectorFeedback.ShowNode", new IMAGE_BRUSH("Graph/Feedback_ShowNode", Icon16x16));

	Set("ClassIcon.K2Node_CallFunction",	new IMAGE_BRUSH_SVG("Starship/Blueprints/icon_Blueprint_Function", Icon16x16));
	Set("ClassIcon.K2Node_FunctionEntry",	new IMAGE_BRUSH_SVG("Starship/Blueprints/icon_Blueprint_Function", Icon16x16));
	Set("ClassIcon.K2Node_CustomEvent",		new IMAGE_BRUSH_SVG("Starship/Common/Event", Icon16x16));
	Set("ClassIcon.K2Node_Event",			new IMAGE_BRUSH_SVG("Starship/Common/Event", Icon16x16));
	Set("ClassIcon.K2Node_Variable",		new IMAGE_BRUSH_SVG("Starship/GraphEditors/Node", Icon16x16, FLinearColor::White));
	Set("ClassIcon.K2Node_VariableGet",		new IMAGE_BRUSH_SVG("Starship/GraphEditors/VarGet", Icon16x16, FLinearColor::White));
	Set("ClassIcon.K2Node_VariableSet",		new IMAGE_BRUSH_SVG("Starship/GraphEditors/VarSet", Icon16x16, FLinearColor::White));
	Set("ClassIcon.K2Node_DynamicCast",		new IMAGE_BRUSH_SVG("Starship/GraphEditors/Cast", Icon16x16));

	Set("GraphEditor.Default_16x", new IMAGE_BRUSH_SVG("Starship/GraphEditors/Node", Icon16x16));
	Set("GraphEditor.InterfaceFunction_16x", new IMAGE_BRUSH_SVG("Starship/GraphEditors/InterfaceFunction", Icon16x16));
	Set("GraphEditor.PureFunction_16x", new IMAGE_BRUSH_SVG("Starship/Blueprints/icon_Blueprint_Function", Icon16x16));
	Set("GraphEditor.PotentialOverrideFunction_16x", new IMAGE_BRUSH_SVG("Starship/Blueprints/icon_Blueprint_OverrideFunction", Icon16x16));
	Set("GraphEditor.OverridePureFunction_16x", new IMAGE_BRUSH_SVG("Starship/Blueprints/icon_Blueprint_OverrideFunction", Icon16x16));
	Set("GraphEditor.SubGraph_16x", new IMAGE_BRUSH_SVG("Starship/GraphEditors/SubGraph", Icon16x16));
	Set("GraphEditor.Animation_16x", new IMAGE_BRUSH_SVG("Starship/Common/Animation", Icon16x16));
	Set("GraphEditor.Conduit_16x", new IMAGE_BRUSH_SVG("Starship/GraphEditors/Conduit", Icon16x16));
	Set("GraphEditor.Rule_16x", new IMAGE_BRUSH_SVG("Starship/GraphEditors/Rule", Icon16x16));
	Set("GraphEditor.State_16x", new IMAGE_BRUSH_SVG("Starship/GraphEditors/State", Icon16x16));
	Set("GraphEditor.StateMachine_16x", new IMAGE_BRUSH_SVG("Starship/GraphEditors/StateMachine", Icon16x16));
	Set("GraphEditor.Event_16x", new IMAGE_BRUSH_SVG("Starship/Common/Event", Icon16x16));
	Set("GraphEditor.CustomEvent_16x", new IMAGE_BRUSH_SVG("Starship/Common/Event", Icon16x16));
	Set("GraphEditor.CallInEditorEvent_16x", new IMAGE_BRUSH_SVG("Starship/GraphEditors/CallInEditorEvent", Icon16x16));
	Set("GraphEditor.Timeline_16x", new IMAGE_BRUSH_SVG("Starship/Common/Timecode", Icon16x16));
	Set("GraphEditor.Documentation_16x", new IMAGE_BRUSH_SVG("Starship/Common/Documentation", Icon16x16));
	Set("GraphEditor.Switch_16x", new IMAGE_BRUSH_SVG("Starship/GraphEditors/Switch", Icon16x16));
	Set("GraphEditor.BreakStruct_16x", new IMAGE_BRUSH_SVG("Starship/GraphEditors/BreakStruct", Icon16x16));
	Set("GraphEditor.MakeStruct_16x", new IMAGE_BRUSH_SVG("Starship/GraphEditors/MakeStruct", Icon16x16));
	Set("GraphEditor.Sequence_16x", new IMAGE_BRUSH_SVG("Starship/GraphEditors/Sequence", Icon16x16));
	Set("GraphEditor.Branch_16x", new IMAGE_BRUSH_SVG("Starship/GraphEditors/Branch", Icon16x16));
	Set("GraphEditor.SpawnActor_16x", new IMAGE_BRUSH_SVG("Starship/GraphEditors/SpawnActor", Icon16x16));
	Set("GraphEditor.PadEvent_16x", new IMAGE_BRUSH_SVG("Starship/Common/PlayerController", Icon16x16));
	Set("GraphEditor.MouseEvent_16x", new IMAGE_BRUSH_SVG("Starship/GraphEditors/MouseEvent", Icon16x16));
	Set("GraphEditor.KeyEvent_16x", new IMAGE_BRUSH_SVG("Starship/Common/ViewportControls", Icon16x16));
	Set("GraphEditor.TouchEvent_16x", new IMAGE_BRUSH_SVG("Starship/Common/TouchInterface", Icon16x16));
	Set("GraphEditor.MakeArray_16x", new IMAGE_BRUSH_SVG("Starship/GraphEditors/MakeArray", Icon16x16));
	Set("GraphEditor.MakeSet_16x", new IMAGE_BRUSH_SVG("Starship/GraphEditors/MakeSet", Icon16x16));
	Set("GraphEditor.MakeMap_16x", new IMAGE_BRUSH_SVG("Starship/GraphEditors/MakeMap", Icon16x16));
	Set("GraphEditor.Enum_16x", new IMAGE_BRUSH_SVG("Starship/GraphEditors/Enum", Icon16x16));
	Set("GraphEditor.Select_16x", new IMAGE_BRUSH_SVG("Starship/GraphEditors/Select", Icon16x16));
	Set("GraphEditor.Cast_16x", new IMAGE_BRUSH_SVG("Starship/GraphEditors/Cast", Icon16x16));

}

void FStarshipSodaStyle::FStyle::SetupLevelSodaStyle()
{
	// Level editor tool bar icons

	{
		Set("MainFrame.ToggleFullscreen",           new IMAGE_BRUSH_SVG("Starship/Common/EnableFullscreen", Icon16x16));
		Set("MainFrame.LoadLayout",                 new IMAGE_BRUSH_SVG("Starship/Common/LayoutLoad", Icon16x16));
		Set("MainFrame.SaveLayout",                 new IMAGE_BRUSH_SVG("Starship/Common/LayoutSave", Icon16x16));
		Set("MainFrame.RemoveLayout",               new IMAGE_BRUSH_SVG("Starship/Common/LayoutRemove", Icon16x16));

		Set("MainFrame.OpenIssueTracker",           new IMAGE_BRUSH_SVG("Starship/Common/IssueTracker", Icon16x16));
		Set("MainFrame.ReportABug",                 new IMAGE_BRUSH_SVG("Starship/Common/Bug", Icon16x16));

		Set("SystemWideCommands.OpenDocumentation", new IMAGE_BRUSH_SVG("Starship/Common/Documentation", Icon16x16));
		Set("MainFrame.DocumentationHome",	        new IMAGE_BRUSH_SVG("Starship/Common/Documentation", Icon16x16));
		Set("MainFrame.BrowseAPIReference",         new IMAGE_BRUSH_SVG("Starship/Common/Documentation", Icon16x16));
		Set("MainFrame.BrowseCVars",                new IMAGE_BRUSH_SVG("Starship/Common/Console", Icon16x16));
		Set("MainFrame.VisitOnlineLearning",		new IMAGE_BRUSH_SVG("Starship/Common/Tutorials", Icon16x16));
		Set("MainFrame.VisitForums",                new IMAGE_BRUSH_SVG("Starship/Common/Forums", Icon16x16));
		Set("MainFrame.VisitSearchForAnswersPage",  new IMAGE_BRUSH_SVG("Starship/Common/QuestionAnswer", Icon16x16));
		Set("MainFrame.VisitSupportWebSite",        new IMAGE_BRUSH_SVG("Starship/Common/Support", Icon16x16));
		Set("MainFrame.CreditsUnrealEd",            new IMAGE_BRUSH_SVG("Starship/Common/Credits", Icon16x16));

		Set("SodaViewport.SelectMode", new IMAGE_BRUSH_SVG("Starship/EditorViewport/select", Icon16x16) );
		Set("SodaViewport.TranslateMode", new IMAGE_BRUSH_SVG( "Starship/EditorViewport/translate", Icon16x16 ) );
		Set("SodaViewport.RotateMode", new IMAGE_BRUSH_SVG("Starship/EditorViewport/rotate", Icon16x16 ) );
		Set("SodaViewport.ScaleMode", new IMAGE_BRUSH_SVG( "Starship/EditorViewport/scale", Icon16x16 ) );
		Set("SodaViewport.TranslateRotateMode", new IMAGE_BRUSH_SVG("Starship/EditorViewport/TranslateRotate3D", Icon16x16 ) );
		Set("SodaViewport.TranslateRotate2DMode", new IMAGE_BRUSH_SVG("Starship/EditorViewport/TranslateRotate2D", Icon16x16 ) );
		Set("SodaViewport.ToggleRealTime", new IMAGE_BRUSH_SVG("Starship/Common/Realtime", Icon16x16));
		Set("SodaViewport.LocationGridSnap", new IMAGE_BRUSH_SVG("Starship/EditorViewport/grid", Icon16x16));
		Set("SodaViewport.RotationGridSnap", new IMAGE_BRUSH_SVG("Starship/EditorViewport/angle", Icon16x16));
		Set("SodaViewport.ScaleGridSnap", new IMAGE_BRUSH_SVG( "Starship/EditorViewport/scale-grid-snap", Icon16x16 ) );
		Set("SodaViewport.ToggleSurfaceSnapping", new IMAGE_BRUSH_SVG( "Starship/EditorViewport/surface-snap", Icon16x16 ) );
		Set("SodaViewport.ToggleSurfaceSnapping", new IMAGE_BRUSH_SVG("Starship/EditorViewport/surface-snap", Icon16x16));
		Set("SodaViewport.RelativeCoordinateSystem_World", new IMAGE_BRUSH_SVG( "Starship/EditorViewport/globe", Icon16x16 ) );
		Set("SodaViewport.CamSpeedSetting", new IMAGE_BRUSH_SVG( "Starship/EditorViewport/camera", Icon16x16) );
		Set("SodaViewport.LitMode",            	  new IMAGE_BRUSH_SVG("Starship/Common/LitCube", Icon16x16 ) );
		Set("SodaViewport.UnlitMode",          	  new IMAGE_BRUSH_SVG("Starship/Common/UnlitCube", Icon16x16 ) );
		Set("SodaViewport.WireframeMode",      	  new IMAGE_BRUSH_SVG("Starship/Common/BrushWireframe", Icon16x16 ) );
		Set("SodaViewport.DetailLightingMode", 	  new IMAGE_BRUSH_SVG("Starship/Common/DetailLighting", Icon16x16 ) );
		Set("SodaViewport.LightingOnlyMode",   	  new IMAGE_BRUSH_SVG("Starship/Common/LightBulb", Icon16x16 ) );
		Set("SodaViewport.PathTracingMode",   	  new IMAGE_BRUSH_SVG("Starship/Common/PathTracing", Icon16x16 ) );
		Set("SodaViewport.RayTracingDebugMode",    new IMAGE_BRUSH_SVG("Starship/Common/RayTracingDebug", Icon16x16 ) );
		Set("SodaViewport.QuadOverdrawMode", new IMAGE_BRUSH_SVG("Starship/Common/OptimizationViewmodes", Icon16x16 ) );	
		Set("SodaViewport.GroupLODColorationMode", new IMAGE_BRUSH_SVG("Starship/Common/LODColorization", Icon16x16) );
		Set("SodaViewport.VisualizeGBufferMode",   new IMAGE_BRUSH_SVG("Starship/Common/BufferVisualization", Icon16x16) );
		Set("SodaViewport.Visualizers", 			  new IMAGE_BRUSH_SVG("Starship/Common/Visualizer", Icon16x16) );
		Set("SodaViewport.LOD", 			  		  new IMAGE_BRUSH_SVG("Starship/Common/LOD", Icon16x16) );
		Set("SodaViewport.ReflectionOverrideMode", new IMAGE_BRUSH_SVG("Starship/Common/Reflections", Icon16x16 ) );
		Set("SodaViewport.VisualizeBufferMode",    new IMAGE_BRUSH_SVG("Starship/Common/BufferVisualization", Icon16x16 ) );
		Set("SodaViewport.VisualizeNaniteMode",    new IMAGE_BRUSH_SVG("Starship/Common/BufferVisualization", Icon16x16 ) );
		Set("SodaViewport.VisualizeLumenMode",    new IMAGE_BRUSH_SVG("Starship/Common/BufferVisualization", Icon16x16 ) );
		Set("SodaViewport.VisualizeVirtualShadowMapMode", new IMAGE_BRUSH_SVG("Starship/Common/BufferVisualization", Icon16x16 ) );
		Set("SodaViewport.CollisionPawn",          new IMAGE_BRUSH_SVG("Starship/Common/PlayerCollision", Icon16x16 ) );
		Set("SodaViewport.CollisionVisibility",    new IMAGE_BRUSH_SVG("Starship/Common/VisibilityCollision", Icon16x16 ) );
		Set("SodaViewport.Perspective",      new IMAGE_BRUSH_SVG("Starship/Common/ViewPerspective", Icon16x16 ) );
		Set("SodaViewport.OrthographicFree", new IMAGE_BRUSH_SVG("Starship/Common/ViewPerspective", Icon16x16));
		Set("SodaViewport.Top",         new IMAGE_BRUSH_SVG("Starship/Common/ViewTop", Icon16x16 ) );
		Set("SodaViewport.Left",        new IMAGE_BRUSH_SVG("Starship/Common/ViewLeft", Icon16x16 ) );
		Set("SodaViewport.Front",       new IMAGE_BRUSH_SVG("Starship/Common/ViewFront", Icon16x16 ) );
		Set("SodaViewport.Bottom",      new IMAGE_BRUSH_SVG("Starship/Common/ViewBottom", Icon16x16 ) );
		Set("SodaViewport.Right",       new IMAGE_BRUSH_SVG("Starship/Common/ViewRight", Icon16x16 ) );
		Set("SodaViewport.Back",        new IMAGE_BRUSH_SVG("Starship/Common/ViewBack", Icon16x16 ) );
		Set("SodaViewport.ToggleStats", new IMAGE_BRUSH_SVG("Starship/Common/Statistics", Icon16x16)); 
		Set("SodaViewport.ToggleFPS", new IMAGE_BRUSH_SVG("Starship/Common/FPS", Icon16x16));
		Set("SodaViewport.ToggleVehiclePanel", new IMAGE_BRUSH_SVG("Starship/Common/Debug", Icon16x16));
		Set("SodaViewport.ToggleFullScreen", new IMAGE_BRUSH_SVG("Starship/Common/Screen", Icon16x16));
		Set("SodaViewport.ToggleViewportToolbar", new IMAGE_BRUSH_SVG("Starship/Common/Toolbar", Icon16x16));
		Set("SodaViewport.RestartLevel", new IMAGE_BRUSH_SVG("Starship/Common/Update", Icon16x16));
		Set("SodaViewport.ClearLevel", new CORE_IMAGE_BRUSH_SVG("Starship/Common/file", Icon16x16));
		Set("SodaViewport.SubMenu.Stats", new IMAGE_BRUSH_SVG("Starship/Common/Statistics", Icon16x16));
		Set("SodaViewport.SubMenu.Bookmarks", new IMAGE_BRUSH_SVG("Starship/Common/Bookmarks", Icon16x16));
		Set("SodaViewport.SubMenu.CreateCamera", new IMAGE_BRUSH_SVG("Starship/Common/CreateCamera", Icon16x16));
		Set("SodaViewport.SubMenu.Layouts", new IMAGE_BRUSH_SVG("Starship/Common/Layout", Icon16x16));
		Set("SodaViewport.RecordDataset", new CORE_IMAGE_BRUSH_SVG("Starship/Common/file", Icon16x16));
		Set("SodaViewport.AutoConnectDB", new CORE_IMAGE_BRUSH_SVG("Starship/Insights/Connection", Icon16x16));
		Set("SodaViewport.TagActors", new IMAGE_BRUSH_SVG("SodaIcons/hive", Icon16x16));
		Set("SodaViewport.ActiveBorderColor", FStyleColors::Primary);
		Set("SodaViewport.ToggleSpectatorMode", new IMAGE_BRUSH_SVG("Starship/Common/Visibility", Icon16x16));
		Set("SodaViewport.PossesNextVehicle", new IMAGE_BRUSH_SVG("SodaIcons/car", Icon16x16));
		Set("SodaViewport.Exit", new IMAGE_BRUSH_SVG("Starship/Common/Exit", Icon16x16));
		

		Set( "ToolPalette.DockingTab", FCheckBoxStyle()
			.SetCheckBoxType(ESlateCheckBoxType::ToggleButton)
			.SetPadding( FMargin(16.0f, 2.0f, 16.0f, 2.0f ) )
			.SetCheckedImage(         CORE_BOX_BRUSH("Docking/Tab_Shape",  2.f/8.0f, FLinearColor(FColor(62, 62, 62)) ) )
			.SetCheckedHoveredImage(  CORE_BOX_BRUSH("Docking/Tab_Shape",  2.f/8.0f, FLinearColor(FColor(62, 62, 62)) ) )
			.SetCheckedPressedImage(  CORE_BOX_BRUSH("Docking/Tab_Shape",  2.f/8.0f, FLinearColor(FColor(62, 62, 62)) ) )
			.SetUncheckedImage(       CORE_BOX_BRUSH("Docking/Tab_Shape",  2.f/8.0f, FLinearColor(FColor(45, 45, 45)) ) )
			.SetUncheckedHoveredImage(CORE_BOX_BRUSH("Docking/Tab_Shape",2.f/8.0f, FLinearColor(FColor(54, 54, 54)) ) )
			.SetUncheckedPressedImage(CORE_BOX_BRUSH("Docking/Tab_Shape",2.f/8.0f, FLinearColor(FColor(54, 54, 54)) ) )
			.SetUndeterminedImage(        FSlateNoResource() )
			.SetUndeterminedHoveredImage( FSlateNoResource() )
			.SetUndeterminedPressedImage( FSlateNoResource() )
		);	
		Set( "ToolPalette.DockingWell", new FSlateColorBrush(FLinearColor(FColor(34, 34, 34, 255))));

		Set( "ToolPalette.DockingLabel", FTextBlockStyle(NormalText)
			.SetFont( DEFAULT_FONT( "Regular", 9 ) ) 
			.SetShadowOffset(FVector2D(1, 1))
			.SetShadowColorAndOpacity(FLinearColor(0, 0, 0, 0.9f))
		);

		// Top level Actors Menu
		Set( "Actors.Attach", new IMAGE_BRUSH_SVG("Starship/Actors/attach", Icon16x16));
		Set( "Actors.Detach", new IMAGE_BRUSH_SVG("Starship/Actors/detach", Icon16x16));
		Set( "Actors.TakeRecorder", new IMAGE_BRUSH_SVG("Starship/Actors/take-recorder", Icon16x16));
		Set( "Actors.GoHere", new IMAGE_BRUSH_SVG("Starship/Actors/go-here", Icon16x16));
		Set( "Actors.SnapViewToObject", new IMAGE_BRUSH_SVG("Starship/Actors/snap-view-to-object", Icon16x16));
		Set( "Actors.SnapObjectToView", new IMAGE_BRUSH_SVG("Starship/Actors/snap-object-to-view", Icon16x16));
		Set( "Actors.ScripterActorActions", new IMAGE_BRUSH_SVG("Starship/Actors/scripted-actor-actions", Icon16x16));
	}

	{
		Set( "AssetDeleteDialog.Background", new IMAGE_BRUSH( "Common/Selection", Icon8x8, FLinearColor( 0.016, 0.016, 0.016 ) ) );
	}

	{

		Set( "SodaLevelViewport.DebugBorder", new BOX_BRUSH( "Old/Window/ViewportDebugBorder", 0.8f, FLinearColor(.7,0,0,.5) ) );
		Set( "SodaLevelViewport.BlackBackground", new FSlateColorBrush( FLinearColor::Black ) ); 
		Set( "SodaLevelViewport.StartingPlayInEditorBorder", new BOX_BRUSH( "Old/Window/ViewportDebugBorder", 0.8f, FLinearColor(0.1f,1.0f,0.1f,1.0f) ) );
		Set( "SodaLevelViewport.StartingSimulateBorder", new BOX_BRUSH( "Old/Window/ViewportDebugBorder", 0.8f, FLinearColor(1.0f,1.0f,0.1f,1.0f) ) );
		Set( "SodaLevelViewport.NonMaximizedBorder", new BORDER_BRUSH("Common/PlainBorder", 2.f / 8.f, FStyleColors::Black));
		Set( "SodaLevelViewport.ReturningToEditorBorder", new BOX_BRUSH( "Old/Window/ViewportDebugBorder", 0.8f, FLinearColor(0.1f,0.1f,1.0f,1.0f) ) );

		Set( "LevelViewportContextMenu.ActorType.Text", FTextBlockStyle(NormalText)
			.SetFont( DEFAULT_FONT( "Regular", 7 ) )
			.SetColorAndOpacity( FSlateColor::UseSubduedForeground() ) );

		Set( "LevelViewportContextMenu.AssetLabel.Text", FTextBlockStyle(NormalText)
			.SetFont( DEFAULT_FONT( "Regular", 9 ) )
			.SetColorAndOpacity( FSlateColor::UseForeground() ) );

		Set( "LevelViewportContextMenu.AssetTileItem.ThumbnailAreaBackground", new FSlateRoundedBoxBrush(FStyleColors::Recessed, 4.0f) );
		
		FLinearColor TransparentRecessed = FStyleColors::Recessed.GetSpecifiedColor();
		TransparentRecessed.A = 0.3f;
		Set( "LevelViewportContextMenu.AssetTileItem.NameAreaBackground", new FSlateRoundedBoxBrush(TransparentRecessed, 4.0f) );

		Set( "SodaLevelViewport.CursorIcon", new IMAGE_BRUSH( "Common/Cursor", Icon16x16 ) );
	}

	// Show flags menus
	{
		Set( "ShowFlagsMenu.AntiAliasing", new IMAGE_BRUSH_SVG( "Starship/Common/AntiAliasing", Icon16x16 ) );
		Set( "ShowFlagsMenu.Atmosphere", new IMAGE_BRUSH_SVG( "Starship/Common/Atmosphere", Icon16x16 ) );
		Set( "ShowFlagsMenu.BSP", new IMAGE_BRUSH_SVG( "Starship/Common/BSP", Icon16x16 ) );
		Set( "ShowFlagsMenu.Collision", new IMAGE_BRUSH_SVG( "Starship/Common/Collision", Icon16x16 ) );
		Set( "ShowFlagsMenu.Decals", new IMAGE_BRUSH_SVG( "Starship/Common/Decals", Icon16x16 ) );
		Set( "ShowFlagsMenu.Fog", new IMAGE_BRUSH_SVG( "Starship/Common/Fog", Icon16x16 ) );
		Set( "ShowFlagsMenu.Grid", new IMAGE_BRUSH_SVG( "Starship/Common/Grid", Icon16x16 ) );
		Set( "ShowFlagsMenu.MediaPlanes", new IMAGE_BRUSH_SVG( "Starship/Common/MediaPlanes", Icon16x16 ) );
		Set( "ShowFlagsMenu.Navigation", new IMAGE_BRUSH_SVG( "Starship/Common/Navigation", Icon16x16 ) );
		Set( "ShowFlagsMenu.Particles", new IMAGE_BRUSH_SVG( "Starship/Common/ParticleSprites", Icon16x16 ) );
		Set( "ShowFlagsMenu.SkeletalMeshes", new IMAGE_BRUSH_SVG( "Starship/Common/SkeletalMesh", Icon16x16 ) );
		Set( "ShowFlagsMenu.StaticMeshes", new IMAGE_BRUSH_SVG( "Starship/Common/StaticMesh", Icon16x16 ) );
		Set( "ShowFlagsMenu.Translucency", new IMAGE_BRUSH_SVG( "Starship/Common/Transparency", Icon16x16 ) );
		Set( "ShowFlagsMenu.WidgetComponents", new IMAGE_BRUSH_SVG( "Starship/Common/WidgetComponents", Icon16x16 ) );

		Set("ShowFlagsMenu.SubMenu.PostProcessing", new IMAGE_BRUSH_SVG("Starship/Common/PostProcessing", Icon16x16));
		Set("ShowFlagsMenu.SubMenu.LightTypes", new IMAGE_BRUSH_SVG("Starship/Common/LightTypes", Icon16x16));
		Set("ShowFlagsMenu.SubMenu.LightingComponents", new IMAGE_BRUSH_SVG("Starship/Common/LightingComponents", Icon16x16));
		Set("ShowFlagsMenu.SubMenu.LightingFeatures", new IMAGE_BRUSH_SVG("Starship/Common/LightingFeatures", Icon16x16));
		Set("ShowFlagsMenu.SubMenu.Lumen", new IMAGE_BRUSH_SVG("Starship/Common/LightingFeatures", Icon16x16));
		Set("ShowFlagsMenu.SubMenu.Developer", new IMAGE_BRUSH_SVG("Starship/Common/Developer", Icon16x16));
		Set("ShowFlagsMenu.SubMenu.Visualize", new IMAGE_BRUSH_SVG("Starship/Common/Visualize", Icon16x16));
		Set("ShowFlagsMenu.SubMenu.Advanced", new IMAGE_BRUSH_SVG("Starship/Common/Advanced", Icon16x16));

		Set("ShowFlagsMenu.SubMenu.Volumes", new IMAGE_BRUSH_SVG("Starship/Common/Volume", Icon16x16));
		Set("ShowFlagsMenu.SubMenu.Layers", new IMAGE_BRUSH_SVG("Starship/Common/Layers", Icon16x16));
		Set("ShowFlagsMenu.SubMenu.FoliageTypes", new IMAGE_BRUSH_SVG("Starship/Common/FoliageTypes", Icon16x16));
		Set("ShowFlagsMenu.SubMenu.Sprites", new IMAGE_BRUSH_SVG("Starship/Common/Sprite", Icon16x16));
	}


	// Mobility Icons
	{
		Set("Mobility.Movable", new IMAGE_BRUSH("/Icons/Mobility/Movable_16x", Icon16x16));
		Set("Mobility.Stationary", new IMAGE_BRUSH("/Icons/Mobility/Adjustable_16x", Icon16x16));
		Set("Mobility.Static", new IMAGE_BRUSH("/Icons/Mobility/Static_16x", Icon16x16));

		const FString SmallRoundedButton(TEXT("Common/SmallRoundedToggle"));
		const FString SmallRoundedButtonStart(TEXT("Common/SmallRoundedToggleLeft"));
		const FString SmallRoundedButtonMiddle(TEXT("Common/SmallRoundedToggleCenter"));
		const FString SmallRoundedButtonEnd(TEXT("Common/SmallRoundedToggleRight"));

		const FLinearColor NormalColor(0.15, 0.15, 0.15, 1);

		Set("Property.ToggleButton", FCheckBoxStyle()
			.SetCheckBoxType(ESlateCheckBoxType::ToggleButton)
			.SetUncheckedImage(BOX_BRUSH(*SmallRoundedButton, FMargin(7.f / 16.f), NormalColor))
			.SetUncheckedPressedImage(BOX_BRUSH(*SmallRoundedButton, FMargin(7.f / 16.f), SelectionColor_Pressed))
			.SetUncheckedHoveredImage(BOX_BRUSH(*SmallRoundedButton, FMargin(7.f / 16.f), SelectionColor_Pressed))
			.SetCheckedHoveredImage(BOX_BRUSH(*SmallRoundedButton, FMargin(7.f / 16.f), SelectionColor))
			.SetCheckedPressedImage(BOX_BRUSH(*SmallRoundedButton, FMargin(7.f / 16.f), SelectionColor))
			.SetCheckedImage(BOX_BRUSH(*SmallRoundedButton, FMargin(7.f / 16.f), SelectionColor)));

		Set("Property.ToggleButton.Start", FCheckBoxStyle()
			.SetCheckBoxType(ESlateCheckBoxType::ToggleButton)
			.SetUncheckedImage(BOX_BRUSH(*SmallRoundedButtonStart, FMargin(7.f / 16.f), NormalColor))
			.SetUncheckedPressedImage(BOX_BRUSH(*SmallRoundedButtonStart, FMargin(7.f / 16.f), SelectionColor_Pressed))
			.SetUncheckedHoveredImage(BOX_BRUSH(*SmallRoundedButtonStart, FMargin(7.f / 16.f), SelectionColor_Pressed))
			.SetCheckedHoveredImage(BOX_BRUSH(*SmallRoundedButtonStart, FMargin(7.f / 16.f), SelectionColor))
			.SetCheckedPressedImage(BOX_BRUSH(*SmallRoundedButtonStart, FMargin(7.f / 16.f), SelectionColor))
			.SetCheckedImage(BOX_BRUSH(*SmallRoundedButtonStart, FMargin(7.f / 16.f), SelectionColor)));

		Set("Property.ToggleButton.Middle", FCheckBoxStyle()
			.SetCheckBoxType(ESlateCheckBoxType::ToggleButton)
			.SetUncheckedImage(BOX_BRUSH(*SmallRoundedButtonMiddle, FMargin(7.f / 16.f), NormalColor))
			.SetUncheckedPressedImage(BOX_BRUSH(*SmallRoundedButtonMiddle, FMargin(7.f / 16.f), SelectionColor_Pressed))
			.SetUncheckedHoveredImage(BOX_BRUSH(*SmallRoundedButtonMiddle, FMargin(7.f / 16.f), SelectionColor_Pressed))
			.SetCheckedHoveredImage(BOX_BRUSH(*SmallRoundedButtonMiddle, FMargin(7.f / 16.f), SelectionColor))
			.SetCheckedPressedImage(BOX_BRUSH(*SmallRoundedButtonMiddle, FMargin(7.f / 16.f), SelectionColor))
			.SetCheckedImage(BOX_BRUSH(*SmallRoundedButtonMiddle, FMargin(7.f / 16.f), SelectionColor)));

		Set("Property.ToggleButton.End", FCheckBoxStyle()
			.SetCheckBoxType(ESlateCheckBoxType::ToggleButton)
			.SetUncheckedImage(BOX_BRUSH(*SmallRoundedButtonEnd, FMargin(7.f / 16.f), NormalColor))
			.SetUncheckedPressedImage(BOX_BRUSH(*SmallRoundedButtonEnd, FMargin(7.f / 16.f), SelectionColor_Pressed))
			.SetUncheckedHoveredImage(BOX_BRUSH(*SmallRoundedButtonEnd, FMargin(7.f / 16.f), SelectionColor_Pressed))
			.SetCheckedHoveredImage(BOX_BRUSH(*SmallRoundedButtonEnd, FMargin(7.f / 16.f), SelectionColor))
			.SetCheckedPressedImage(BOX_BRUSH(*SmallRoundedButtonEnd, FMargin(7.f / 16.f), SelectionColor))
			.SetCheckedImage(BOX_BRUSH(*SmallRoundedButtonEnd, FMargin(7.f / 16.f), SelectionColor)));
	}

}

void FStarshipSodaStyle::FStyle::SetupPersonaStyle()
{


	// Play in editor / play in world
	{
		// Leftmost button for backplate style toolbar buttons
		FToolBarStyle MainToolbarLeftButton = FStarshipCoreStyle::GetCoreStyle().GetWidgetStyle<FToolBarStyle>("AssetEditorToolbar");

		const FButtonStyle LeftToolbarButton = FButtonStyle(MainToolbarLeftButton.ButtonStyle)
			.SetNormal(FSlateRoundedBoxBrush(FStyleColors::Dropdown, FVector4(4.0f, 0.0f, 0.0f, 4.0f)))
			.SetHovered(FSlateRoundedBoxBrush(FStyleColors::Dropdown, FVector4(4.0f, 0.0f, 0.0f, 4.0f)))
			.SetPressed(FSlateRoundedBoxBrush(FStyleColors::Dropdown, FVector4(4.0f, 0.0f, 0.0f, 4.0f)))
			.SetDisabled(FSlateRoundedBoxBrush(FStyleColors::Dropdown, FVector4(4.0f, 0.0f, 0.0f, 4.0f)))
			.SetNormalPadding(FMargin(8.f, 2.f, 6.f, 2.f))
			.SetPressedPadding(FMargin(8.f, 2.f, 6.f, 2.f));


		MainToolbarLeftButton.SetButtonStyle(LeftToolbarButton);
		MainToolbarLeftButton.SetButtonPadding(FMargin(10.f, 0.0f, 0.0f, 0.0f));
		MainToolbarLeftButton.SetSeparatorPadding(FMargin(0.f, 0.f, 8.f, 0.f));

		Set("Toolbar.BackplateLeft", MainToolbarLeftButton);

		// Specialized Play Button (Left button with green color)
		FLinearColor GreenHSV = FStyleColors::AccentGreen.GetSpecifiedColor().LinearRGBToHSV();
		FLinearColor GreenHover = FLinearColor(GreenHSV.R, GreenHSV.G * .5, GreenHSV.B, GreenHSV.A).HSVToLinearRGB();
		FLinearColor GreenPress = FLinearColor(GreenHSV.R, GreenHSV.G, GreenHSV.B*.5, GreenHSV.A).HSVToLinearRGB(); 

		FToolBarStyle MainToolbarPlayButton = MainToolbarLeftButton;

		const FButtonStyle PlayToolbarButton = FButtonStyle(MainToolbarPlayButton.ButtonStyle)
			.SetNormalForeground(FStyleColors::AccentGreen)
			.SetPressedForeground(GreenPress)
			.SetHoveredForeground(GreenHover);

		MainToolbarPlayButton.SetButtonStyle(PlayToolbarButton);

		Set("Toolbar.BackplateLeftPlay", MainToolbarPlayButton);

		// Center Buttons for backplate style toolbar buttons
		FToolBarStyle MainToolbarCenterButton = MainToolbarLeftButton;

		const FButtonStyle CenterToolbarButton = FButtonStyle(MainToolbarCenterButton.ButtonStyle)
			.SetNormal(FSlateRoundedBoxBrush(FStyleColors::Dropdown, FVector4(0.0f, 0.0f, 0.0f, 0.0f)))
			.SetHovered(FSlateRoundedBoxBrush(FStyleColors::Dropdown, FVector4(0.0f, 0.0f, 0.0f, 0.0f)))
			.SetPressed(FSlateRoundedBoxBrush(FStyleColors::Dropdown, FVector4(0.0f, 0.0f, 0.0f, 0.0f)))
			.SetDisabled(FSlateRoundedBoxBrush(FStyleColors::Dropdown, FVector4(0.0f, 0.0f, 0.0f, 0.0f)))
			.SetNormalPadding(FMargin(2.f, 2.f, 6.f, 2.f))
			.SetPressedPadding(FMargin(2.f, 2.f, 6.f, 2.f));

		MainToolbarCenterButton.SetButtonPadding(0.0f);
		MainToolbarCenterButton.SetButtonStyle(CenterToolbarButton);

		Set("Toolbar.BackplateCenter", MainToolbarCenterButton);

		// Specialized Stop Button (Center button + Red color)

		FLinearColor RedHSV = FStyleColors::AccentRed.GetSpecifiedColor().LinearRGBToHSV();

		FLinearColor RedHover = FLinearColor(RedHSV.R, RedHSV.G * .5, RedHSV.B, RedHSV.A).HSVToLinearRGB();
		FLinearColor RedPress = FLinearColor(RedHSV.R, RedHSV.G, RedHSV.B * .5, RedHSV.A).HSVToLinearRGB();

		FToolBarStyle MainToolbarStopButton = MainToolbarCenterButton;

		const FButtonStyle StopToolbarButton = FButtonStyle(MainToolbarStopButton.ButtonStyle)
			.SetNormalForeground(FStyleColors::AccentRed)
			.SetPressedForeground(RedPress)
			.SetHoveredForeground(RedHover);

		MainToolbarStopButton.SetButtonStyle(StopToolbarButton);

		Set("Toolbar.BackplateCenterStop", MainToolbarStopButton);

		// Rightmost button for backplate style toolbar buttons
		FToolBarStyle MainToolbarRightButton = MainToolbarLeftButton;

		const FButtonStyle RightToolbarButton = FButtonStyle(MainToolbarRightButton.ButtonStyle)
			.SetNormal(FSlateRoundedBoxBrush(FStyleColors::Dropdown, FVector4(0.0f, 4.0f, 4.0f, 0.0f)))
			.SetHovered(FSlateRoundedBoxBrush(FStyleColors::Dropdown, FVector4(0.0f, 4.0f, 4.0f, 0.0f)))
			.SetPressed(FSlateRoundedBoxBrush(FStyleColors::Dropdown, FVector4(0.0f, 4.0f, 4.0f, 0.0f)))
			.SetDisabled(FSlateRoundedBoxBrush(FStyleColors::Dropdown, FVector4(0.0f, 4.0f, 4.0f, 0.0f)))
			.SetNormalPadding(FMargin(2.f, 2.f, 8.f, 2.f))
			.SetPressedPadding(FMargin(2.f, 2.f, 8.f, 2.f));

		MainToolbarRightButton.SetButtonStyle(RightToolbarButton);
		MainToolbarRightButton.SetButtonPadding(FMargin(0.0f, 0.0f, 4.0f, 0.0f));
		MainToolbarRightButton.SetSeparatorPadding(FMargin(4.f, -5.f, 8.f, -5.f));

		Set("Toolbar.BackplateRight", MainToolbarRightButton);

		// Rightmost button for backplate style toolbar buttons as a combo button
		FToolBarStyle MainToolbarRightComboButton = MainToolbarLeftButton;

		const FButtonStyle RightToolbarComboButton = FButtonStyle(MainToolbarRightComboButton.ButtonStyle)
			.SetNormal(FSlateRoundedBoxBrush(FStyleColors::Dropdown, FVector4(0.0f, 4.0f, 4.0f, 0.0f)))
			.SetHovered(FSlateRoundedBoxBrush(FStyleColors::Dropdown, FVector4(0.0f, 4.0f, 4.0f, 0.0f)))
			.SetPressed(FSlateRoundedBoxBrush(FStyleColors::Dropdown, FVector4(0.0f, 4.0f, 4.0f, 0.0f)))
			.SetDisabled(FSlateRoundedBoxBrush(FStyleColors::Dropdown, FVector4(0.0f, 4.0f, 4.0f, 0.0f)))
			.SetNormalPadding(FMargin(7.f, 2.f, 6.f, 2.f))
			.SetPressedPadding(FMargin(7.f, 2.f, 6.f, 2.f));

		FComboButtonStyle PlayToolbarComboButton = FComboButtonStyle(FStarshipCoreStyle::GetCoreStyle().GetWidgetStyle<FComboButtonStyle>("ComboButton"))
			.SetDownArrowPadding(FMargin(-19.f, 0.f, 2.f, 0.f))
			.SetDownArrowImage(CORE_IMAGE_BRUSH_SVG("Starship/Common/ellipsis-vertical-narrow", FVector2D(6, 24)));
		PlayToolbarComboButton.ButtonStyle = RightToolbarComboButton;

		MainToolbarRightComboButton.SetButtonStyle(RightToolbarComboButton);
		MainToolbarRightComboButton.SetComboButtonStyle(PlayToolbarComboButton);
		MainToolbarRightComboButton.SetSeparatorPadding(FMargin(8.f, 0.f, 8.f, 0.f));
		MainToolbarRightComboButton.SetComboButtonPadding(FMargin(1.0f, 0.0f, 8.0f, 0.0f));

		Set("Toolbar.BackplateRightCombo", MainToolbarRightComboButton);

	}

	
	// Property Access 
	{
		Set("PropertyAccess.CompiledContext.Text", FTextBlockStyle(NormalText)
			.SetFont(DEFAULT_FONT("Italic", 8))
			.SetColorAndOpacity(FLinearColor(218.0f/255.0f,218.0f/255.0f,96.0f/255.0f, 0.5f))
		);

		Set("PropertyAccess.CompiledContext.Border", new FSlateRoundedBoxBrush(FStyleColors::DropdownOutline, 2.0f));
	}

	Set("SodaVehicleBar.Save", new IMAGE_BRUSH_SVG("Starship/Common/SaveCurrent", Icon20x20));
	Set("SodaVehicleBar.Export", new CORE_IMAGE_BRUSH_SVG("Starship/Common/export_20", Icon20x20));
	Set("SodaVehicleBar.Import", new CORE_IMAGE_BRUSH_SVG("Starship/Common/import_20", Icon20x20));
	Set("SodaVehicleBar.Components", new IMAGE_BRUSH_SVG("Starship/Common/WorldOutliner", Icon20x20));
	Set("SodaVehicleBar.Reset", new IMAGE_BRUSH_SVG("Starship/Common/Update", Icon20x20));
}

void FStarshipSodaStyle::FStyle::SetupClassIconsAndThumbnails()
{
	// Actor Classes Outliner
	{

		struct FClassIconInfo
		{
			FClassIconInfo(const TCHAR* InType, bool bInHas64Size = true)
				: Type(InType)
				, bHas64Size(bInHas64Size)
			{}

			const TCHAR* Type;
			bool bHas64Size;
		};

		Set("ClassIcon.Light", new IMAGE_BRUSH("Icons/ActorIcons/LightActor_16x", Icon16x16));
		Set("ClassIcon.BrushAdditive", new IMAGE_BRUSH("Icons/ActorIcons/Brush_Add_16x", Icon16x16));
		Set("ClassIcon.BrushSubtractive", new IMAGE_BRUSH("Icons/ActorIcons/Brush_Subtract_16x", Icon16x16));
		Set("ClassIcon.Deleted", new IMAGE_BRUSH("Icons/ActorIcons/DeletedActor_16px", Icon16x16));

		// Component classes
	
		Set("ClassIcon.BlueprintCore", new IMAGE_BRUSH("Icons/AssetIcons/Blueprint_16x", Icon16x16));
		Set("ClassIcon.LightComponent", new IMAGE_BRUSH("Icons/ActorIcons/LightActor_16x", Icon16x16));
		Set("ClassIcon.ArrowComponent", new IMAGE_BRUSH("Icons/ActorIcons/Arrow_16px", Icon16x16));
		Set("ClassIcon.MaterialBillboardComponent", new IMAGE_BRUSH("Icons/ActorIcons/MaterialSprite_16px", Icon16x16));
		Set("ClassIcon.BillboardComponent", new IMAGE_BRUSH("Icons/ActorIcons/SpriteComponent_16px", Icon16x16));
		Set("ClassIcon.TimelineComponent", new IMAGE_BRUSH("Icons/ActorIcons/TimelineComponent_16px", Icon16x16));
		Set("ClassIcon.ChildActorComponent", new IMAGE_BRUSH("Icons/ActorIcons/ChildActorComponent_16px", Icon16x16));

		Set("ClassIcon.AudioComponent", new IMAGE_BRUSH_SVG("Starship/AssetIcons/Audio_16", Icon16x16));
		Set("ClassIcon.BoxComponent", new IMAGE_BRUSH_SVG("Starship/AssetIcons/BoxCollision_16", Icon16x16));
		Set("ClassIcon.CapsuleComponent", new IMAGE_BRUSH_SVG("Starship/AssetIcons/CapsuleCollision_16", Icon16x16));
		Set("ClassIcon.SphereComponent", new IMAGE_BRUSH_SVG("Starship/AssetIcons/SphereCollision_16", Icon16x16));
		Set("ClassIcon.SplineComponent", new IMAGE_BRUSH_SVG("Starship/AssetIcons/Spline_16", Icon16x16));

		Set("ClassIcon.AtmosphericFogComponent", new IMAGE_BRUSH_SVG("Starship/AssetIcons/AtmosphericFog_16", Icon16x16));
		Set("ClassIcon.BrushComponent", new IMAGE_BRUSH_SVG("Starship/AssetIcons/Brush_16", Icon16x16));
		Set("ClassIcon.CableComponent", new IMAGE_BRUSH_SVG("Starship/AssetIcons/CableActor_16", Icon16x16));
		Set("ClassIcon.CameraComponent", new IMAGE_BRUSH_SVG("Starship/AssetIcons/CameraActor_16", Icon16x16));
		Set("ClassIcon.DecalComponent", new IMAGE_BRUSH_SVG("Starship/AssetIcons/DecalActor_16", Icon16x16));
		Set("ClassIcon.DirectionalLightComponent", new IMAGE_BRUSH_SVG("Starship/AssetIcons/DirectionalLight_16", Icon16x16));
		Set("ClassIcon.ExponentialHeightFogComponent", new IMAGE_BRUSH_SVG("Starship/AssetIcons/ExponentialHeightFog_16", Icon16x16));
		Set("ClassIcon.ForceFeedbackComponent", new IMAGE_BRUSH_SVG("Starship/AssetIcons/ForceFeedbackEffect_16", Icon16x16));
		Set("ClassIcon.ParticleSystemComponent", new IMAGE_BRUSH_SVG("Starship/AssetIcons/Emitter_16", Icon16x16));
		Set("ClassIcon.PlanarReflectionComponent", new IMAGE_BRUSH_SVG("Starship/AssetIcons/PlanarReflectionCapture_16", Icon16x16));
		Set("ClassIcon.PointLightComponent", new IMAGE_BRUSH_SVG("Starship/AssetIcons/PointLight_16", Icon16x16));
		Set("ClassIcon.RectLightComponent", new IMAGE_BRUSH_SVG("Starship/AssetIcons/RectLight_16", Icon16x16));
		Set("ClassIcon.RadialForceComponent", new IMAGE_BRUSH_SVG("Starship/AssetIcons/RadialForceActor_16", Icon16x16));
		Set("ClassIcon.SceneCaptureComponent2D", new IMAGE_BRUSH_SVG("Starship/AssetIcons/SceneCapture2D_16", Icon16x16));
		Set("ClassIcon.SceneCaptureComponent", new IMAGE_BRUSH_SVG("Starship/AssetIcons/SphereReflectionCapture_16", Icon16x16));
		Set("ClassIcon.SingleAnimSkeletalComponent", new IMAGE_BRUSH_SVG("Starship/AssetIcons/SkeletalMesh_16", Icon16x16));
		Set("ClassIcon.SkyAtmosphereComponent", new IMAGE_BRUSH_SVG("Starship/AssetIcons/SkyAtmosphere_16", Icon16x16));
		Set("ClassIcon.SkeletalMeshComponent", new IMAGE_BRUSH_SVG("Starship/AssetIcons/SkeletalMesh_16", Icon16x16));
		Set("ClassIcon.SpotLightComponent", new IMAGE_BRUSH_SVG("Starship/AssetIcons/SpotLight_16", Icon16x16));
		Set("ClassIcon.StaticMeshComponent", new IMAGE_BRUSH_SVG("Starship/AssetIcons/StaticMesh_16", Icon16x16));
		Set("ClassIcon.TextRenderComponent", new IMAGE_BRUSH_SVG("Starship/AssetIcons/TextRenderActor_16", Icon16x16));
		Set("ClassIcon.VectorFieldComponent", new IMAGE_BRUSH_SVG("Starship/AssetIcons/VectorFieldVolume_16", Icon16x16));
		Set("ClassIcon.VolumetricCloudComponent", new IMAGE_BRUSH_SVG("Starship/AssetIcons/VolumetricCloud_16", Icon16x16));

		Set("ClassIcon.MovableMobilityIcon", new IMAGE_BRUSH("Icons/ActorIcons/Light_Movable_16x", Icon16x16));
		Set("ClassIcon.StationaryMobilityIcon", new IMAGE_BRUSH("Icons/ActorIcons/Light_Adjustable_16x", Icon16x16));
		Set("ClassIcon.ComponentMobilityHeaderIcon", new IMAGE_BRUSH("Icons/ActorIcons/ComponentMobilityHeader_7x16", Icon7x16));

		// Asset Type Classes
		const TCHAR* AssetTypes[] = {

			TEXT("AbilitySystemComponent"),
			TEXT("AIPerceptionComponent"),
			TEXT("CameraAnim"),
			TEXT("Default"),
			TEXT("DirectionalLightMovable"),
			TEXT("DirectionalLightStatic"),
			TEXT("DirectionalLightStationary"),
			TEXT("FontFace"),
			TEXT("ForceFeedbackEffect"),
			TEXT("InterpData"),
			TEXT("LevelSequence"),
			TEXT("LightmassCharacterIndirectDetailVolume"),
			TEXT("MassiveLODOverrideVolume"),
			TEXT("MaterialParameterCollection"),
			TEXT("MultiFont"),
			TEXT("ParticleSystem"),
			TEXT("PhysicsConstraintComponent"),
			TEXT("PhysicsThrusterComponent"),
			TEXT("SkyLightComponent"),
			TEXT("SlateWidgetStyleAsset"),
			TEXT("StringTable"),
			TEXT("SpotLightMovable"),
			TEXT("SpotLightStatic"),
			TEXT("SpotLightStationary"),
			TEXT("Cube"),
			TEXT("Sphere"),
			TEXT("Cylinder"),
			TEXT("Cone"),
			TEXT("Plane"),
		};

		for (int32 TypeIndex = 0; TypeIndex < UE_ARRAY_COUNT(AssetTypes); ++TypeIndex)
		{
			const TCHAR* Type = AssetTypes[TypeIndex];
			Set( *FString::Printf(TEXT("ClassIcon.%s"), Type),		new IMAGE_BRUSH(FString::Printf(TEXT("Icons/AssetIcons/%s_%dx"), Type, 16), Icon16x16 ) );
		}

		const FClassIconInfo AssetTypesSVG[] = {
			{TEXT("Actor")},
			{TEXT("ActorComponent")},
			{TEXT("AIController")},
			{TEXT("AimOffsetBlendSpace")},
			{TEXT("AimOffsetBlendSpace1D")},
			{TEXT("AmbientSound")},
			{TEXT("AnimationModifier")},
			{TEXT("AnimationSharingSetup")},
			{TEXT("AnimBlueprint")},
			{TEXT("AnimComposite")},
			{TEXT("AnimInstance")},
			{TEXT("AnimLayerInterface")},
			{TEXT("AnimMontage")},
			{TEXT("AnimSequence")},
			{TEXT("ApplicationLifecycleComponent")},
			{TEXT("AtmosphericFog")},
			{TEXT("AudioVolume")},
			{TEXT("BehaviorTree")},
			{TEXT("BlackboardData")},
			{TEXT("BlendSpace")},
			{TEXT("BlendSpace1D")},
			{TEXT("BlockingVolume")},
			{TEXT("Blueprint")},
			{TEXT("BlueprintFunctionLibrary")},
			{TEXT("BlueprintGeneratedClass")},
			{TEXT("BlueprintInterface")},
			{TEXT("BlueprintMacroLibrary")},
			{TEXT("BoxReflectionCapture")},
			{TEXT("Brush")},
			{TEXT("ButtonStyleAsset")},
			{TEXT("CableActor")},
			{TEXT("CameraActor")},
			{TEXT("CameraBlockingVolume")},
			{TEXT("CameraRig_Crane")},
			{TEXT("CameraRig_Rail")},
			{TEXT("Character")},
			{TEXT("CharacterMovementComponent")},
			{TEXT("CineCameraActor")},
			{TEXT("Class")},
			{TEXT("CompositingElement")},
			{TEXT("CullDistanceVolume")},
			{TEXT("CurveBase")},
			{TEXT("DataAsset")},
			{TEXT("DataTable")},
			{TEXT("DecalActor")},
			{TEXT("DefaultPawn")},
			{TEXT("DialogueVoice")},
			{TEXT("DialogueWave")},
			{TEXT("DirectionalLight")},
			{TEXT("DocumentationActor")},
			{TEXT("EditorTutorial")},
			{TEXT("EnvQuery")},
			{TEXT("Emitter")},
			{TEXT("EmptyActor")},
			{TEXT("ExponentialHeightFog")},
			{TEXT("FileMediaOutput")},
			{TEXT("FileMediaSource")},
			{TEXT("FoliageType_Actor")},
			{TEXT("Font")},
			{TEXT("ForceFeedback")},
			{TEXT("GameModeBase")},
			{TEXT("GameStateBase")},
			{TEXT("GeometryCollection")},
			{TEXT("GroupActor")},
			{TEXT("HierarchicalInstancedStaticMeshComponent")},
			{TEXT("HUD")},
			{TEXT("ImagePlate")},
			{TEXT("InstancedStaticMeshComponent")},
			{TEXT("Interface")},
			{TEXT("KillZVolume")},
			{TEXT("LevelBounds")},
			{TEXT("LevelInstance")},
			{TEXT("LevelInstancePivot")},
			{TEXT("PackedLevelActor")},
			{TEXT("LevelScriptActor")},
			{TEXT("LevelSequenceActor")},
			{TEXT("LevelStreamingVolume")},
			{TEXT("LightmassCharacterDetailIndirectVolume")},
			{TEXT("LightmassImportanceVolume")},
			{TEXT("LightmassVolume")},
			{TEXT("LiveLinkPreset")},
			{TEXT("Material")},
			{TEXT("MaterialFunction")},
			{TEXT("MaterialInstanceActor")},
			{TEXT("MaterialInstanceConstant")},
			{TEXT("MediaPlayer")},
			{TEXT("MediaTexture")},
			{TEXT("MirrorDataTable")},
			{TEXT("ModularSynthPresetBank")},
			{TEXT("NavLink")},
			{TEXT("NavLinkProxy")},
			{TEXT("NavMeshBoundsVolume")},
			{TEXT("NavModifierComponent")},
			{TEXT("NavModifierVolume")},
			{TEXT("Note")},
			{TEXT("Object")},
			{TEXT("ObjectLibrary")},
			{TEXT("PainCausingVolume")},
			{TEXT("Pawn")},
			{TEXT("PawnNoiseEmitterComponent")},
			{TEXT("PawnSensingComponent")},
			{TEXT("PhysicalMaterial")},
			{TEXT("PhysicsAsset")},
			{TEXT("PhysicsConstraintActor")},
			{TEXT("PhysicsHandleComponent")},
			{TEXT("PhysicsThruster")},
			{TEXT("PhysicsVolume")},
			{TEXT("PlanarReflectionCapture")},
			{TEXT("PlatformMediaSource")},
			{TEXT("PlayerController")},
			{TEXT("PlayerStart")},
			{TEXT("PointLight")},
			{TEXT("PoseAsset")},
			{TEXT("PostProcessVolume")},
			{TEXT("PrecomputedVisibilityOverrideVolume")},
			{TEXT("PrecomputedVisibilityVolume")},
			{TEXT("ProceduralFoliageBlockingVolume")},
			{TEXT("ProceduralFoliageVolume")},
			{TEXT("ProjectileMovementComponent")},
			{TEXT("RadialForceActor")},
			{TEXT("RectLight")},
			{TEXT("ReflectionCapture")},
			{TEXT("ReverbEffect")},
			{TEXT("RotatingMovementComponent")},
			{TEXT("SceneCapture2D")},
			{TEXT("SceneCaptureCube")},
			{TEXT("SceneComponent")},
			{TEXT("SkeletalMeshActor")},
			{TEXT("Skeleton")},
			{TEXT("SkyAtmosphere")},
			{TEXT("SkyLight")},
			{TEXT("SlateBrushAsset")},
			{TEXT("SoundAttenuation")},
			{TEXT("SoundClass")},
			{TEXT("SoundConcurrency")},
			{TEXT("SoundCue")},
			{TEXT("SoundEffectSourcePreset")},
			{TEXT("SoundMix")},
			{TEXT("SoundSubmix") },
			{TEXT("SphereReflectionCapture")},
			{TEXT("SpotLight")},
			{TEXT("SpringArmComponent")},
			{TEXT("StaticMesh")},
			{TEXT("StaticMeshActor")},
			{TEXT("StreamMediaSource")},
			{TEXT("SubsurfaceProfile")},
			{TEXT("TargetPoint")},
			{TEXT("TemplateSequence")},
			{TEXT("TextRenderActor")},
			{TEXT("Texture2D")},
			{TEXT("TextureRenderTarget2D")},
			{TEXT("TextureRenderTargetCube")},
			{TEXT("TimeCodeSynchronizer")},
			{TEXT("TouchInterface")},
			{TEXT("TriggerBase")},
			{TEXT("TriggerBox")},
			{TEXT("TriggerCapsule")},
			{TEXT("TriggerSphere")},
			{TEXT("TriggerVolume")},
			{TEXT("UserDefinedCaptureProtocol")},
			{TEXT("UserDefinedEnum")},
			{TEXT("UserDefinedStruct") },
			{TEXT("UserWidget")},
			{TEXT("VectorField")},
			{TEXT("VectorFieldVolume")},
			{TEXT("Volume")},
			{TEXT("VolumetricCloud"), false},
			{TEXT("VolumetricLightmapDensityVolume")},
			{TEXT("WidgetBlueprint")},
			{TEXT("WidgetBlueprintGeneratedClass")},
			{TEXT("WindDirectionalSource")},
			{TEXT("World")},
		};
	
		// SVG Asset icons
		{
			for (int32 TypeIndex = 0; TypeIndex < UE_ARRAY_COUNT(AssetTypesSVG); ++TypeIndex)
			{
				const FClassIconInfo& Info = AssetTypesSVG[TypeIndex];

				// Look up if the brush already exists to audit old vs new icons during starship development.
				FString ClassIconName = FString::Printf(TEXT("ClassIcon.%s"), Info.Type);
				if (GetOptionalBrush(*ClassIconName, nullptr, nullptr))
				{
					UE_LOG(LogSlate, Log, TEXT("%s already found"), *ClassIconName);
				}

				Set(*FString::Printf(TEXT("ClassIcon.%s"), Info.Type), new IMAGE_BRUSH_SVG(FString::Printf(TEXT("Starship/AssetIcons/%s_%d"), Info.Type, 16), Icon16x16));
				if (Info.bHas64Size)
				{
					Set(*FString::Printf(TEXT("ClassThumbnail.%s"), Info.Type), new IMAGE_BRUSH_SVG(FString::Printf(TEXT("Starship/AssetIcons/%s_%d"), Info.Type, 64), Icon64x64));
				}
				else
				{
					// Temp to avoid missing icons while in progress. use the 64 variant for 16 for now.  
					Set(*FString::Printf(TEXT("ClassThumbnail.%s"), Info.Type), new IMAGE_BRUSH_SVG(FString::Printf(TEXT("Starship/AssetIcons/%s_%d"), Info.Type, 16), Icon64x64));
				}
			}
		}
	}

}

void FStarshipSodaStyle::FStyle::SetupColorPickerStyle()
{
	Set("ColorPicker.ColorThemes", new IMAGE_BRUSH_SVG("Starship/ColorPicker/ColorThemes", Icon16x16));
}


void FStarshipSodaStyle::FStyle::SetupTutorialStyles()
{
	// Documentation tooltip defaults
	const FSlateColor HyperlinkColor(FLinearColor(0.1f, 0.1f, 0.5f));
	{
		const FTextBlockStyle DocumentationTooltipText = FTextBlockStyle(NormalText)
			.SetFont(DEFAULT_FONT("Regular", 9))
			.SetColorAndOpacity(FLinearColor::Black);
		Set("Documentation.SDocumentationTooltip", FTextBlockStyle(DocumentationTooltipText));

		const FTextBlockStyle DocumentationTooltipTextSubdued = FTextBlockStyle(NormalText)
			.SetFont(DEFAULT_FONT("Regular", 8))
			.SetColorAndOpacity(FLinearColor(0.1f, 0.1f, 0.1f));
		Set("Documentation.SDocumentationTooltipSubdued", FTextBlockStyle(DocumentationTooltipTextSubdued));

		const FTextBlockStyle DocumentationTooltipHyperlinkText = FTextBlockStyle(NormalText)
			.SetFont(DEFAULT_FONT("Regular", 8))
			.SetColorAndOpacity(HyperlinkColor);
		Set("Documentation.SDocumentationTooltipHyperlinkText", FTextBlockStyle(DocumentationTooltipHyperlinkText));

		const FButtonStyle DocumentationTooltipHyperlinkButton = FButtonStyle()
			.SetNormal(BORDER_BRUSH("Old/HyperlinkDotted", FMargin(0, 0, 0, 3 / 16.0f), HyperlinkColor))
			.SetPressed(FSlateNoResource())
			.SetHovered(BORDER_BRUSH("Old/HyperlinkUnderline", FMargin(0, 0, 0, 3 / 16.0f), HyperlinkColor));
		Set("Documentation.SDocumentationTooltipHyperlinkButton", FButtonStyle(DocumentationTooltipHyperlinkButton));
	}


	// Documentation defaults
	const FTextBlockStyle DocumentationText = FTextBlockStyle(NormalText)
		.SetColorAndOpacity(FLinearColor::Black)
		.SetFont(DEFAULT_FONT("Regular", 11));

	const FTextBlockStyle DocumentationHyperlinkText = FTextBlockStyle(DocumentationText)
		.SetColorAndOpacity(HyperlinkColor);

	const FTextBlockStyle DocumentationHeaderText = FTextBlockStyle(NormalText)
		.SetColorAndOpacity(FLinearColor::Black)
		.SetFont(DEFAULT_FONT("Black", 32));

	const FButtonStyle DocumentationHyperlinkButton = FButtonStyle()
		.SetNormal(BORDER_BRUSH("Old/HyperlinkDotted", FMargin(0, 0, 0, 3 / 16.0f), HyperlinkColor))
		.SetPressed(FSlateNoResource())
		.SetHovered(BORDER_BRUSH("Old/HyperlinkUnderline", FMargin(0, 0, 0, 3 / 16.0f), HyperlinkColor));

	// Documentation
	{
		Set("Documentation.Content", FTextBlockStyle(DocumentationText));

		const FHyperlinkStyle DocumentationHyperlink = FHyperlinkStyle()
			.SetUnderlineStyle(DocumentationHyperlinkButton)
			.SetTextStyle(DocumentationText)
			.SetPadding(FMargin(0.0f));
		Set("Documentation.Hyperlink", DocumentationHyperlink);

		Set("Documentation.Hyperlink.Button", FButtonStyle(DocumentationHyperlinkButton));
		Set("Documentation.Hyperlink.Text", FTextBlockStyle(DocumentationHyperlinkText));
		Set("Documentation.NumberedContent", FTextBlockStyle(DocumentationText));
		Set("Documentation.BoldContent", FTextBlockStyle(DocumentationText)
			.SetTypefaceFontName(TEXT("Bold")));

		Set("Documentation.Header1", FTextBlockStyle(DocumentationHeaderText)
			.SetFontSize(32));

		Set("Documentation.Header2", FTextBlockStyle(DocumentationHeaderText)
			.SetFontSize(24));

		Set("Documentation.Separator", new BOX_BRUSH("Common/Separator", 1 / 4.0f, FLinearColor(1, 1, 1, 0.5f)));
	}

	// Tutorials
	{
		const FLinearColor TutorialButtonColor = FLinearColor(0.15f, 0.15f, 0.15f, 1.0f);
		const FLinearColor TutorialSelectionColor = FLinearColor(0.19f, 0.33f, 0.72f);
		const FLinearColor TutorialNavigationButtonColor = FLinearColor(0.0f, 0.59f, 0.14f, 1.0f);
		const FLinearColor TutorialNavigationButtonHoverColor = FLinearColor(0.2f, 0.79f, 0.34f, 1.0f);
		const FLinearColor TutorialNavigationBackButtonColor = TutorialNavigationButtonColor;
		const FLinearColor TutorialNavigationBackButtonHoverColor = TutorialNavigationButtonHoverColor;

		const FTextBlockStyle TutorialText = FTextBlockStyle(DocumentationText)
			.SetColorAndOpacity(FLinearColor::Black)
			.SetHighlightColor(TutorialSelectionColor);

		const FTextBlockStyle TutorialHeaderText = FTextBlockStyle(DocumentationHeaderText)
			.SetColorAndOpacity(FLinearColor::Black)
			.SetHighlightColor(TutorialSelectionColor);

		const FTextBlockStyle TutorialBrowserText = FTextBlockStyle(TutorialText)
			.SetColorAndOpacity(FSlateColor::UseForeground())
			.SetHighlightColor(TutorialSelectionColor);

		Set("Tutorials.Browser.Text", TutorialBrowserText);

		Set("Tutorials.Browser.WelcomeHeader", FTextBlockStyle(TutorialBrowserText)
			.SetFontSize(20));

		Set("Tutorials.Browser.SummaryHeader", FTextBlockStyle(TutorialBrowserText)
			.SetFontSize(16));

		Set("Tutorials.Browser.SummaryText", FTextBlockStyle(TutorialBrowserText)
			.SetFontSize(10));

		Set("Tutorials.Browser.HighlightTextColor", TutorialSelectionColor);

		Set("Tutorials.Browser.Button", FButtonStyle()
			.SetNormal(BOX_BRUSH("Common/ButtonHoverHint", FMargin(4 / 16.0f), FLinearColor(0.05f, 0.05f, 0.05f, 1)))
			.SetHovered(BOX_BRUSH("Common/ButtonHoverHint", FMargin(4 / 16.0f), FLinearColor(0.07f, 0.07f, 0.07f, 1)))
			.SetPressed(BOX_BRUSH("Common/ButtonHoverHint", FMargin(4 / 16.0f), FLinearColor(0.08f, 0.08f, 0.08f, 1)))
			.SetNormalPadding(FMargin(0, 0, 0, 1))
			.SetPressedPadding(FMargin(0, 1, 0, 0)));

		Set("Tutorials.Browser.BackButton", FButtonStyle()
			.SetNormal(BOX_BRUSH("Common/ButtonHoverHint", FMargin(4 / 16.0f), FLinearColor(1.0f, 1.0f, 1.0f, 0.0f)))
			.SetHovered(BOX_BRUSH("Common/ButtonHoverHint", FMargin(4 / 16.0f), FLinearColor(1.0f, 1.0f, 1.0f, 0.05f)))
			.SetPressed(BOX_BRUSH("Common/ButtonHoverHint", FMargin(4 / 16.0f), FLinearColor(1.0f, 1.0f, 1.0f, 0.05f)))
			.SetNormalPadding(FMargin(0, 0, 0, 1))
			.SetPressedPadding(FMargin(0, 1, 0, 0)));

		Set("Tutorials.Content.Button", FButtonStyle()
			.SetNormal(BOX_BRUSH("Common/ButtonHoverHint", FMargin(4 / 16.0f), FLinearColor(0, 0, 0, 0)))
			.SetHovered(BOX_BRUSH("Common/ButtonHoverHint", FMargin(4 / 16.0f), FLinearColor(1, 1, 1, 1)))
			.SetPressed(BOX_BRUSH("Common/ButtonHoverHint", FMargin(4 / 16.0f), FLinearColor(1, 1, 1, 1)))
			.SetNormalPadding(FMargin(0, 0, 0, 1))
			.SetPressedPadding(FMargin(0, 1, 0, 0)));

		Set("Tutorials.Content.NavigationButtonWrapper", FButtonStyle()
			.SetNormal(FSlateNoResource())
			.SetHovered(FSlateNoResource())
			.SetPressed(FSlateNoResource())
			.SetNormalPadding(FMargin(0, 0, 0, 1))
			.SetPressedPadding(FMargin(0, 1, 0, 0)));

		Set("Tutorials.Content.NavigationButton", FButtonStyle()
			.SetNormal(BOX_BRUSH("Common/ButtonHoverHint", FMargin(4 / 16.0f), TutorialNavigationButtonColor))
			.SetHovered(BOX_BRUSH("Common/ButtonHoverHint", FMargin(4 / 16.0f), TutorialNavigationButtonHoverColor))
			.SetPressed(BOX_BRUSH("Common/ButtonHoverHint", FMargin(4 / 16.0f), TutorialNavigationButtonHoverColor))
			.SetNormalPadding(FMargin(0, 0, 0, 1))
			.SetPressedPadding(FMargin(0, 1, 0, 0)));

		Set("Tutorials.Content.NavigationBackButton", FButtonStyle()
			.SetNormal(BOX_BRUSH("Common/ButtonHoverHint", FMargin(4 / 16.0f), TutorialNavigationBackButtonColor))
			.SetHovered(BOX_BRUSH("Common/ButtonHoverHint", FMargin(4 / 16.0f), TutorialNavigationBackButtonHoverColor))
			.SetPressed(BOX_BRUSH("Common/ButtonHoverHint", FMargin(4 / 16.0f), TutorialNavigationBackButtonHoverColor))
			.SetNormalPadding(FMargin(0, 0, 0, 1))
			.SetPressedPadding(FMargin(0, 1, 0, 0)));

		Set("Tutorials.Content.NavigationText", FTextBlockStyle(TutorialText));

		Set("Tutorials.Content.Color", FLinearColor(1.0f, 1.0f, 1.0f, 0.9f));
		Set("Tutorials.Content.Color.Hovered", FLinearColor(1.0f, 1.0f, 1.0f, 1.0f));

		Set("Tutorials.Navigation.Button", FButtonStyle()
			.SetNormal(BOX_BRUSH("Common/ButtonHoverHint", FMargin(4 / 16.0f), FLinearColor(0, 0, 0, 0)))
			.SetHovered(BOX_BRUSH("Common/ButtonHoverHint", FMargin(4 / 16.0f), FLinearColor(0, 0, 0, 0)))
			.SetPressed(BOX_BRUSH("Common/ButtonHoverHint", FMargin(4 / 16.0f), FLinearColor(0, 0, 0, 0)))
			.SetNormalPadding(FMargin(0, 0, 0, 1))
			.SetPressedPadding(FMargin(0, 1, 0, 0)));

		Set("Tutorials.WidgetContent", FTextBlockStyle(TutorialText)
			.SetFontSize(10));

		Set("Tutorials.ButtonColor", TutorialButtonColor);
		Set("Tutorials.ButtonHighlightColor", TutorialSelectionColor);
		Set("Tutorials.ButtonDisabledColor", SelectionColor_Inactive);

		Set("Tutorials.PageHeader", FTextBlockStyle(TutorialHeaderText)
			.SetFontSize(22));

		Set("Tutorials.CurrentExcerpt", FTextBlockStyle(TutorialHeaderText)
			.SetFontSize(16));

		Set("Tutorials.NavigationButtons", FTextBlockStyle(TutorialHeaderText)
			.SetFontSize(16));

		// UDN documentation styles
		Set("Tutorials.Content", FTextBlockStyle(TutorialText)
			.SetColorAndOpacity(FSlateColor::UseForeground()));
		Set("Tutorials.Hyperlink.Text", FTextBlockStyle(DocumentationHyperlinkText));
		Set("Tutorials.NumberedContent", FTextBlockStyle(TutorialText));
		Set("Tutorials.BoldContent", FTextBlockStyle(TutorialText)
			.SetTypefaceFontName(TEXT("Bold")));

		Set("Tutorials.Header1", FTextBlockStyle(TutorialHeaderText)
			.SetFontSize(32));

		Set("Tutorials.Header2", FTextBlockStyle(TutorialHeaderText)
			.SetFontSize(24));

		Set("Tutorials.Hyperlink.Button", FButtonStyle(DocumentationHyperlinkButton)
			.SetNormal(BORDER_BRUSH("Old/HyperlinkDotted", FMargin(0, 0, 0, 3 / 16.0f), FLinearColor::Black))
			.SetHovered(BORDER_BRUSH("Old/HyperlinkUnderline", FMargin(0, 0, 0, 3 / 16.0f), FLinearColor::Black)));

		Set("Tutorials.Separator", new BOX_BRUSH("Common/Separator", 1 / 4.0f, FLinearColor::Black));

		Set("Tutorials.ProgressBar", FProgressBarStyle()
			.SetBackgroundImage(BOX_BRUSH("Common/ProgressBar_Background", FMargin(5.f / 12.f)))
			.SetFillImage(BOX_BRUSH("Common/ProgressBar_NeutralFill", FMargin(5.f / 12.f)))
			.SetMarqueeImage(IMAGE_BRUSH("Common/ProgressBar_Marquee", FVector2D(20, 12), FLinearColor::White, ESlateBrushTileType::Horizontal))
		);

		// Default text styles
		{
			const FTextBlockStyle RichTextNormal = FTextBlockStyle()
				.SetFont(DEFAULT_FONT("Regular", 11))
				.SetColorAndOpacity(FSlateColor::UseForeground())
				.SetShadowOffset(FVector2D::ZeroVector)
				.SetShadowColorAndOpacity(FLinearColor::Black)
				.SetHighlightColor(FLinearColor(0.02f, 0.3f, 0.0f))
				.SetHighlightShape(BOX_BRUSH("Common/TextBlockHighlightShape", FMargin(3.f / 8.f)));
			Set("Tutorials.Content.Text", RichTextNormal);

			Set("Tutorials.Content.TextBold", FTextBlockStyle(RichTextNormal)
				.SetFont(DEFAULT_FONT("Bold", 11)));

			Set("Tutorials.Content.HeaderText1", FTextBlockStyle(RichTextNormal)
				.SetFontSize(20));

			Set("Tutorials.Content.HeaderText2", FTextBlockStyle(RichTextNormal)
				.SetFontSize(16));

			{
				const FButtonStyle RichTextHyperlinkButton = FButtonStyle()
					.SetNormal(BORDER_BRUSH("Old/HyperlinkDotted", FMargin(0, 0, 0, 3 / 16.0f), FLinearColor::Blue))
					.SetPressed(FSlateNoResource())
					.SetHovered(BORDER_BRUSH("Old/HyperlinkUnderline", FMargin(0, 0, 0, 3 / 16.0f), FLinearColor::Blue));

				const FTextBlockStyle RichTextHyperlinkText = FTextBlockStyle(RichTextNormal)
					.SetColorAndOpacity(FLinearColor::Blue);

				Set("Tutorials.Content.HyperlinkText", RichTextHyperlinkText);

				// legacy style
				Set("TutorialEditableText.Editor.HyperlinkText", RichTextHyperlinkText);

				const FHyperlinkStyle RichTextHyperlink = FHyperlinkStyle()
					.SetUnderlineStyle(RichTextHyperlinkButton)
					.SetTextStyle(RichTextHyperlinkText)
					.SetPadding(FMargin(0.0f));
				Set("Tutorials.Content.Hyperlink", RichTextHyperlink);

				// legacy style
				Set("TutorialEditableText.Editor.Hyperlink", RichTextHyperlink);
			}
		}

		// Toolbar
		{
			const FLinearColor NormalColor(FColor(0xffeff3f3));
			const FLinearColor SelectedColor(FColor(0xffdbe4d5));
			const FLinearColor HoverColor(FColor(0xffdbe4e4));
			const FLinearColor DisabledColor(FColor(0xaaaaaa));
			const FLinearColor TextColor(FColor(0xff2c3e50));

			Set("TutorialEditableText.RoundedBackground", new BOX_BRUSH("Common/RoundedSelection_16x", 4.0f / 16.0f, FLinearColor(FColor(0xffeff3f3))));

			Set("TutorialEditableText.Toolbar.TextColor", TextColor);

			Set("TutorialEditableText.Toolbar.Text", FTextBlockStyle(NormalText)
				.SetFont(DEFAULT_FONT("Regular", 10))
				.SetColorAndOpacity(TextColor)
			);

			Set("TutorialEditableText.Toolbar.BoldText", FTextBlockStyle(NormalText)
				.SetFont(DEFAULT_FONT("Bold", 10))
				.SetColorAndOpacity(TextColor)
			);

			Set("TutorialEditableText.Toolbar.ItalicText", FTextBlockStyle(NormalText)
				.SetFont(DEFAULT_FONT("Italic", 10))
				.SetColorAndOpacity(TextColor)
			);

			Set("TutorialEditableText.Toolbar.Checkbox", FCheckBoxStyle()
				.SetCheckBoxType(ESlateCheckBoxType::CheckBox)
				.SetUncheckedImage(IMAGE_BRUSH("Common/CheckBox", Icon16x16, FLinearColor::White))
				.SetUncheckedHoveredImage(IMAGE_BRUSH("Common/CheckBox", Icon16x16, HoverColor))
				.SetUncheckedPressedImage(IMAGE_BRUSH("Common/CheckBox_Hovered", Icon16x16, HoverColor))
				.SetCheckedImage(IMAGE_BRUSH("Common/CheckBox_Checked_Hovered", Icon16x16, FLinearColor::White))
				.SetCheckedHoveredImage(IMAGE_BRUSH("Common/CheckBox_Checked_Hovered", Icon16x16, HoverColor))
				.SetCheckedPressedImage(IMAGE_BRUSH("Common/CheckBox_Checked", Icon16x16, HoverColor))
				.SetUndeterminedImage(IMAGE_BRUSH("Common/CheckBox_Undetermined", Icon16x16, FLinearColor::White))
				.SetUndeterminedHoveredImage(IMAGE_BRUSH("Common/CheckBox_Undetermined_Hovered", Icon16x16, HoverColor))
				.SetUndeterminedPressedImage(IMAGE_BRUSH("Common/CheckBox_Undetermined_Hovered", Icon16x16, FLinearColor::White))
			);
		}

	}
}

void FStarshipSodaStyle::FStyle::SetupScenarioEditorStyle()
{
	Set("ScenarioAction.EventNode", new FSlateRoundedBoxBrush(FStyleColors::Recessed, 12.f, FStyleColors::AccentRed, 1.0));
	Set("ScenarioAction.ConditionalNode", new FSlateRoundedBoxBrush(FStyleColors::Recessed, 12.f, FStyleColors::AccentOrange, 1.0));
	Set("ScenarioAction.ActionNode", new FSlateRoundedBoxBrush(FStyleColors::Panel, 12.f, FStyleColors::AccentGreen, 1.0));

	Set("ScenarioAction.EventMark", new FSlateRoundedBoxBrush(FStyleColors::AccentRed, 2.f, FStyleColors::AccentRed, 1.0));
	Set("ScenarioAction.ConditionalMark", new FSlateRoundedBoxBrush(FStyleColors::AccentOrange, 2.f, FStyleColors::AccentOrange, 1.0));
	Set("ScenarioAction.ActionMark", new FSlateRoundedBoxBrush(FStyleColors::AccentGreen, 2.f, FStyleColors::AccentGreen, 1.0));

	Set("ScenarioAction.AddConditionalButton", FButtonStyle(Button)
		.SetNormal(FSlateRoundedBoxBrush(FStyleColors::Recessed, 12.f, FStyleColors::AccentOrange, 1.0))
		.SetHovered(FSlateRoundedBoxBrush(FStyleColors::Dropdown, 12.f, FStyleColors::AccentOrange, 1.0))
		.SetPressed(FSlateRoundedBoxBrush(FStyleColors::Dropdown, 12.f, FStyleColors::AccentOrange, 1.0))
	);

	Set("ScenarioAction.AddEventButton", FButtonStyle(Button)
		.SetNormal(FSlateRoundedBoxBrush(FStyleColors::Recessed, 12.f, FStyleColors::AccentRed, 1.0))
		.SetHovered(FSlateRoundedBoxBrush(FStyleColors::Dropdown, 12.f, FStyleColors::AccentRed, 1.0))
		.SetPressed(FSlateRoundedBoxBrush(FStyleColors::Dropdown, 12.f, FStyleColors::AccentRed, 1.0))
	);

	Set("ScenarioAction.AddActionButton", FButtonStyle(Button)
		.SetNormal(FSlateRoundedBoxBrush(FStyleColors::Recessed, 12.f, FStyleColors::AccentGreen, 1.0))
		.SetHovered(FSlateRoundedBoxBrush(FStyleColors::Dropdown, 12.f, FStyleColors::AccentGreen, 1.0))
		.SetPressed(FSlateRoundedBoxBrush(FStyleColors::Dropdown, 12.f, FStyleColors::AccentGreen, 1.0))
	);

	Set("ScenarioAction.ConditionalSeparator", new FSlateRoundedBoxBrush(FStyleColors::Dropdown, 2.f, FStyleColors::Dropdown, 0));

	Set("ScenarioAction.ActionRowDefault", FTableRowStyle(NormalTableRowStyle)
		.SetEvenRowBackgroundBrush(FSlateColorBrush(FStyleColors::Panel))
		.SetEvenRowBackgroundHoveredBrush(FSlateColorBrush(FStyleColors::Header))
		.SetOddRowBackgroundBrush(FSlateColorBrush(FStyleColors::Panel))
		.SetOddRowBackgroundHoveredBrush(FSlateColorBrush(FStyleColors::Header)));
		//.SetSelectorFocusedBrush(BORDER_BRUSH("Common/Selector", FMargin(4.f / 16.f), SelectorColor))
		//.SetActiveBrush(IMAGE_BRUSH("Common/Selection", Icon8x8, SelectionColor))
		//.SetActiveHoveredBrush(IMAGE_BRUSH("Common/Selection", Icon8x8, SelectionColor))
		//.SetInactiveBrush(IMAGE_BRUSH("Common/Selection", Icon8x8, SelectionColor_Inactive))
		//.SetInactiveHoveredBrush(IMAGE_BRUSH("Common/Selection", Icon8x8, SelectionColor_Inactive)));

	Set("ScenarioAction.ActionRowContent", FTableRowStyle(NormalTableRowStyle)
		.SetEvenRowBackgroundBrush(FSlateNoResource())
		.SetEvenRowBackgroundHoveredBrush(FSlateNoResource())
		.SetOddRowBackgroundBrush(FSlateNoResource())
		.SetOddRowBackgroundHoveredBrush(FSlateNoResource())
		.SetSelectorFocusedBrush(FSlateNoResource())
		.SetActiveBrush(FSlateNoResource())
		.SetActiveHoveredBrush(FSlateNoResource())
		.SetInactiveBrush(FSlateNoResource())
		.SetInactiveHoveredBrush(FSlateNoResource()));

	Set("ScenarioAction.ButtonAddRow", FButtonStyle(Button)
		.SetNormal(FSlateRoundedBoxBrush(FStyleColors::Transparent, 4.0f, FStyleColors::Transparent, 1))
		.SetHovered(FSlateRoundedBoxBrush(FStyleColors::Hover, 4.0f, FStyleColors::Transparent, 1))
		.SetPressed(FSlateRoundedBoxBrush(FStyleColors::Header, 4.0f, FStyleColors::Transparent, 1))
		.SetDisabled(FSlateRoundedBoxBrush(FStyleColors::Dropdown, 4.0f, FStyleColors::Transparent, 1))
		.SetNormalForeground(FStyleColors::Transparent)
		.SetHoveredForeground(FStyleColors::ForegroundHover)
		.SetPressedForeground(FStyleColors::ForegroundHover)
		.SetDisabledForeground(FStyleColors::Foreground));
		//.SetNormalPadding(ButtonMargins)
		//.SetPressedPadding(PressedButtonMargins));

	Set("ScenarioAction.OpenEditor", new IMAGE_BRUSH_SVG("Starship/AssetIcons/LevelSequenceActor_16", Icon20x20));
}

void FStarshipSodaStyle::FStyle::SetupPakWindowStyle()
{
	const FTextBlockStyle ButtonText = FAppStyle::Get().GetWidgetStyle<FTextBlockStyle>("PrimaryButtonText");
	FTextBlockStyle LargeText = FAppStyle::Get().GetWidgetStyle<FTextBlockStyle>("Text.Large");

	const float IconSize = 16.0f;
	const float PaddingAmount = 2.0f;

	/*
	Set("Plugins.TabIcon", new IMAGE_BRUSH_SVG("Plugins", Icon16x16));
	Set("Plugins.BreadcrumbArrow", new IMAGE_BRUSH("SmallArrowRight", Icon10x10));
	Set("Plugins.ListBorder", new FSlateRoundedBoxBrush(FStyleColors::Recessed, 4.0f));
	Set("Plugins.RestartWarningBorder", new FSlateRoundedBoxBrush(FStyleColors::Panel, 5.0f, FStyleColors::Warning, 1.0f));

	FTextBlockStyle WarningText = FTextBlockStyle(NormalText)
		.SetColorAndOpacity(FStyleColors::White);

	Set("Plugins.WarningText", WarningText);

	
	//Category Tree Item


	
	Set("CategoryTreeItem.IconSize", IconSize);
	Set("CategoryTreeItem.PaddingAmount", PaddingAmount);

	Set("CategoryTreeItem.BuiltIn", new IMAGE_BRUSH("icon_plugins_builtin_20x", Icon20x20));
	Set("CategoryTreeItem.Installed", new IMAGE_BRUSH("icon_plugins_installed_20x", Icon20x20));
	Set("CategoryTreeItem.LeafItemWithPlugin", new IMAGE_BRUSH("hiererchy_16x", Icon12x12));
	Set("CategoryTreeItem.ExpandedCategory", new IMAGE_BRUSH("FolderOpen", FVector2D(18, 16)));
	Set("CategoryTreeItem.Category", new IMAGE_BRUSH("FolderClosed", FVector2D(18, 16)));

	//Root Category Tree Item
	const float ExtraTopPadding = 12.f;
	const float ExtraBottomPadding = 8.f;
	const float AllPluginsExtraTopPadding = 9.f;
	const float AllPluginsExtraBottomPadding = 7.f;

	Set("CategoryTreeItem.Root.BackgroundBrush", new FSlateNoResource);
	Set("CategoryTreeItem.Root.BackgroundPadding", FMargin(PaddingAmount, PaddingAmount + ExtraTopPadding, PaddingAmount, PaddingAmount + ExtraBottomPadding));
	Set("CategoryTreeItem.Root.AllPluginsBackgroundPadding", FMargin(PaddingAmount, PaddingAmount + AllPluginsExtraTopPadding, PaddingAmount, PaddingAmount + AllPluginsExtraBottomPadding));

	FTextBlockStyle Text = FTextBlockStyle(ButtonText);
	Text.ColorAndOpacity = FStyleColors::Foreground;
	Text.TransformPolicy = ETextTransformPolicy::ToUpper;
	Set("CategoryTreeItem.Root.Text", Text);


	FTextBlockStyle RootPluginCountText = FTextBlockStyle(NormalText);
	Set("CategoryTreeItem.Root.PluginCountText", RootPluginCountText);

	//Subcategory Tree Item
	Set("CategoryTreeItem.BackgroundBrush", new FSlateNoResource);
	Set("CategoryTreeItem.BackgroundPadding", FMargin(PaddingAmount));


	FTextBlockStyle CategoryText = FTextBlockStyle(NormalText);
	CategoryText.ColorAndOpacity = FStyleColors::Foreground;
	Set("CategoryTreeItem.Text", CategoryText);

	FTextBlockStyle PluginCountText = FTextBlockStyle(NormalText);
	Set("CategoryTreeItem.PluginCountText", PluginCountText);
	*/

	//Plugin Tile
	Set("PakItem.RestrictedBorderImage", new FSlateRoundedBoxBrush(FStyleColors::AccentRed, 8.f));
	Set("PakItem.BetaBorderImage", new FSlateRoundedBoxBrush(FStyleColors::AccentOrange.GetSpecifiedColor().CopyWithNewOpacity(0.8), 8.f));
	Set("PakItem.ExperimentalBorderImage", new FSlateRoundedBoxBrush(FStyleColors::AccentPurple, 8.f));
	Set("PakItem.NewLabelBorderImage", new FSlateRoundedBoxBrush(FStyleColors::AccentGreen.GetSpecifiedColor().CopyWithNewOpacity(0.8), 8.f));
	Set("PakItem.BorderImage", new FSlateRoundedBoxBrush(FStyleColors::Header, 4.0));
	Set("PakItem.ThumbnailBorderImage", new FSlateRoundedBoxBrush(FStyleColors::Panel, 4.0));

	Set("PakItem.Padding", PaddingAmount);

	const float HorizontalTilePadding = 8.0f;
	Set("PakItem.HorizontalTilePadding", HorizontalTilePadding);

	const float VerticalTilePadding = 4.0f;
	Set("PakItem.VerticalTilePadding", VerticalTilePadding);

	const float ThumbnailImageSize = 69.0f;
	Set("PakItem.ThumbnailImageSize", ThumbnailImageSize);

	Set("PakItem.BackgroundBrush", new FSlateNoResource);
	Set("PakItem.BackgroundPadding", FMargin(PaddingAmount));

	FTextBlockStyle NameText = FTextBlockStyle(LargeText)
		.SetColorAndOpacity(FStyleColors::White);
	Set("PakItem.NameText", NameText);

	FTextBlockStyle DescriptionText = FTextBlockStyle(NormalText)
		.SetColorAndOpacity(FStyleColors::Foreground);
	Set("PakItem.DescriptionText", DescriptionText);


	FTextBlockStyle BetaText = FTextBlockStyle(NormalText)
		.SetColorAndOpacity(FStyleColors::White);
	Set("PakItem.BetaText", BetaText);


	FTextBlockStyle VersionNumberText = FTextBlockStyle(LargeText)
		.SetColorAndOpacity(FStyleColors::Foreground);
	Set("PakItem.VersionNumberText", VersionNumberText);

	FTextBlockStyle NewLabelText = FTextBlockStyle(NormalText)
		.SetColorAndOpacity(FLinearColor(0.05f, 0.05f, 0.05f));
	Set("PakItem.NewLabelText", NewLabelText);

	Set("PakItem.BetaWarning", new IMAGE_BRUSH("icon_plugins_betawarn_14px", FVector2D(14, 14)));

	// Metadata editor
	Set("PluginMetadataNameFont", DEFAULT_FONT("Bold", 18));

	// Plugin Creator
	const FButtonStyle& BaseButtonStyle = FStarshipCoreStyle::GetCoreStyle().GetWidgetStyle<FButtonStyle>("Button");
	Set("PluginPath.BrowseButton",
		FButtonStyle(BaseButtonStyle)
		.SetNormal(FSlateRoundedBoxBrush(FStyleColors::Secondary, 4.0f, FStyleColors::Secondary, 2.0f))
		.SetHovered(FSlateRoundedBoxBrush(FStyleColors::Hover, 4.0f, FStyleColors::Hover, 2.0f))
		.SetPressed(FSlateRoundedBoxBrush(FStyleColors::Header, 4.0f, FStyleColors::Header, 2.0f))
		.SetNormalPadding(FMargin(2, 2, 2, 2))
		.SetPressedPadding(FMargin(2, 3, 2, 1)));
}

void FStarshipSodaStyle::FStyle::SetupQuickStartWindowStyle()
{
	
	Set("QuickStart.Icons.Docs", new IMAGE_BRUSH_SVG("QuickStart/docs", Icon64x64));
	Set("QuickStart.Icons.Enter", new IMAGE_BRUSH_SVG("QuickStart/enter", Icon64x64));
	Set("QuickStart.Icons.F2", new IMAGE_BRUSH_SVG("QuickStart/f2", Icon64x64));
	Set("QuickStart.Icons.F3", new IMAGE_BRUSH_SVG("QuickStart/f3", Icon64x64));
	Set("QuickStart.Icons.Git", new IMAGE_BRUSH_SVG("QuickStart/git", Icon64x64));
	Set("QuickStart.Icons.Mouse", new IMAGE_BRUSH_SVG("QuickStart/mouse", Icon64x64));
	Set("QuickStart.Icons.WASD", new IMAGE_BRUSH_SVG("QuickStart/wasd", Icon64x64));
	Set("QuickStart.Icons.YouTube", new IMAGE_BRUSH_SVG("QuickStart/youtube", Icon64x64));
}

#undef DEFAULT_FONT
#undef ICON_FONT
#undef LOCTEXT_NAMESPACE

END_SLATE_FUNCTION_BUILD_OPTIMIZATION
