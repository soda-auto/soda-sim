// Copyright Epic Games, Inc. All Rights Reserved.
// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "RuntimePropertyEditor/PropertyEditorDelegates.h"
#include "Framework/Commands/UICommandList.h"
#include "RuntimePropertyEditor/DetailsViewArgs.h"

class AActor;
class FNotifyHook;
class FTabManager;
class FUICommandList;

namespace soda
{
class FPropertyPath;
class IDetailKeyframeHandler;
class IDetailPropertyExtensionHandler;
class IDetailRootObjectCustomization;
class IPropertyTypeIdentifier;
class FDetailsViewObjectFilter;
class FComplexPropertyNode;

typedef TArray<TSharedPtr<FComplexPropertyNode>> FRootPropertyNodeList;

DECLARE_DELEGATE_RetVal_OneParam(bool, FOnValidateDetailsViewPropertyNodes, const FRootPropertyNodeList&)

/**
 * Interface class for all detail views
 */
class IDetailsView : public SCompoundWidget
{
public:
	/** Sets the callback for when the property view changes */
	virtual void SetOnObjectArrayChanged(FOnObjectArrayChanged OnObjectArrayChangedDelegate) = 0;

	/** List of all selected objects we are inspecting */
	virtual const TArray< TWeakObjectPtr<UObject> >& GetSelectedObjects() const = 0;

	/** @return	Returns list of selected actors we are inspecting */
	virtual const TArray< TWeakObjectPtr<AActor> >& GetSelectedActors() const = 0;

	/** @return Returns information about the selected set of actors */
	//virtual const struct FSelectedActorInfo& GetSelectedActorInfo() const = 0;

	/** @return Whether or not the details view is viewing a CDO */
	virtual bool HasClassDefaultObject() const = 0;

	/**
	 * Registers a custom detail layout delegate for a specific class in this instance of the details view only
	 *
	 * @param Class	The class the custom detail layout is for
	 * @param DetailLayoutDelegate	The delegate to call when querying for custom detail layouts for the classes properties
	 */
	virtual void RegisterInstancedCustomPropertyLayout(UStruct* Class, FOnGetDetailCustomizationInstance DetailLayoutDelegate) = 0;
	virtual void RegisterInstancedCustomPropertyTypeLayout(FName PropertyTypeName, FOnGetPropertyTypeCustomizationInstance PropertyTypeLayoutDelegate, TSharedPtr<IPropertyTypeIdentifier> Identifier = nullptr) = 0; 

	/**
	 * Unregisters a custom detail layout delegate for a specific class in this instance of the details view only
	 *
	 * @param Class	The class with the custom detail layout delegate to remove
	 */
	virtual void UnregisterInstancedCustomPropertyLayout(UStruct* Class) = 0;
	virtual void UnregisterInstancedCustomPropertyTypeLayout(FName PropertyTypeName, TSharedPtr<IPropertyTypeIdentifier> Identifier = nullptr) = 0;

	/**
	 * Registers a customization that will be used only if this details panel contains multiple top level objects.
	 * I.E it was created with bAllowMultipleTopLevelObjects = true.	This interface will be used to customize the header for each top level object in the details panel
	 *
	 * @param InRootObjectCustomization	If null is passed in, the customization will be removed
	 */
	virtual void SetRootObjectCustomizationInstance(TSharedPtr<class IDetailRootObjectCustomization> InRootObjectCustomization) = 0;

	/**
	 * Sets the objects this details view is viewing
	 *
	 * @param InObjects		The list of objects to observe
	 * @param bForceRefresh	If true, doesn't check if new objects are being set
	 * @param bOverrideLock	If true, will set the objects even if the details view is locked
	 */
	virtual void SetObjects( const TArray<UObject*>& InObjects, bool bForceRefresh = false, bool bOverrideLock = false ) = 0;
	virtual void SetObjects( const TArray< TWeakObjectPtr< UObject > >& InObjects, bool bForceRefresh = false, bool bOverrideLock = false ) = 0;

	/**
	 * Sets a single object that details view is viewing
	 *
	 * @param InObject		The object to view
	 * @param bForceRefresh	If true, doesn't check if new objects are being set
	 */
	virtual void SetObject( UObject* InObject, bool bForceRefresh = false ) = 0;

	/** Removes all invalid objects being observed by this details panel */
	virtual void RemoveInvalidObjects() = 0;

	/** Set overrides that should be used when looking for packages that contain the given object (used when editing a transient copy of an object, but you need access to th real package) */
	virtual void SetObjectPackageOverrides(const TMap<TWeakObjectPtr<UObject>, TWeakObjectPtr<UPackage>>& InMapping) = 0;

	/**
	 * Returns true if the details view is locked and cant have its observed objects changed 
	 */
	virtual bool IsLocked() const = 0;

	/**
	 * @return true if the details view can be updated from editor selection
	 */
	virtual bool IsUpdatable() const = 0;

	/**
	 * @return True if there is any filter of properties active in this details panel
	 */
	virtual bool HasActiveSearch() const = 0;
	
	/** 
	 * Clears any search terms in the current filter
	 */
	virtual void ClearSearch() = 0;

	/**
	 * @return The number of visible top level objects. This value is affected by filtering. 
	 * Note: this value will always be 1 if this details panel was not created with bAllowMultipleTopLevelObjects=true
	 */
	virtual int32 GetNumVisibleTopLevelObjects() const = 0;

	/** @return The identifier for this details view, or NAME_None is this view is anonymous */
	virtual FName GetIdentifier() const = 0;

	/**
	 * Sets a delegate to call to determine if a specific property should be visible in this instance of the details view
	 */ 
	virtual void SetIsPropertyVisibleDelegate( FIsPropertyVisible InIsPropertyVisible ) = 0;
	virtual FIsPropertyVisible& GetIsPropertyVisibleDelegate() = 0;

	/**
	 * Sets a delegate to call to determine if a specific property should be read-only in this instance of the details view
	 */ 
	virtual void SetIsPropertyReadOnlyDelegate( FIsPropertyReadOnly InIsPropertyReadOnly ) = 0;
	virtual FIsPropertyReadOnly& GetIsPropertyReadOnlyDelegate() = 0;

	/**
	 * Sets a delegate to call to determine if a specific custom row should be visible in this instance of the details view
	 */
	virtual void SetIsCustomRowVisibleDelegate(FIsCustomRowVisible InIsCustomRowVisible) = 0;
	virtual FIsCustomRowVisible& GetIsCustomRowVisibleDelegate() = 0;

	/**
	 * Sets a delegate to call to determine if a specific custom row should be visible in this instance of the details view
	 */
	virtual void SetIsCustomRowReadOnlyDelegate(FIsCustomRowReadOnly InIsCustomRowVisible) = 0;
	virtual FIsCustomRowReadOnly& GetIsCustomRowReadOnlyDelegate() = 0;

	/**
	 * Sets a delegate to call to layout generic details not specific to an object being viewed
	 */ 
	virtual void SetGenericLayoutDetailsDelegate( FOnGetDetailCustomizationInstance OnGetGenericDetails ) = 0;
	virtual FOnGetDetailCustomizationInstance& GetGenericLayoutDetailsDelegate() = 0;

	/**
	 * Sets a delegate to call to determine if the properties  editing is enabled
	 */ 
	virtual void SetIsPropertyEditingEnabledDelegate( FIsPropertyEditingEnabled IsPropertyEditingEnabled ) = 0;
	virtual FIsPropertyEditingEnabled& GetIsPropertyEditingEnabledDelegate() = 0;

	virtual void SetKeyframeHandler( TSharedPtr<IDetailKeyframeHandler> InKeyframeHandler ) = 0;
	virtual TSharedPtr<IDetailKeyframeHandler> GetKeyframeHandler() const = 0;

	virtual void SetExtensionHandler(TSharedPtr<IDetailPropertyExtensionHandler> InExtensionHandler) = 0;
	virtual TSharedPtr<IDetailPropertyExtensionHandler> GetExtensionHandler() const = 0;

	/**
	 * @return true if property editing is enabled (based on the FIsPropertyEditingEnabled delegate)
	 */ 
	virtual bool IsPropertyEditingEnabled() const = 0;

	/**
	 * A delegate which is called after properties have been edited and PostEditChange has been called on all objects.
	 * This can be used to safely make changes to data that the details panel is observing instead of during PostEditChange (which is
	 * unsafe)
	 */
	virtual FOnFinishedChangingProperties& OnFinishedChangingProperties() const = 0;

	/** 
	 * Sets the visible state of the filter box/property grid area
	 */
	virtual void HideFilterArea(bool bIsVisible) = 0;

	/**
	 * Returns a list of all the properties displayed (via full path), order in list corresponds to draw order:
	 */
	virtual TArray< FPropertyPath > GetPropertiesInOrderDisplayed() const = 0;

	/**
	 * Creates a box around the treenode corresponding to Property and scrolls the treenode into view
	 */
	virtual void HighlightProperty(const FPropertyPath& Property) = 0;
	
	/**
	 * Forces all advanced property sections to be in expanded state:
	 */
	virtual void ShowAllAdvancedProperties() = 0;
	
	/**
	 * Refreshes the visibility of root objects in this details view. 
	 * Note: This method has no effect if the details panel is viewing a single top-level object set only
	 */
	virtual void RefreshRootObjectVisibility() = 0;
	/**
	 * Assigns delegate called when view is filtered, useful for updating external control logic:
	 */
	virtual void SetOnDisplayedPropertiesChanged(FOnDisplayedPropertiesChanged InOnDisplayedPropertiesChangedDelegate) = 0;
	virtual FOnDisplayedPropertiesChanged& GetOnDisplayedPropertiesChanged() = 0;

	/**
	 * Disables or enables customization of the details view:
	 */
	virtual void SetDisableCustomDetailLayouts(bool bInDisableCustomDetailLayouts) = 0;

	/**
	 * Sets the set of properties that are considered differing, used when filtering out identical properties
	 */
	virtual void UpdatePropertyAllowList(const TSet<FPropertyPath> InAllowedProperties) = 0;

	/** Returns the name area widget used to display object naming functionality so it can be placed in a custom location.  Note FDetailsViewArgs.bCustomNameAreaLocation must be true */
	virtual TSharedPtr<SWidget> GetNameAreaWidget() = 0;

	/** Optionally add custom tools into the NameArea */
	virtual void SetNameAreaCustomContent(TSharedRef<SWidget>& InCustomContent) = 0;

	/** Returns the search area widget used to display search and view options so it can be placed in a custom location.  Note FDetailsViewArgs.bCustomFilterAreaLocation must be true */
	virtual TSharedPtr<SWidget> GetFilterAreaWidget() = 0;

	/** Returns the command list of the hosting toolkit (can be nullptr if the widget that contains the details panel didn't route a command list in) */
	virtual TSharedPtr<FUICommandList> GetHostCommandList() const = 0;

	/** Returns the tab manager of the hosting toolkit (can be nullptr if the details panel is not hosted within a tab) */
	virtual TSharedPtr<FTabManager> GetHostTabManager() const = 0;

	/** Sets the tab manager of the hosting toolkit (can be nullptr if the details panel is not hosted within a tab) */
	virtual void SetHostTabManager(TSharedPtr<FTabManager> InTabManager) = 0;

	/** Force refresh */
	virtual void ForceRefresh() = 0;

	/** Invalidates any cached state without necessarily doing a complete rebuild. */
	virtual void InvalidateCachedState()
	{
		ForceRefresh();
	}
	
	/** Sets an optional object filter to use for more complex handling of what a details panel is viewing. */
	virtual void SetObjectFilter(TSharedPtr<FDetailsViewObjectFilter> InFilter) = 0;

	/** Sets the custom filter(s) to be used when selecting values for class properties in this view. */
	virtual void SetClassViewerFilters(const TArray<TSharedRef<class IClassViewerFilter>>& InFilters) = 0;

	/** Allows other systems to add a custom filter in the details panel */
	virtual void SetCustomFilterDelegate(FSimpleDelegate InDelegate) = 0;

	virtual void SetCustomFilterLabel(FText InText) = 0;

		/* Use this function to set a callback for SDetailsView that will skip the EnsureDataIsValid call in Tick.
	 * This is useful if your implementation doesn't need to validate nodes every tick or needs to perform some other form of validation. */
	virtual void SetCustomValidatePropertyNodesFunction(FOnValidateDetailsViewPropertyNodes InCustomValidatePropertyNodesFunction) = 0;


};

} // namespace soda