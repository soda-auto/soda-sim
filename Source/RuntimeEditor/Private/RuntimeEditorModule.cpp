// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "RuntimeEditorModule.h"
#include "UObject/UnrealType.h"
#include "Widgets/Layout/SBorder.h"
#include "Modules/ModuleManager.h"
#include "Framework/Application/SlateApplication.h"
#include "Engine/UserDefinedEnum.h"
#include "Engine/UserDefinedStruct.h"
#include "RuntimePropertyEditor/Presentation/PropertyEditor/PropertyEditor.h"
#include "RuntimePropertyEditor/SSingleProperty.h"
#include "RuntimePropertyEditor/IDetailsView.h"
#include "RuntimePropertyEditor/SDetailsView.h"
#include "RuntimePropertyEditor/IPropertyTableWidgetHandle.h"
#include "RuntimePropertyEditor/IPropertyTable.h"
//#include "UserInterface/PropertyTable/SPropertyTable.h"
//#include "UserInterface/PropertyTable/PropertyTableWidgetHandle.h"
//#include "IAssetTools.h"
//#include "AssetToolsModule.h"
//#include "SPropertyTreeViewImpl.h"
//#include "Interfaces/IMainFrameModule.h"
#include "RuntimePropertyEditor/IPropertyChangeListener.h"
//#include "PropertyChangeListener.h"
//#include "Toolkits/AssetEditorToolkit.h"
//#include "PropertyEditorToolkit.h"

#include "RuntimePropertyEditor/Presentation/PropertyTable/PropertyTable.h"
#include "RuntimePropertyEditor/IPropertyTableCellPresenter.h"
//#include "UserInterface/PropertyTable/TextPropertyTableCellPresenter.h"

#include "RuntimePropertyEditor/SStructureDetailsView.h"
//#include "Widgets/Colors/SColorPicker.h"
#include "RuntimePropertyEditor/PropertyRowGenerator.h"

#include "RuntimeMetaData.h"

#include "SodaDetailCustomizations/ActorDetails.h"
#include "SodaDetailCustomizations/ActorComponentDetails.h"
#include "SodaDetailCustomizations/SceneComponentDetails.h"
#include "SodaDetailCustomizations/KeyStructCustomization.h"
#include "SodaDetailCustomizations/ComponentReferenceCustomization.h"
#include "SodaDetailCustomizations/ObjectDetails.h"
#include "SodaDetailCustomizations/SoftObjectPathCustomization.h"


// For Documentation
#include "Containers/UnrealString.h"
#include "Delegates/Delegate.h"
#include "RuntimeDocumentation/Documentation.h"
#include "Framework/Commands/UICommandInfo.h"
#include "Framework/MultiBox/MultiBoxDefs.h"
#include "HAL/Platform.h"
#include "RuntimeDocumentation/IDocumentation.h"
#include "Internationalization/Text.h"
#include "Misc/Attribute.h"
#include "Modules/ModuleManager.h"
#include "Templates/SharedPointer.h"
#include "UObject/NameTypes.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SNullWidget.h"
#include "Widgets/SToolTip.h"

//For ClassViewer
#include "RuntimeClassViewer/SClassViewer.h"

namespace soda
{

TSharedRef<IPropertyTypeCustomization> FPropertyTypeLayoutCallback::GetCustomizationInstance() const
{
	return PropertyTypeLayoutDelegate.Execute();
}

void FPropertyTypeLayoutCallbackList::Add( const FPropertyTypeLayoutCallback& NewCallback )
{
	if( !NewCallback.PropertyTypeIdentifier.IsValid() )
	{
		BaseCallback = NewCallback;
	}
	else
	{
		IdentifierList.Add( NewCallback );
	}
}

void FPropertyTypeLayoutCallbackList::Remove( const TSharedPtr<IPropertyTypeIdentifier>& InIdentifier )
{
	if( !InIdentifier.IsValid() )
	{
		BaseCallback = FPropertyTypeLayoutCallback();
	}
	else
	{
		IdentifierList.RemoveAllSwap( [&InIdentifier]( FPropertyTypeLayoutCallback& Callback) { return Callback.PropertyTypeIdentifier == InIdentifier; } );
	}
}

const FPropertyTypeLayoutCallback& FPropertyTypeLayoutCallbackList::Find( const IPropertyHandle& PropertyHandle ) const 
{
	if( IdentifierList.Num() > 0 )
	{
		const FPropertyTypeLayoutCallback* Callback =
			IdentifierList.FindByPredicate
			(
				[&]( const FPropertyTypeLayoutCallback& InCallback )
				{
					return InCallback.PropertyTypeIdentifier->IsPropertyTypeCustomized( PropertyHandle );
				}
			);

		if( Callback )
		{
			return *Callback;
		}
	}

	return BaseCallback;
}

}

bool FRuntimeEditorModule::IsCustomizedStruct(const UStruct* Struct, const soda::FCustomPropertyTypeLayoutMap& InstancePropertyTypeLayoutMap) const
{
	bool bFound = false;
	if (Struct && !Struct->IsA<UUserDefinedStruct>())
	{
		bFound = InstancePropertyTypeLayoutMap.Contains(Struct->GetFName());
		if (!bFound)
		{
			bFound = GlobalPropertyTypeToLayoutMap.Contains(Struct->GetFName());
		}
	}

	return bFound;
}


soda::FPropertyTypeLayoutCallback FRuntimeEditorModule::GetPropertyTypeCustomization(const FProperty* Property, const soda::IPropertyHandle& PropertyHandle, const soda::FCustomPropertyTypeLayoutMap& InstancedPropertyTypeLayoutMap)
{
	if( Property )
	{
		const FStructProperty* StructProperty = CastField<FStructProperty>(Property);
		bool bStructProperty = StructProperty && StructProperty->Struct;
		const bool bUserDefinedStruct = bStructProperty && StructProperty->Struct->IsA<UUserDefinedStruct>();
		bStructProperty &= !bUserDefinedStruct;

		const UEnum* Enum = nullptr;

		if (const FByteProperty* ByteProperty = CastField<FByteProperty>(Property))
		{
			Enum = ByteProperty->Enum;
		}
		else if (const FEnumProperty* EnumProperty = CastField<FEnumProperty>(Property))
		{
			Enum = EnumProperty->GetEnum();
		}

		if (Enum && Enum->IsA<UUserDefinedEnum>())
		{
			Enum = nullptr;
		}

		const FObjectProperty* ObjectProperty = CastField<FObjectProperty>(Property);
		const bool bObjectProperty = ObjectProperty != NULL && ObjectProperty->PropertyClass != NULL;

		FName PropertyTypeName;
		if( bStructProperty )
		{
			PropertyTypeName = StructProperty->Struct->GetFName();
		}
		else if( Enum )
		{
			PropertyTypeName = Enum->GetFName();
		}
		else if ( bObjectProperty )
		{
			UClass* PropertyClass = ObjectProperty->PropertyClass;
			while (PropertyClass)
			{
				const soda::FPropertyTypeLayoutCallback& Callback = FindPropertyTypeLayoutCallback(PropertyClass->GetFName(), PropertyHandle, InstancedPropertyTypeLayoutMap);
				if (Callback.IsValid())
				{
					return Callback;
				}

				PropertyClass = PropertyClass->GetSuperClass();
			}
		}
		else
		{
			PropertyTypeName = Property->GetClass()->GetFName();
		}

		return FindPropertyTypeLayoutCallback(PropertyTypeName, PropertyHandle, InstancedPropertyTypeLayoutMap);
	}

	return soda::FPropertyTypeLayoutCallback();
}

soda::FPropertyTypeLayoutCallback FRuntimeEditorModule::FindPropertyTypeLayoutCallback(FName PropertyTypeName, const soda::IPropertyHandle& PropertyHandle, const soda::FCustomPropertyTypeLayoutMap& InstancedPropertyTypeLayoutMap)
{
	if (PropertyTypeName != NAME_None)
	{
		const soda::FPropertyTypeLayoutCallbackList* LayoutCallbacks = InstancedPropertyTypeLayoutMap.Find(PropertyTypeName);

		if (!LayoutCallbacks)
		{
			LayoutCallbacks = GlobalPropertyTypeToLayoutMap.Find(PropertyTypeName);
		}

		if (LayoutCallbacks)
		{
			const soda::FPropertyTypeLayoutCallback& Callback = LayoutCallbacks->Find(PropertyHandle);
			return Callback;
		}
	}

	return soda::FPropertyTypeLayoutCallback();
}


/*
TSharedRef< FAssetEditorToolkit > FRuntimeEditorModule::CreatePropertyEditorToolkit(const EToolkitMode::Type Mode, const TSharedPtr< class IToolkitHost >& InitToolkitHost, UObject* ObjectToEdit)
{
	return FPropertyEditorToolkit::CreateEditor(Mode, InitToolkitHost, ObjectToEdit);
}


TSharedRef< FAssetEditorToolkit > FRuntimeEditorModule::CreatePropertyEditorToolkit(const EToolkitMode::Type Mode, const TSharedPtr< IToolkitHost >& InitToolkitHost, const TArray< UObject* >& ObjectsToEdit)
{
	return FPropertyEditorToolkit::CreateEditor(Mode, InitToolkitHost, ObjectsToEdit);
}

TSharedRef< FAssetEditorToolkit > FRuntimeEditorModule::CreatePropertyEditorToolkit(const EToolkitMode::Type Mode, const TSharedPtr< IToolkitHost >& InitToolkitHost, const TArray< TWeakObjectPtr< UObject > >& ObjectsToEdit)
{
	TArray< UObject* > RawObjectsToEdit;
	for (auto ObjectIter = ObjectsToEdit.CreateConstIterator(); ObjectIter; ++ObjectIter)
	{
		RawObjectsToEdit.Add(ObjectIter->Get());
	}

	return FPropertyEditorToolkit::CreateEditor(Mode, InitToolkitHost, RawObjectsToEdit);
}
*/

/*
TSharedRef<soda::FSlateStyleSet> FRuntimeEditorModule::CreateSodaStyleInstance() const
{
	return FSlateSodaStyle::Create(FSlateSodaStyle::Settings);
}
*/

void FRuntimeEditorModule::StartupModule()
{
	//StructOnScopePropertyOwner = nullptr;

	RegisterObjectCustomizations();
	RegisterPropertyTypeCustomizations();

	Documentation = soda::FDocumentation::Create();
	FMultiBoxSettings::ToolTipConstructor = FMultiBoxSettings::FConstructToolTip::CreateLambda([this](const TAttribute<FText>& ToolTipText, const TSharedPtr<SWidget>& OverrideContent, const TSharedPtr<const FUICommandInfo>& Action) 
	{
		if ( Action.IsValid() )
		{
			return Documentation->CreateToolTip( ToolTipText, OverrideContent, FString( TEXT("Shared/") ) + Action->GetBindingContext().ToString(), Action->GetCommandName().ToString() );
		}

		TSharedPtr< SWidget > ToolTipContent;
        if ( OverrideContent.IsValid() )
        {
            ToolTipContent = OverrideContent;
        }
        else
        {
            ToolTipContent = SNullWidget::NullWidget;
        }
        
		return SNew( SToolTip )
			   .Text( ToolTipText )
			   [
					ToolTipContent.ToSharedRef()
			   ];
	});
}

void FRuntimeEditorModule::ShutdownModule()
{
	// No need to remove this object from root since the final GC pass doesn't care about root flags
	//StructOnScopePropertyOwner = nullptr;

	// NOTE: It's vital that we clean up everything created by this DLL here!  We need to make sure there
	//       are no outstanding references to objects as the compiled code for this module's class will
	//       literally be unloaded from memory after this function exits.  This even includes instantiated
	//       templates, such as delegate wrapper objects that are allocated by the module!
	//DestroyColorPicker();

	AllDetailViews.Empty();
	AllSinglePropertyViews.Empty();

	soda::SClassViewer::DestroyClassHierarchy();
}

TSharedRef<SWindow> FRuntimeEditorModule::CreateFloatingDetailsView(const TArray< UObject* >& InObjects, bool bIsLockable)
{

	TSharedRef<SWindow> NewSlateWindow = SNew(SWindow)
		.Title(NSLOCTEXT("PropertyEditor", "WindowTitle", "Property Editor"))
		.ClientSize(FVector2D(400, 550));

	// If the main frame exists parent the window to it
	TSharedPtr< SWindow > ParentWindow;

	/*
	if (FModuleManager::Get().IsModuleLoaded("MainFrame"))
	{
		IMainFrameModule& MainFrame = FModuleManager::GetModuleChecked<IMainFrameModule>("MainFrame");
		ParentWindow = MainFrame.GetParentWindow();
	}
	*/

	if (ParentWindow.IsValid())
	{
		// Parent the window to the main frame 
		FSlateApplication::Get().AddWindowAsNativeChild(NewSlateWindow, ParentWindow.ToSharedRef());
	}
	else
	{
		FSlateApplication::Get().AddWindow(NewSlateWindow);
	}

	soda::FDetailsViewArgs Args;
	Args.bHideSelectionTip = true;
	Args.bLockable = bIsLockable;

	TSharedRef<soda::IDetailsView> DetailView = CreateDetailView(Args);

	bool bHaveTemplate = false;
	for (int32 i = 0; i < InObjects.Num(); i++)
	{
		if (InObjects[i] != NULL && InObjects[i]->IsTemplate())
		{
			bHaveTemplate = true;
			break;
		}
	}

	//DetailView->SetIsPropertyVisibleDelegate(FIsPropertyVisible::CreateStatic(&ShouldShowProperty, bHaveTemplate));

	DetailView->SetObjects(InObjects);

	NewSlateWindow->SetContent(
		SNew(SBorder)
		.BorderImage(FSodaStyle::GetBrush(TEXT("PropertyWindow.WindowBorder")))
		[
			DetailView
		]
	);

	return NewSlateWindow;
}



void FRuntimeEditorModule::NotifyCustomizationModuleChanged()
{
	if (FSlateApplication::IsInitialized())
	{
		// The module was changed (loaded or unloaded), force a refresh.  Note it is assumed the module unregisters all customization delegates before this
		for (int32 ViewIndex = 0; ViewIndex < AllDetailViews.Num(); ++ViewIndex)
		{
			if (AllDetailViews[ViewIndex].IsValid())
			{
				TSharedPtr<soda::SDetailsView> DetailViewPin = AllDetailViews[ViewIndex].Pin();

				DetailViewPin->ForceRefresh();
			}
		}
	}
}


/*
static bool ShouldShowProperty(const FPropertyAndParent& PropertyAndParent, bool bHaveTemplate)
{
	const FProperty& Property = PropertyAndParent.Property;

	if ( bHaveTemplate )
	{
		const UClass* PropertyOwnerClass = Property.GetOwner<const UClass>();
		const bool bDisableEditOnTemplate = PropertyOwnerClass 
			&& PropertyOwnerClass->IsNative()
			&& Property.HasAnyPropertyFlags(CPF_DisableEditOnTemplate);
		if(bDisableEditOnTemplate)
		{
			return false;
		}
	}
	return true;
}
*/

/*
TSharedRef<SPropertyTreeViewImpl> FRuntimeEditorModule::CreatePropertyView( 
	UObject* InObject,
	bool bAllowFavorites, 
	bool bIsLockable, 
	bool bHiddenPropertyVisibility, 
	bool bAllowSearch, 
	bool ShowTopLevelNodes,
	FNotifyHook* InNotifyHook, 
	float InNameColumnWidth,
	FOnPropertySelectionChanged OnPropertySelectionChanged,
	FOnPropertyClicked OnPropertyMiddleClicked,
	FConstructExternalColumnHeaders ConstructExternalColumnHeaders,
	FConstructExternalColumnCell ConstructExternalColumnCell)
{
	TSharedRef<SPropertyTreeViewImpl> PropertyView = 
		SNew( SPropertyTreeViewImpl )
		.IsLockable( bIsLockable )
		.AllowFavorites( bAllowFavorites )
		.HiddenPropertyVis( bHiddenPropertyVisibility )
		.NotifyHook( InNotifyHook )
		.AllowSearch( bAllowSearch )
		.ShowTopLevelNodes( ShowTopLevelNodes )
		.NameColumnWidth( InNameColumnWidth )
		.OnPropertySelectionChanged( OnPropertySelectionChanged )
		.OnPropertyMiddleClicked( OnPropertyMiddleClicked )
		.ConstructExternalColumnHeaders( ConstructExternalColumnHeaders )
		.ConstructExternalColumnCell( ConstructExternalColumnCell );

	if( InObject )
	{
		TArray<UObject*> Objects;
		Objects.Add( InObject );
		PropertyView->SetObjectArray( Objects );
	}

	return PropertyView;
}
*/

/*
TSharedPtr<FAssetThumbnailPool> FRuntimeEditorModule::GetThumbnailPool()
{
	if (!GlobalThumbnailPool.IsValid())
	{
		// Create a thumbnail pool for the view if it doesn't exist.  This does not use resources of no thumbnails are used
		GlobalThumbnailPool = MakeShareable(new FAssetThumbnailPool(50, true));
	}

	return GlobalThumbnailPool;
}
*/

TSharedRef<soda::IDetailsView> FRuntimeEditorModule::CreateDetailView( const soda::FDetailsViewArgs& DetailsViewArgs )
{
	// Compact the list of detail view instances
	for( int32 ViewIndex = 0; ViewIndex < AllDetailViews.Num(); ++ViewIndex )
	{
		if ( !AllDetailViews[ViewIndex].IsValid() )
		{
			AllDetailViews.RemoveAtSwap( ViewIndex );
			--ViewIndex;
		}
	}

	TSharedRef<soda::SDetailsView> DetailView = SNew(soda::SDetailsView, DetailsViewArgs);

	AllDetailViews.Add( DetailView );

	//PropertyEditorOpened.Broadcast();
	return DetailView;
}

/*
TSharedPtr<IDetailsView> FRuntimeEditorModule::FindDetailView( const FName ViewIdentifier ) const
{
	if(ViewIdentifier.IsNone())
	{
		return nullptr;
	}

	for(auto It = AllDetailViews.CreateConstIterator(); It; ++It)
	{
		TSharedPtr<SDetailsView> DetailsView = It->Pin();
		if(DetailsView.IsValid() && DetailsView->GetIdentifier() == ViewIdentifier)
		{
			return DetailsView;
		}
	}

	return nullptr;
}
*/


TSharedPtr<soda::ISinglePropertyView> FRuntimeEditorModule::CreateSingleProperty( UObject* InObject, FName InPropertyName, const soda::FSinglePropertyParams& InitParams )
{
	// Compact the list of detail view instances
	for( int32 ViewIndex = 0; ViewIndex < AllSinglePropertyViews.Num(); ++ViewIndex )
	{
		if ( !AllSinglePropertyViews[ViewIndex].IsValid() )
		{
			AllSinglePropertyViews.RemoveAtSwap( ViewIndex );
			--ViewIndex;
		}
	}

	TSharedRef<soda::SSingleProperty> Property =
		SNew(soda::SSingleProperty )
		.Object( InObject )
		.PropertyName( InPropertyName )
		.NamePlacement( InitParams.NamePlacement )
		.NameOverride( InitParams.NameOverride )
		.NotifyHook( InitParams.NotifyHook )
		.PropertyFont( InitParams.Font );

	if( Property->HasValidProperty() )
	{
		AllSinglePropertyViews.Add( Property );

		return Property;
	}

	return NULL;
}


/*
TSharedRef< IPropertyTable > FRuntimeEditorModule::CreatePropertyTable()
{
	return MakeShareable( new FPropertyTable() );
}
*/

/*
TSharedRef< SWidget > FRuntimeEditorModule::CreatePropertyTableWidget( const TSharedRef< class IPropertyTable >& PropertyTable )
{
	return SNew( SPropertyTable, PropertyTable );
}
*/

/*
TSharedRef< SWidget > FRuntimeEditorModule::CreatePropertyTableWidget( const TSharedRef< IPropertyTable >& PropertyTable, const TArray< TSharedRef< class IPropertyTableCustomColumn > >& Customizations )
{
	return SNew( SPropertyTable, PropertyTable )
		.ColumnCustomizations( Customizations );
}
*/

/*
TSharedRef< IPropertyTableWidgetHandle > FRuntimeEditorModule::CreatePropertyTableWidgetHandle( const TSharedRef< IPropertyTable >& PropertyTable )
{
	TSharedRef< FPropertyTableWidgetHandle > FWidgetHandle = MakeShareable( new FPropertyTableWidgetHandle( SNew( SPropertyTable, PropertyTable ) ) );

	TSharedRef< IPropertyTableWidgetHandle > IWidgetHandle = StaticCastSharedRef<IPropertyTableWidgetHandle>(FWidgetHandle);

	 return IWidgetHandle;
}
*/

/*
TSharedRef< IPropertyTableWidgetHandle > FRuntimeEditorModule::CreatePropertyTableWidgetHandle( const TSharedRef< IPropertyTable >& PropertyTable, const TArray< TSharedRef< class IPropertyTableCustomColumn > >& Customizations )
{
	TSharedRef< FPropertyTableWidgetHandle > FWidgetHandle = MakeShareable( new FPropertyTableWidgetHandle( SNew( SPropertyTable, PropertyTable )
		.ColumnCustomizations( Customizations )));

	TSharedRef< IPropertyTableWidgetHandle > IWidgetHandle = StaticCastSharedRef<IPropertyTableWidgetHandle>(FWidgetHandle);

	 return IWidgetHandle;
}
*/

/*
TSharedRef< IPropertyTableCellPresenter > FRuntimeEditorModule::CreateTextPropertyCellPresenter(const TSharedRef< class FPropertyNode >& InPropertyNode, const TSharedRef< class IPropertyTableUtilities >& InPropertyUtilities, 
																								 const FSlateFontInfo* InFontPtr)
{
	FSlateFontInfo InFont;

	if (InFontPtr == NULL)
	{
		// Encapsulating reference to Private file PropertyTableConstants.h
		InFont = FSodaStyle::GetFontStyle( PropertyTableConstants::NormalFontStyle );
	}
	else
	{
		InFont = *InFontPtr;
	}

	TSharedRef< FPropertyEditor > PropertyEditor = FPropertyEditor::Create( InPropertyNode, InPropertyUtilities );
	return MakeShareable( new FTextPropertyTableCellPresenter( PropertyEditor, InPropertyUtilities, InFont) );
}
*/


FStructProperty* FRuntimeEditorModule::RegisterStructOnScopeProperty(TSharedRef<FStructOnScope> StructOnScope)
{
	const FName StructName = StructOnScope->GetStruct()->GetFName();
	FStructProperty* StructProperty = RegisteredStructToProxyMap.FindRef(StructName);

	if (!StructProperty)
	{
		if (!StructOnScopePropertyOwner)
		{
			// Create a container for all StructOnScope property objects.
			// It's important that this container is the owner of these properties and maintains a linked list 
			// to all of the properties created here. This is automatically handled by the specialized property constructor.
			StructOnScopePropertyOwner = NewObject<UStruct>(GetTransientPackage(), TEXT("StructOnScope"), RF_Transient);
			StructOnScopePropertyOwner->AddToRoot();
		}
		UScriptStruct* InnerStruct = Cast<UScriptStruct>(const_cast<UStruct*>(StructOnScope->GetStruct()));
		StructProperty = new FStructProperty(StructOnScopePropertyOwner, *MakeUniqueObjectName(StructOnScopePropertyOwner, UField::StaticClass(), InnerStruct->GetFName()).ToString(), RF_Transient);
		StructProperty->Struct = InnerStruct;
		StructProperty->ElementSize = StructOnScope->GetStruct()->GetStructureSize();
		StructOnScopePropertyOwner->AddCppProperty(StructProperty);

		RegisteredStructToProxyMap.Add(StructName, StructProperty);
	}

	return StructProperty;
}

/*
TSharedRef<IPropertyChangeListener> FRuntimeEditorModule::CreatePropertyChangeListener()
{
	return MakeShareable( new FPropertyChangeListener );
}
*/

void FRuntimeEditorModule::RegisterCustomClassLayout( FName ClassName, soda::FOnGetDetailCustomizationInstance DetailLayoutDelegate )
{
	if (ClassName != NAME_None)
	{
		soda::FDetailLayoutCallback Callback;
		Callback.DetailLayoutDelegate = DetailLayoutDelegate;
		// @todo: DetailsView: Fix me: this specifies the order in which detail layouts should be queried
		Callback.Order = ClassNameToDetailLayoutNameMap.Num();

		ClassNameToDetailLayoutNameMap.Add(ClassName, Callback);
	}
}

void FRuntimeEditorModule::UnregisterCustomClassLayout( FName ClassName )
{
	if (ClassName.IsValid() && (ClassName != NAME_None))
	{
		ClassNameToDetailLayoutNameMap.Remove(ClassName);
	}
}

void FRuntimeEditorModule::RegisterCustomPropertyTypeLayout(FName PropertyTypeName, soda::FOnGetPropertyTypeCustomizationInstance PropertyTypeLayoutDelegate, TSharedPtr<soda::IPropertyTypeIdentifier> Identifier)
{
	if (PropertyTypeName != NAME_None)
	{
		soda::FPropertyTypeLayoutCallback Callback;
		Callback.PropertyTypeLayoutDelegate = PropertyTypeLayoutDelegate;
		Callback.PropertyTypeIdentifier = Identifier;

		soda::FPropertyTypeLayoutCallbackList* LayoutCallbacks = GlobalPropertyTypeToLayoutMap.Find(PropertyTypeName);
		if (LayoutCallbacks)
		{
			LayoutCallbacks->Add(Callback);
		}
		else
		{
			soda::FPropertyTypeLayoutCallbackList NewLayoutCallbacks;
			NewLayoutCallbacks.Add(Callback);
			GlobalPropertyTypeToLayoutMap.Add(PropertyTypeName, NewLayoutCallbacks);
		}
	}
}

void FRuntimeEditorModule::UnregisterCustomPropertyTypeLayout(FName PropertyTypeName, TSharedPtr<soda::IPropertyTypeIdentifier> Identifier)
{
	if (!PropertyTypeName.IsValid() || (PropertyTypeName == NAME_None))
	{
		return;
	}

	soda::FPropertyTypeLayoutCallbackList* LayoutCallbacks = GlobalPropertyTypeToLayoutMap.Find(PropertyTypeName);

	if (LayoutCallbacks)
	{
		LayoutCallbacks->Remove(Identifier);
	}
}


/*
void FRuntimeEditorModule::UnregisterCustomPropertyTypeLayout( FName PropertyTypeName, TSharedPtr<IPropertyTypeIdentifier> Identifier)
{
	if (!PropertyTypeName.IsValid() || (PropertyTypeName == NAME_None))
	{
		return;
	}

	FPropertyTypeLayoutCallbackList* LayoutCallbacks = GlobalPropertyTypeToLayoutMap.Find(PropertyTypeName);

	if (LayoutCallbacks)
	{
		LayoutCallbacks->Remove(Identifier);
	}
}
*/

/*
bool FRuntimeEditorModule::HasUnlockedDetailViews() const
{
	uint32 NumUnlockedViews = 0;
	for( int32 ViewIndex = 0; ViewIndex < AllDetailViews.Num(); ++ViewIndex )
	{
		const TWeakPtr<SDetailsView>& DetailView = AllDetailViews[ ViewIndex ];
		if( DetailView.IsValid() )
		{
			TSharedPtr<SDetailsView> DetailViewPin = DetailView.Pin();
			if( DetailViewPin->IsUpdatable() &&  !DetailViewPin->IsLocked() )
			{
				return true;
			}
		}
	}

	return false;
}
*/

/**
 * Refreshes property windows with a new list of objects to view
 * 
 * @param NewObjectList	The list of objects each property window should view
 */

/*
void FRuntimeEditorModule::UpdatePropertyViews( const TArray<UObject*>& NewObjectList )
{
	DestroyColorPicker();

	TSet<AActor*> ValidObjects;
	
	for( int32 ViewIndex = 0; ViewIndex < AllDetailViews.Num(); ++ViewIndex )
	{
		if( AllDetailViews[ ViewIndex ].IsValid() )
		{
			TSharedPtr<SDetailsView> DetailViewPin = AllDetailViews[ ViewIndex ].Pin();
			if( DetailViewPin->IsUpdatable() )
			{
				if( !DetailViewPin->IsLocked() )
				{
					DetailViewPin->SetObjects(NewObjectList, true);
				}
				else
				{
					DetailViewPin->ForceRefresh();
				}
			}
		}
	}
}
*/

/*
void FRuntimeEditorModule::ReplaceViewedObjects( const TMap<UObject*, UObject*>& OldToNewObjectMap )
{
	// Replace objects from detail views
	for( int32 ViewIndex = 0; ViewIndex < AllDetailViews.Num(); ++ViewIndex )
	{
		if( AllDetailViews[ ViewIndex ].IsValid() )
		{
			TSharedPtr<SDetailsView> DetailViewPin = AllDetailViews[ ViewIndex ].Pin();

			DetailViewPin->ReplaceObjects( OldToNewObjectMap );
		}
	}

	// Replace objects from single views
	for( int32 ViewIndex = 0; ViewIndex < AllSinglePropertyViews.Num(); ++ViewIndex )
	{
		if( AllSinglePropertyViews[ ViewIndex ].IsValid() )
		{
			TSharedPtr<SSingleProperty> SinglePropPin = AllSinglePropertyViews[ ViewIndex ].Pin();

			SinglePropPin->ReplaceObjects( OldToNewObjectMap );
		}
	}
}
*/

/*
void FRuntimeEditorModule::RemoveDeletedObjects( TArray<UObject*>& DeletedObjects )
{
	DestroyColorPicker();

	// remove deleted objects from detail views
	for( int32 ViewIndex = 0; ViewIndex < AllDetailViews.Num(); ++ViewIndex )
	{
		if( AllDetailViews[ ViewIndex ].IsValid() )
		{
			TSharedPtr<SDetailsView> DetailViewPin = AllDetailViews[ ViewIndex ].Pin();

			DetailViewPin->RemoveDeletedObjects( DeletedObjects );
		}
	}

	// remove deleted object from single property views
	for( int32 ViewIndex = 0; ViewIndex < AllSinglePropertyViews.Num(); ++ViewIndex )
	{
		if( AllSinglePropertyViews[ ViewIndex ].IsValid() )
		{
			TSharedPtr<SSingleProperty> SinglePropPin = AllSinglePropertyViews[ ViewIndex ].Pin();

			SinglePropPin->RemoveDeletedObjects( DeletedObjects );
		}
	}
}
*/

TSharedRef<soda::IStructureDetailsView> FRuntimeEditorModule::CreateStructureDetailView(const soda::FDetailsViewArgs& DetailsViewArgs, const soda::FStructureDetailsViewArgs& StructureDetailsViewArgs, TSharedPtr<FStructOnScope> StructData, const FText& CustomName)
{
	TSharedRef<soda::SStructureDetailsView> DetailView =
		SNew(soda::SStructureDetailsView)
		.DetailsViewArgs(DetailsViewArgs)
		.CustomName(CustomName);

	struct FStructureDetailsViewFilter
	{
		static bool HasFilter( const soda::FStructureDetailsViewArgs InStructureDetailsViewArgs )
		{
			const bool bShowEverything = InStructureDetailsViewArgs.bShowObjects 
				&& InStructureDetailsViewArgs.bShowAssets 
				&& InStructureDetailsViewArgs.bShowClasses 
				&& InStructureDetailsViewArgs.bShowInterfaces;
			return !bShowEverything;
		}

		static bool PassesFilter( const soda::FPropertyAndParent& PropertyAndParent, const soda::FStructureDetailsViewArgs InStructureDetailsViewArgs )
		{
			const auto ArrayProperty = CastField<FArrayProperty>(&PropertyAndParent.Property);
			const auto SetProperty = CastField<FSetProperty>(&PropertyAndParent.Property);
			const auto MapProperty = CastField<FMapProperty>(&PropertyAndParent.Property);

			// If the property is a container type, the filter should test against the type of the container's contents
			const FProperty* PropertyToTest = ArrayProperty ? ArrayProperty->Inner : &PropertyAndParent.Property;
			PropertyToTest = SetProperty ? SetProperty->ElementProp : PropertyToTest;
			PropertyToTest = MapProperty ? MapProperty->ValueProp : PropertyToTest;

			// Meta-data should always be queried off the top-level property, as the inner (for container types) is generated by UHT
			const FProperty* MetaDataProperty = &PropertyAndParent.Property;

			if( InStructureDetailsViewArgs.bShowClasses && (PropertyToTest->IsA<FClassProperty>() || PropertyToTest->IsA<FSoftClassProperty>()) )
			{
				return true;
			}

			if( InStructureDetailsViewArgs.bShowInterfaces && PropertyToTest->IsA<FInterfaceProperty>() )
			{
				return true;
			}

			const auto ObjectProperty = CastField<FObjectPropertyBase>(PropertyToTest);
			if( ObjectProperty )
			{
				if( InStructureDetailsViewArgs.bShowAssets )
				{
					// Is this an "asset" property?
					if( PropertyToTest->IsA<FSoftObjectProperty>())
					{
						return true;
					}

					// Not an "asset" property, but it may still be a property using an asset class type (such as a raw pointer)
					if( ObjectProperty->PropertyClass )
					{
						if (ensure(MetaDataProperty) && FRuntimeMetaData::HasMetaData(MetaDataProperty, TEXT("AllowedClasses")))
						{
							return true;
						}
						else
						{
							// We can use the asset tools module to see whether this type has asset actions (which likely means it's an asset class type)
							//FAssetToolsModule& AssetToolsModule = FAssetToolsModule::GetModule();
							//return AssetToolsModule.Get().GetAssetTypeActionsForClass(ObjectProperty->PropertyClass).IsValid();
							return false;
						}
					}
				}

				return InStructureDetailsViewArgs.bShowObjects;
			}

			return true;
		}
	};

	// Only add the filter if we need to exclude things
	if (FStructureDetailsViewFilter::HasFilter(StructureDetailsViewArgs))
	{
		DetailView->SetIsPropertyVisibleDelegate(soda::FIsPropertyVisible::CreateStatic(&FStructureDetailsViewFilter::PassesFilter, StructureDetailsViewArgs));
	}
	DetailView->SetStructureData(StructData);

	return DetailView;
}


TSharedRef<soda::IPropertyRowGenerator> FRuntimeEditorModule::CreatePropertyRowGenerator(const soda::FPropertyRowGeneratorArgs& InArgs)
{
	return MakeShared<soda::FPropertyRowGenerator>(InArgs/*, GetThumbnailPool()*/);
}


void FRuntimeEditorModule::RegisterObjectCustomizations()
{
	RegisterCustomClassLayout("Object", soda::FOnGetDetailCustomizationInstance::CreateStatic(&soda::FObjectDetails::MakeInstance));
	RegisterCustomClassLayout("Actor", soda::FOnGetDetailCustomizationInstance::CreateStatic(&soda::FActorDetails::MakeInstance));
	RegisterCustomClassLayout("ActorComponent", soda::FOnGetDetailCustomizationInstance::CreateStatic(&soda::FActorComponentDetails::MakeInstance));
	RegisterCustomClassLayout("SceneComponent", soda::FOnGetDetailCustomizationInstance::CreateStatic(&soda::FSceneComponentDetails::MakeInstance));	
}

void FRuntimeEditorModule::RegisterPropertyTypeCustomizations()
{
	RegisterCustomPropertyTypeLayout("SoftObjectPath", soda::FOnGetPropertyTypeCustomizationInstance::CreateStatic(&soda::FSoftObjectPathCustomization::MakeInstance));
	RegisterCustomPropertyTypeLayout("Key", soda::FOnGetPropertyTypeCustomizationInstance::CreateStatic(&soda::FKeyStructCustomization::MakeInstance));
	RegisterCustomPropertyTypeLayout("ComponentReference", soda::FOnGetPropertyTypeCustomizationInstance::CreateStatic(&soda::FComponentReferenceCustomization::MakeInstance));
}

class TSharedRef< soda::IDocumentation > FRuntimeEditorModule::GetDocumentation() const
{
	return Documentation.ToSharedRef();
}

IMPLEMENT_MODULE(FRuntimeEditorModule, RuntimeEditor);

