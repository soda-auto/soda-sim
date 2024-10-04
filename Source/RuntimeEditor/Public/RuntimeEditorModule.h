// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SWidget.h"
#include "Widgets/SWindow.h"
#include "Modules/ModuleInterface.h"
#include "UObject/StructOnScope.h"
#include "RuntimePropertyEditor/IDetailsView.h"
#include "RuntimePropertyEditor/PropertyEditorDelegates.h"
#include "RuntimePropertyEditor/IPropertyTypeCustomization.h"
#include "RuntimeStructViewer/StructViewerModule.h"

class FNotifyHook;
class SWindow;

namespace soda
{

class FAssetEditorToolkit;
class IPropertyHandle;
class IPropertyTable;
class IPropertyTableCellPresenter;
class ISinglePropertyView;
class SDetailsView;
class SPropertyTreeViewImpl;
class SSingleProperty;
class IPropertyTypeCustomization;
class IDetailsView;
class IStructureDetailsView;
class IPropertyRowGenerator;
struct FDetailsViewArgs;
struct FSinglePropertyParams;
struct FStructureDetailsViewArgs;
struct FPropertyRowGeneratorArgs;

/**
 * The location of a property name relative to its editor widget                   
 */
namespace EPropertyNamePlacement
{
	enum Type
	{
		/** Do not show the property name */
		Hidden,
		/** Show the property name to the left of the widget */
		Left,
		/** Show the property name to the right of the widget */
		Right,
		/** Inside the property editor edit box (unused for properties that dont have edit boxes ) */
		Inside,
	};
}


/**
 * Potential results from accessing the values of properties                   
 */
namespace FPropertyAccess
{
	enum Result
	{
		/** Multiple values were found so the value could not be read */
		MultipleValues,
		/** Failed to set or get the value */
		Fail,
		/** Successfully set the got the value */
		Success,
	};
}





/**
 * Base class for adding an extra data to identify a custom property type
 */
class IPropertyTypeIdentifier
{
public:
	virtual ~IPropertyTypeIdentifier() {}

	/**
	 * Called to identify if a property type is customized
	 *
	 * @param IPropertyHandle	Handle to the property being tested
	 */
	virtual bool IsPropertyTypeCustomized( const IPropertyHandle& PropertyHandle ) const = 0;
};

typedef TMap< TWeakObjectPtr<UStruct>, FDetailLayoutCallback > FCustomDetailLayoutMap;
typedef TMap< FName, FDetailLayoutCallback > FCustomDetailLayoutNameMap;



/** Struct used to control the visibility of properties in a Structure Detail View */
struct FStructureDetailsViewArgs
{
	FStructureDetailsViewArgs()
		: bShowObjects(false)
		, bShowAssets(true)
		, bShowClasses(true)
		, bShowInterfaces(false)
	{
	}

	/** True if we should show general object properties in the details view */
	bool bShowObjects : 1;

	/** True if we should show asset properties in the details view */
	bool bShowAssets : 1;

	/** True if we should show class properties in the details view */
	bool bShowClasses : 1;

	/** True if we should show interface properties in the details view */
	bool bShowInterfaces : 1;
};

class IDocumentation;

} // namespace soda


class RUNTIMEEDITOR_API FRuntimeEditorModule : public IModuleInterface
{
public:


	//virtual TSharedRef<class FSlateStyleSet> CreateSodaStyleInstance() const;
	
	/**
	 * Called right after the module has been loaded                   
	 */
	virtual void StartupModule();

	/**
	 * Called by the module manager right before this module is unloaded
	 */
	virtual void ShutdownModule();

	/**
	 * Refreshes property windows with a new list of objects to view
	 * 
	 * @param NewObjectList	The list of objects each property window should view
	 */
	//virtual void UpdatePropertyViews( const TArray<UObject*>& NewObjectList );

	/**
	 * Replaces objects being viewed by open property views with new objects
	 *
	 * @param OldToNewObjectMap	A mapping between object to replace and its replacement
	 */
	//virtual void ReplaceViewedObjects( const TMap<UObject*, UObject*>& OldToNewObjectMap );

	/**
	 * Removes deleted objects from property views that are observing them
	 *
	 * @param DeletedObjects	The objects to delete
	 */
	//virtual void RemoveDeletedObjects( TArray<UObject*>& DeletedObjects );

	/**
	 * Returns true if there is an unlocked detail view
	 */
	//virtual bool HasUnlockedDetailViews() const;

	/**
	 * Registers a custom detail layout delegate for a specific class
	 *
	 * @param ClassName	The name of the class that the custom detail layout is for
	 * @param DetailLayoutDelegate	The delegate to call when querying for custom detail layouts for the classes properties
	 */
	virtual void RegisterCustomClassLayout( FName ClassName, soda::FOnGetDetailCustomizationInstance DetailLayoutDelegate );

	/**
	 * Unregisters a custom detail layout delegate for a specific class name
	 *
	 * @param ClassName	The class name with the custom detail layout delegate to remove
	 */
	virtual void UnregisterCustomClassLayout( FName ClassName );


	/**
	 * Registers a property type customization
	 * A property type is a specific FProperty type, a struct, or enum type
	 *
	 * @param PropertyTypeName		The name of the property type to customize.  For structs and enums this is the name of the struct class or enum	(not StructProperty or ByteProperty) 
	 * @param PropertyTypeLayoutDelegate	The delegate to call when querying for a custom layout of the property type
	 * @param Identifier			An identifier to use to differentiate between two customizations on the same type
	 */
	virtual void RegisterCustomPropertyTypeLayout( FName PropertyTypeName, soda::FOnGetPropertyTypeCustomizationInstance PropertyTypeLayoutDelegate, TSharedPtr<soda::IPropertyTypeIdentifier> Identifier = nullptr);

	/**
	 * Unregisters a custom detail layout for a properrty type
	 *
	 * @param PropertyTypeName 	The name of the property type that was registered
	 * @param Identifier 		An identifier to use to differentiate between two customizations on the same type
	 */
	virtual void UnregisterCustomPropertyTypeLayout( FName PropertyTypeName, TSharedPtr<soda::IPropertyTypeIdentifier> InIdentifier = nullptr);

	/**
	 * Customization modules should call this when that module has been unloaded, loaded, etc...
	 * so the property module can clean up its data.  Needed to support dynamic reloading of modules
	 */
	virtual void NotifyCustomizationModuleChanged();

	/**
	 * Creates a new detail view
	 *
 	 * @param DetailsViewArgs		The struct containing all the user definable details view arguments
	 * @return The new detail view
	 */
	virtual TSharedRef<soda::IDetailsView> CreateDetailView( const soda::FDetailsViewArgs& DetailsViewArgs );

	/**
	 * Find an existing detail view
	 *
 	 * @param ViewIdentifier	The name of the details view to find
	 * @return The existing detail view, or null if it wasn't found
	 */
	//virtual TSharedPtr<class soda::IDetailsView> FindDetailView( const FName ViewIdentifier ) const;

	/**
	 *  Convenience method for creating a new floating details window (a details view with its own top level window)
	 *
	 * @param InObjects			The objects to create the detail view for.
	 * @param bIsLockable		True if the property view can be locked.
	 * @return The new details view window.
	 */
	virtual TSharedRef<SWindow> CreateFloatingDetailsView( const TArray< UObject* >& InObjects, bool bIsLockable );

	/**
	 * Creates a standalone widget for a single property
	 *
	 * @param InObject			The object to view
	 * @param InPropertyName	The name of the property to display
	 * @param InitParams		Optional init params for a single property
	 * @return The new property if valid or null
	 */
	virtual TSharedPtr<soda::ISinglePropertyView> CreateSingleProperty( UObject* InObject, FName InPropertyName, const soda::FSinglePropertyParams& InitParams );

	virtual TSharedRef<soda::IStructureDetailsView> CreateStructureDetailView(const soda::FDetailsViewArgs& DetailsViewArgs, const soda::FStructureDetailsViewArgs& StructureDetailsViewArgs, TSharedPtr<class FStructOnScope> StructData, const FText& CustomName = FText::GetEmpty());

	virtual TSharedRef<soda::IPropertyRowGenerator> CreatePropertyRowGenerator(const soda::FPropertyRowGeneratorArgs& InArgs);

	/**
	 * Creates a property change listener that notifies users via a  delegate when a property on an object changes
	 *
	 * @return The new property change listener
	 */
	//virtual TSharedRef<class IPropertyChangeListener> CreatePropertyChangeListener();

	//virtual TSharedRef< class soda::IPropertyTable > CreatePropertyTable();

	//virtual TSharedRef< SWidget > CreatePropertyTableWidget( const TSharedRef< class soda::IPropertyTable >& PropertyTable );

	//virtual TSharedRef< SWidget > CreatePropertyTableWidget( const TSharedRef< class soda::IPropertyTable >& PropertyTable, const TArray< TSharedRef< class soda::IPropertyTableCustomColumn > >& Customizations );
	//virtual TSharedRef< class soda::IPropertyTableWidgetHandle > CreatePropertyTableWidgetHandle( const TSharedRef< soda::IPropertyTable >& PropertyTable );
	//virtual TSharedRef< class soda::IPropertyTableWidgetHandle > CreatePropertyTableWidgetHandle( const TSharedRef< soda::IPropertyTable >& PropertyTable, const TArray< TSharedRef< class soda::IPropertyTableCustomColumn > >& Customizations );

	//virtual TSharedRef< soda::IPropertyTableCellPresenter > CreateTextPropertyCellPresenter( const TSharedRef< class soda::FPropertyNode >& InPropertyNode, const TSharedRef< class soda::IPropertyTableUtilities >& InPropertyUtilities,const FSlateFontInfo* InFontPtr = NULL);

	/**
	 * Register a floating struct on scope so that the details panel may use it as a property
	 *
	 * @param StructOnScope		The struct to register
	 * @return The struct property that may may be associated with the details panel
 	 */
	virtual FStructProperty* RegisterStructOnScopeProperty(TSharedRef<FStructOnScope> StructOnScope);

	/**
	 *
	 */


	
	soda::FPropertyTypeLayoutCallback FindPropertyTypeLayoutCallback(FName PropertyTypeName, const soda::IPropertyHandle& PropertyHandle, const soda::FCustomPropertyTypeLayoutMap& InstancedPropertyTypeLayoutMapp);


	//DECLARE_EVENT(PropertyEditorModule, FPropertyEditorOpenedEvent);
	//virtual FPropertyEditorOpenedEvent& OnPropertyEditorOpened() { return PropertyEditorOpened; }

	//virtual TSharedRef< soda::FAssetEditorToolkit > CreatePropertyEditorToolkit(const EToolkitMode::Type Mode, const TSharedPtr< class IToolkitHost >& InitToolkitHost, UObject* ObjectToEdit);
	//virtual TSharedRef< soda::FAssetEditorToolkit > CreatePropertyEditorToolkit(const EToolkitMode::Type Mode, const TSharedPtr< IToolkitHost >& InitToolkitHost, const TArray< UObject* >& ObjectsToEdit);
	//virtual TSharedRef< soda::FAssetEditorToolkit > CreatePropertyEditorToolkit(const EToolkitMode::Type Mode, const TSharedPtr< IToolkitHost >& InitToolkitHost, const TArray< TWeakObjectPtr< UObject > >& ObjectsToEdit);

	const soda::FCustomDetailLayoutNameMap& GetClassNameToDetailLayoutNameMap() const { return ClassNameToDetailLayoutNameMap; }
	soda::FPropertyTypeLayoutCallback GetPropertyTypeCustomization(const FProperty* InProperty, const soda::IPropertyHandle& PropertyHandle, const soda::FCustomPropertyTypeLayoutMap& InstancedPropertyTypeLayoutMap);

	/** Get the global row extension generators. */
	soda::FOnGenerateGlobalRowExtension& GetGlobalRowExtensionDelegate() { return OnGenerateGlobalRowExtension; }

	bool IsCustomizedStruct(const UStruct* Struct, const soda::FCustomPropertyTypeLayoutMap& InstancePropertyTypeLayoutMap) const;


	class TSharedRef< soda::IDocumentation > GetDocumentation() const;

	soda::FStructViewerModule& GetStructViewer() { return StructViewerModule; }

private:

	/**
	 * Creates and returns a property view widget for embedding property views in other widgets
	 * NOTE: At this time these MUST not be referenced by the caller of CreatePropertyView when the property module unloads
	 * 
	 * @param	InObject						The UObject that the property view should observe(Optional)
	 * @param	bAllowFavorites					Whether the property view should save favorites
	 * @param	bIsLockable						Whether or not the property view is lockable
	 * @param	bAllowSearch					Whether or not the property window allows searching it
	 * @param	InNotifyHook					Notify hook to call on some property change events
	 * @param	ColumnWidth						The width of the name column
	 * @param	OnPropertySelectionChanged		Delegate for notifying when the property selection has changed.
	 * @return	The newly created SPropertyTreeViewImpl widget
	 */
	//virtual TSharedRef<soda::SPropertyTreeViewImpl> CreatePropertyView( UObject* InObject, bool bAllowFavorites, bool bIsLockable, bool bHiddenPropertyVisibility, bool bAllowSearch, bool ShowTopLevelNodes, FNotifyHook* InNotifyHook, float InNameColumnWidth, soda::FOnPropertySelectionChanged OnPropertySelectionChanged, soda::FOnPropertyClicked OnPropertyMiddleClicked, soda::FConstructExternalColumnHeaders ConstructExternalColumnHeaders, soda::FConstructExternalColumnCell soda::ConstructExternalColumnCell );

	//TSharedPtr<FAssetThumbnailPool> GetThumbnailPool();

	void RegisterObjectCustomizations();
	void RegisterPropertyTypeCustomizations();

private:
	/** All created detail views */
	TArray< TWeakPtr<soda::SDetailsView> > AllDetailViews;
	/** All created single property views */
	TArray< TWeakPtr<soda::SSingleProperty> > AllSinglePropertyViews;
	/** A mapping of class names to detail layout delegates, called when querying for custom detail layouts */
	soda::FCustomDetailLayoutNameMap ClassNameToDetailLayoutNameMap;
	/** A mapping of property names to property type layout delegates, called when querying for custom property layouts */
	soda::FCustomPropertyTypeLayoutMap GlobalPropertyTypeToLayoutMap;
	/** Event to be called when a property editor is opened */
	//soda::FPropertyEditorOpenedEvent PropertyEditorOpened;
	/** Mapping of registered floating UStructs to their struct proxy so they show correctly in the details panel */
	TMap<FName, FStructProperty*> RegisteredStructToProxyMap;
	/** Shared thumbnail pool used by property row generators */
	//TSharedPtr<class FAssetThumbnailPool> GlobalThumbnailPool;
	/** Container for ScopeOnStruct FStructProperty objects */
	UStruct* StructOnScopePropertyOwner;
	/** Delegate called to extend the name column widget on a property row. */
	soda::FOnGenerateGlobalRowExtension OnGenerateGlobalRowExtension;


	bool bUsingStarshipStyle = false;

	TSharedPtr< soda::IDocumentation > Documentation;

	soda::FStructViewerModule StructViewerModule;
};
