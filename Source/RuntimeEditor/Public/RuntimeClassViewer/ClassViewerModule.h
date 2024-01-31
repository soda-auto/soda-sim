// Copyright Epic Games, Inc. All Rights Reserved.
// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "AssetRegistry/AssetData.h"
#include "Containers/Array.h"
#include "CoreMinimal.h"
#include "CoreTypes.h"
#include "Delegates/Delegate.h"
#include "Internationalization/Text.h"
#include "Logging/LogMacros.h"
#include "Modules/ModuleInterface.h"
#include "Templates/SharedPointer.h"


class SWidget;
class UClass;

namespace soda
{

class IPropertyHandle;
class FClassViewerFilterFuncs;
class IClassViewerFilter;

DEFINE_LOG_CATEGORY_STATIC(LogEditorClassViewer, Log, All);

/** Delegate used with the Class Viewer in 'class picking' mode.  You'll bind a delegate when the
    class viewer widget is created, which will be fired off when a class is selected in the list */
DECLARE_DELEGATE_OneParam( FOnClassPicked, UClass* );

namespace EClassViewerMode
{
	enum Type
	{
		/** Allows all classes to be browsed and selected; syncs selection with the editor; drag and drop attachment, etc. */
		ClassBrowsing,

		/** Sets the class viewer to operate as a class 'picker'. */
		ClassPicker,
	};
}

namespace EClassViewerDisplayMode
{
	enum Type
	{
		/** Default will choose what view mode based on if in Viewer or Picker mode. */
		DefaultView,

		/** Displays all classes as a tree. */
		TreeView,

		/** Displays all classes as a list. */
		ListView,
	};
}

enum class EClassViewerNameTypeToDisplay : uint8
{
	/** Display both the display name and class name if they're available and different. */
	Dynamic,

	/** Always use the display name */
	DisplayName,

	/** Always use the class name */
	ClassName,
};

/**
 * Settings for the Class Viewer set by the programmer before spawning an instance of the widget.  This
 * is used to modify the class viewer's behavior in various ways, such as filtering in or out specific classes.
 */
PRAGMA_DISABLE_DEPRECATION_WARNINGS
class FClassViewerInitializationOptions
{

public:
	/** [Deprecated] The filter to use on classes in this instance. */
	UE_DEPRECATED(5.0, "Please add to the ClassFilters array member instead.")
	TSharedPtr<IClassViewerFilter> ClassFilter;

	/** The filter(s) to use on classes in this instance. */
	TArray<TSharedRef<IClassViewerFilter>> ClassFilters;

	/** Mode to operate in */
	EClassViewerMode::Type Mode;

	/** Mode to display the classes using. */
	EClassViewerDisplayMode::Type DisplayMode;

	/** Filters so only actors will be displayed. */
	bool bIsActorsOnly;

	/** Filters so only placeable actors will be displayed. Forces bIsActorsOnly to true. */
	bool bIsPlaceableOnly;

	/** Filters so only base blueprints will be displayed. */
	bool bIsBlueprintBaseOnly;

	/** Shows unloaded blueprints. Will not be filtered out based on non-bool filter options. */
	bool bShowUnloadedBlueprints;

	/** Shows a "None" option, only available in Picker mode. */
	bool bShowNoneOption;

	/** true will show the UObject root class. */
	bool bShowObjectRootClass;

	/** If true, root nodes will be expanded by default. */
	bool bExpandRootNodes;

	/** If true, all nodes will be expanded by default. */
	bool bExpandAllNodes;

	/** true allows class dynamic loading on selection */
	bool bEnableClassDynamicLoading;

	/** Controls what name is shown for classes */
	EClassViewerNameTypeToDisplay NameTypeToDisplay;

	/** the title string of the class viewer if required. */
	FText ViewerTitleString;

	/** The property this class viewer be working on. */
	TSharedPtr<IPropertyHandle> PropertyHandle;

	/** The passed in property handle will be used to gather referencing assets. If additional referencing assets should be reported, supply them here. */
	TArray<FAssetData> AdditionalReferencingAssets;

	/** true (the default) shows the view options at the bottom of the class picker */
	bool bAllowViewOptions;

	/** true (the default) shows a background border behind the class viewer widget. */
	bool bShowBackgroundBorder = true;

	/** Defines additional classes you want listed in the "Common Classes" section for the picker. */
	TArray<UClass*> ExtraPickerCommonClasses;

	/** false by default, restricts the class picker to only showing editor module classes */
	bool bEditorClassesOnly;

	/** Will set the initially selected row, if possible, to this class when the viewer is created */
	UClass* InitiallySelectedClass;

	/** (true) Will show the default classes if they exist. */
	bool bShowDefaultClasses;

public:

	/** Constructor */
	FClassViewerInitializationOptions()	
		: Mode( EClassViewerMode::ClassPicker )
		, DisplayMode(EClassViewerDisplayMode::DefaultView)
		, bIsActorsOnly(false)
		, bIsPlaceableOnly(false)
		, bIsBlueprintBaseOnly(false)
		, bShowUnloadedBlueprints(true)
		, bShowNoneOption(false)
		, bShowObjectRootClass(false)
		, bExpandRootNodes(true)
		, bExpandAllNodes(false)
		, bEnableClassDynamicLoading(true)
		, NameTypeToDisplay(EClassViewerNameTypeToDisplay::ClassName)
		, bAllowViewOptions(true)
		, bEditorClassesOnly(false)
		, InitiallySelectedClass(nullptr)
		, bShowDefaultClasses(true)
	{
	}
};
PRAGMA_ENABLE_DEPRECATION_WARNINGS


} // namespace soda