// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "STransformViewportToolbar.h"
#include "EngineDefines.h"
#include "Modules/ModuleManager.h"
#include "Widgets/SBoxPanel.h"
#include "Styling/SlateTypes.h"
#include "Framework/Commands/UIAction.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SUniformGridPanel.h"
#include "Widgets/Layout/SSpacer.h"
#include "SodaStyleSet.h"
#include "Soda/UI/SodaViewportCommands.h"
#include "Widgets/Input/SNumericEntryBox.h"
#include "Styling/ToolBarStyle.h"
#include "UI/Toolbar/Common/SViewportToolBarMenu.h"
#include "UI/Toolbar/Common/SViewportToolBarButton.h"
#include "Soda/UI/SSodaViewport.h"
#include "Soda/SodaGameViewportClient.h"
#include "Soda/SodaGameMode.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"

#define LOCTEXT_NAMESPACE "TransformToolBar"

namespace soda
{

void STransformViewportToolBar::Construct( const FArguments& InArgs )
{
	Viewport = InArgs._Viewport;
	CommandList = InArgs._CommandList;

	ChildSlot
	[
		MakeTransformToolBar(InArgs._Extenders)
	];

	SViewportToolBar::Construct(SViewportToolBar::FArguments());
}

TSharedRef< SWidget > STransformViewportToolBar::MakeTransformToolBar( const TSharedPtr< FExtender > InExtenders )
{
	FSlimHorizontalToolBarBuilder ToolbarBuilder( CommandList, FMultiBoxCustomization::None, InExtenders );

	// Use a custom style
	FName ToolBarStyle = "EditorViewportToolBar";
	ToolbarBuilder.SetStyle(&FSodaStyle::Get(), ToolBarStyle);
	ToolbarBuilder.SetLabelVisibility(EVisibility::Collapsed);

	// Transform controls cannot be focusable as it fights with the press space to change transform mode feature
	ToolbarBuilder.SetIsFocusable( false );

	ToolbarBuilder.BeginSection("Transform");
	{
		ToolbarBuilder.BeginBlockGroup();

		//Select Mode
		static FName SelectModeName = FName(TEXT("SelectMode"));
		ToolbarBuilder.AddToolBarButton(FSodalViewportCommands::Get().SelectMode, NAME_None, TAttribute<FText>(), TAttribute<FText>(), 
			FSlateIcon(FSodaStyle::GetStyleSetName(), "SodaViewport.SelectMode"), SelectModeName);

		// Translate Mode
		static FName TranslateModeName = FName(TEXT("TranslateMode"));
		ToolbarBuilder.AddToolBarButton( FSodalViewportCommands::Get().TranslateMode, NAME_None, TAttribute<FText>(), TAttribute<FText>(), 
			FSlateIcon(FSodaStyle::GetStyleSetName(), "SodaViewport.TranslateMode"), TranslateModeName);

		// Rotate Mode
		static FName RotateModeName = FName(TEXT("RotateMode"));
		ToolbarBuilder.AddToolBarButton( FSodalViewportCommands::Get().RotateMode, NAME_None, TAttribute<FText>(), TAttribute<FText>(), 
			FSlateIcon(FSodaStyle::GetStyleSetName(), "SodaViewport.RotateMode"), RotateModeName );

		// Scale Mode
		static FName ScaleModeName = FName(TEXT("ScaleMode"));
		ToolbarBuilder.AddToolBarButton( FSodalViewportCommands::Get().ScaleMode, NAME_None, TAttribute<FText>(), TAttribute<FText>(), 
			FSlateIcon(FSodaStyle::GetStyleSetName(), "SodaViewport.ScaleMode"), ScaleModeName );


		ToolbarBuilder.EndBlockGroup();
		ToolbarBuilder.AddSeparator();

		ToolbarBuilder.SetIsFocusable( false );

		ToolbarBuilder.AddToolBarButton( FSodalViewportCommands::Get().CycleTransformGizmoCoordSystem,
			NAME_None,
			TAttribute<FText>(),
			TAttribute<FText>(),
			TAttribute<FSlateIcon>(this, &STransformViewportToolBar::GetLocalToWorldIcon),
			FName(TEXT("CycleTransformGizmoCoordSystem")),

			// explictly specify what this widget should look like as a menu item
			FNewMenuDelegate::CreateLambda( []( FMenuBuilder& InMenuBuilder )
			{
				InMenuBuilder.AddMenuEntry(FSodalViewportCommands::Get().RelativeCoordinateSystem_World);
				InMenuBuilder.AddMenuEntry(FSodalViewportCommands::Get().RelativeCoordinateSystem_Local);
			}
		));
		
	}

	ToolbarBuilder.EndSection();

	return ToolbarBuilder.MakeWidget();
}

FReply STransformViewportToolBar::OnCycleCoordinateSystem()
{
	if( Viewport.IsValid() )
	{
		Viewport.Pin()->OnCycleCoordinateSystem();
	}
	
	return FReply::Handled();
}

FSlateIcon STransformViewportToolBar::GetLocalToWorldIcon() const
{
	if( Viewport.IsValid() && Viewport.Pin()->IsCoordSystemActive(soda::COORD_World) )
	{
		static FName WorldIcon("SodaViewport.RelativeCoordinateSystem_World");
		return FSlateIcon(FSodaStyle::GetStyleSetName(), WorldIcon);
	}
	
	static FName LocalIcon("Icons.Transform");
	return FSlateIcon(FSodaStyle::GetStyleSetName(), LocalIcon);
}

} // namespace soda

#undef LOCTEXT_NAMESPACE 
