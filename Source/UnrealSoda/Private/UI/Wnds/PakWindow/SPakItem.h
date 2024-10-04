// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Widgets/SCompoundWidget.h"
#include "SodaPak.h"

enum class ECheckBoxState : uint8;
struct FSlateDynamicImageBrush;

class SPakItemList;


/**
 * Widget that represents a "tile" for a single plugin in our plugins list
 */
class SPakItem : public SCompoundWidget
{
	SLATE_BEGIN_ARGS( SPakItem )
	{
	}

	SLATE_END_ARGS()


	/** Widget constructor */
	void Construct( const FArguments& Args, const TSharedRef< class SPakItemList > Owner, TSharedRef<FSodaPak> SodaPak);

private:

	/** Returns text to display for the plugin name. */
	FText GetPakNameText() const;

	/** Updates the contents of this tile */
	void RecreateWidgets();

	/** Returns the checked state for the enabled checkbox */
	ECheckBoxState IsPakInstalled() const;

	/** Called when the enabled checkbox is clicked */
	void OnEnablePluginCheckboxChanged(ECheckBoxState NewCheckedState);

	/** Used to determine whether to show the edit and package buttons for this plugin */
	EVisibility GetAuthoringButtonsVisibility() const;



private:

	/** The item we're representing the in tree */
	TSharedPtr<FSodaPak> SodaPak;

	/** Weak pointer back to its owner */
	TWeakPtr< SPakItemList > OwnerWeak;

	/** Brush resource for the image that is dynamically loaded */
	TSharedPtr< FSlateDynamicImageBrush > PluginIconDynamicImageBrush;
};

