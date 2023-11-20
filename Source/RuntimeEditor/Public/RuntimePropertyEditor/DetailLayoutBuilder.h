// Copyright Epic Games, Inc. All Rights Reserved.
// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Fonts/SlateFontInfo.h"
#include "SodaStyleSet.h"
#include "RuntimePropertyEditor/PropertyHandle.h"
#include "RuntimePropertyEditor/IDetailPropertyRow.h"

namespace soda
{

class IDetailCategoryBuilder;
class IDetailsView;

namespace ECategoryPriority
{
	enum Type
	{

		// Highest sort priority
		Variable = 0,
		Transform,
		Important,
		TypeSpecific,
		Default,
		// Lowest sort priority
		Uncommon,
	};
}

/**
 * The builder for laying custom details
 */
class IDetailLayoutBuilder
{
	
public:
	using FOnCategorySortOrderFunction = TFunction<void(const TMap<FName, IDetailCategoryBuilder*>& /* AllCategoryMap */)>;

	virtual ~IDetailLayoutBuilder(){}

	/**
	 * @return the font used for properties and details
	 */ 
	static FSlateFontInfo GetDetailFont() { return FSodaStyle::GetFontStyle( TEXT("PropertyWindow.NormalFont") ); }

	/**
	 * @return the bold font used for properties and details
	 */ 
	static FSlateFontInfo GetDetailFontBold() { return FSodaStyle::GetFontStyle( TEXT("PropertyWindow.BoldFont") ); }
	
	/**
	 * @return the italic font used for properties and details
	 */ 
	static FSlateFontInfo GetDetailFontItalic() { return FSodaStyle::GetFontStyle( TEXT("PropertyWindow.ItalicFont") ); }
	
	/**
	 * @return the parent detail view for this layout builder
	 */
	virtual const IDetailsView* GetDetailsView() const = 0;

	/**
	 * @return the parent detail view for this layout builder
	 */
	virtual IDetailsView* GetDetailsView() = 0;

	/**
	 * @return The base class of the objects being customized in this detail layout
	 */
	virtual UClass* GetBaseClass() const = 0;
	
	/**
	 * Get the root objects observed by this layout.  
	 * This is not guaranteed to be the same as the objects customized by this builder.  See GetObjectsBeingCustomized for that.
	 */
	virtual const TArray< TWeakObjectPtr<UObject> >& GetSelectedObjects() const = 0;

	/**
	 * Gets the current object(s) being customized by this builder
	 *
	 * If this is a sub-object customization it will return those sub objects.  Otherwise the root objects will be returned.
	 */
	virtual void GetObjectsBeingCustomized( TArray< TWeakObjectPtr<UObject> >& OutObjects ) const = 0;

	/**
	 * Gets the current struct(s) being customized by this builder
	 *
	 * If this is a sub-struct customization it will return those sub struct.  Otherwise the root struct will be returned.
	 */
	virtual void GetStructsBeingCustomized( TArray< TSharedPtr<FStructOnScope> >& OutStructs ) const = 0;

	/**
	 *	@return the utilities various widgets need access to certain features of PropertyDetails
	 */
	virtual const TSharedRef< class IPropertyUtilities > GetPropertyUtilities() const = 0; 


	/**
	 * Edits an existing category or creates a new one
	 * 
	 * @param CategoryName				The name of the category
	 * @param NewLocalizedDisplayName	The new display name of the category (optional)
	 * @param CategoryType				Category type to define sort order.  Category display order is sorted by this type (optional)
	 */
	virtual IDetailCategoryBuilder& EditCategory(FName CategoryName, const FText& NewLocalizedDisplayName = FText::GetEmpty(), ECategoryPriority::Type CategoryType = ECategoryPriority::Default) = 0;

	/**
	 * Gets the current set of existing category names. This includes both categories derived from properties and categories added via EditCategory.
	 * @param	OutCategoryNames	 The array of category names
	 */
	virtual void GetCategoryNames(TArray<FName>& OutCategoryNames) const = 0;

	/**
	 * Adds sort algorythm which overrides standard algorythm with that provided by the caller.
	 * Function called on each category after all categories have been added, and provides caller
	 * with ability to override sort order.
	 * @param 	SortFunction Function called on final pass of detail panel build, which provides map of all categories to set final sort order on.
	 */
	virtual void SortCategories(const FOnCategorySortOrderFunction& SortFunction) = 0;

	/** 
	 * Adds the property to its given category automatically. Useful in detail customizations which want to preserve categories.
	 * @param InPropertyHandle			The handle to the property that you want to add to its own category.
	 * @return the property row with which the property was added.
	 */
	virtual IDetailPropertyRow& AddPropertyToCategory(TSharedPtr<IPropertyHandle> InPropertyHandle) = 0;

	/**
	 * Adds a custom row to the property's category automatically. Useful in detail customizations which want to preserve categories.
	 * @param InPropertyHandle			The handle to the property that you want to add to its own category.
	 * @param InCustomSearchString		A string which is used to filter this custom row when a user types into the details panel search box.
	 * @return the detail widget that can be further customized.
	 */
	virtual FDetailWidgetRow& AddCustomRowToCategory(TSharedPtr<IPropertyHandle> InPropertyHandle, const FText& InCustomSearchString, bool bForAdvanced = false) = 0;

	/**
	 * Adds an external object's property to this details panel's PropertyMap.
	 * Allows getting the property handle for the property without having to generate a row widget.
	 *
	 * @param Objects		List of objects that contain the property.
	 * @param PropertyName	Name of the property to generate a node from.
	 * @return The property handle created tied to generated property node.
	 */	
	virtual TSharedPtr<IPropertyHandle> AddObjectPropertyData(TConstArrayView<UObject*> Objects, FName PropertyName) = 0;

	/**
	 * Adds an external structure's property data to this details panel's PropertyMap.
	 * Allows getting the property handle for the property without having to generate a row widget.
	 *
	 * @param StructData    Struct data to find the property within.
	 * @param PropertyName	Name of the property to generate a node from.
	 * @return			    The property handle tied to the generated property node.
	 */
	virtual TSharedPtr<IPropertyHandle> AddStructurePropertyData(const TSharedPtr<FStructOnScope>& StructData, FName PropertyName) = 0;

	/**
	 * Allows for the customization of a property row for a property that already exists on a class being edited in the details panel
	 * The property will remain in the default location but the widget or other attributes for the property can be changed 
	 * Note This cannot be used to customize other customizations

	 * @param InPropertyHandle	The handle to the property that you want to add to its own category.
	 * @return					The property row to edit or nullptr if the property row does not exist
	 */
	virtual IDetailPropertyRow* EditDefaultProperty(TSharedPtr<IPropertyHandle> InPropertyHandle) = 0;

	/**
	 * Hides an entire category
	 *
	 * @param CategoryName	The name of the category to hide
	 */
	virtual void HideCategory( FName CategoryName ) = 0;
	
	/**
	 * Gets a handle to a property which can be used to read and write the property value and identify the property in other detail customization interfaces.
	 * Instructions
	 *
	 * @param Path	The path to the property.  Can be just a name of the property or a path in the format outer.outer.value[optional_index_for_static_arrays]
	 * @param ClassOutermost	Optional outer class if accessing a property outside of the current class being customized
	 * @param InstanceName		Optional instance name if multiple FProperty's of the same type exist. such as two identical structs, the instance name is one of the struct variable names)
	    Examples:

		struct MyStruct
		{ 
			int32 StaticArray[3];
			float FloatVar;
		}

		class MyActor
		{ 
			MyStruct Struct1;
			MyStruct Struct2;
			float MyFloat
		}
		
		To access StaticArray at index 2 from Struct2 in MyActor, your path would be MyStruct.StaticArray[2]" and your instance name is "Struct2"
		To access MyFloat in MyActor you can just pass in "MyFloat" because the name of the property is unambiguous
	 */
	virtual TSharedRef<IPropertyHandle> GetProperty( const FName PropertyPath, const UStruct* ClassOutermost = NULL, FName InstanceName = NAME_None ) const = 0;

	/**
	 * Gets the top level property, for showing the warning for experimental or early access class
	 * 
	 * @return the top level property name
	 */

	virtual FName GetTopLevelProperty() = 0;

	/**
	 * Hides a property from view 
	 *
	 * @param PropertyHandle	The handle of the property to hide from view
	 */
	virtual void HideProperty( const TSharedPtr<IPropertyHandle> PropertyHandle ) = 0;

	/**
	 * Hides a property from view
	 *
	 * @param Path						The path to the property.  Can be just a name of the property or a path in the format outer.outer.value[optional_index_for_static_arrays]
	 * @param NewLocalizedDisplayName	Optional display name to show instead of the default name
	 * @param ClassOutermost			Optional outer class if accessing a property outside of the current class being customized
	 * @param InstanceName				Optional instance name if multiple FProperty's of the same type exist. such as two identical structs, the instance name is one of the struct variable names)
	 * See IDetailCategoryBuilder::GetProperty for clarification of parameters
	 */
	virtual void HideProperty( FName PropertyPath, const UStruct* ClassOutermost = NULL, FName InstanceName = NAME_None ) = 0;

	/**
	 * Refreshes the details view and regenerates all the customized layouts
	 * Use only when you need to remove or add complicated dynamic items
	 */
	virtual void ForceRefreshDetails() = 0;

	/**
	 * Gets the thumbnail pool that should be used for rendering thumbnails in the details view                   
	 */
	//virtual TSharedPtr<class FAssetThumbnailPool> GetThumbnailPool() const = 0;

	/**
	 * @return true if the property should be visible in the details panel or false if the specific details panel is not showing this property
	 */
	virtual bool IsPropertyVisible( TSharedRef<IPropertyHandle> PropertyHandle ) const = 0;

	/**
	 * @return true if the property should be visible in the details panel or false if the specific details panel is not showing this property
	 */
	virtual bool IsPropertyVisible( const struct FPropertyAndParent& PropertyAndParent ) const = 0;

	/**
	 * @return True if an object in the builder is a class default object
	 */
	virtual bool HasClassDefaultObject() const = 0;

	/**
	 * Registers a custom detail layout delegate for a specific type in this layout only
	 *
	 * @param PropertyTypeName	The type the custom detail layout is for
	 * @param DetailLayoutDelegate	The delegate to call when querying for custom detail layouts for the classes properties
	 */
	virtual void RegisterInstancedCustomPropertyTypeLayout(FName PropertyTypeName, FOnGetPropertyTypeCustomizationInstance PropertyTypeLayoutDelegate, TSharedPtr<IPropertyTypeIdentifier> Identifier = nullptr) = 0;
};

} // namespace soda