// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SWidget.h"
#include "Widgets/SToolTip.h"


namespace FRuntimeEditorUtils
{
	/**
	 * Gets the page that documentation for this class is contained on
	 *
	 * @param	InClass		Class we want to find the documentation page of
	 * @return				Path to the documentation page
	 */
	RUNTIMEEDITOR_API FString GetDocumentationPage(const UClass* Class);

	/**
	 * Gets the excerpt to use for this class
	 * Excerpt will be contained on the page returned by GetDocumentationPage
	 *
	 * @param	InClass		Class we want to find the documentation excerpt of
	 * @return				Name of the to the documentation excerpt
	 */
	RUNTIMEEDITOR_API FString GetDocumentationExcerpt(const UClass* Class);

	/**
	 * Gets the tooltip to display for a given class
	 *
	 * @param	InClass		Class we want to build a tooltip for
	 * @return				Shared reference to the constructed tooltip
	 */
	RUNTIMEEDITOR_API TSharedRef<SToolTip> GetTooltip(const UClass* Class);

	/**
	 * Gets the tooltip to display for a given class with specified text for the tooltip
	 *
	 * @param	InClass			Class we want to build a tooltip for
	 * @param	OverrideText	The text to display on the standard tooltip
	 * @return					Shared reference to the constructed tooltip
	 */
	RUNTIMEEDITOR_API TSharedRef<SToolTip> GetTooltip(const UClass* Class, const TAttribute<FText>& OverrideText);

	//RUNTIMEEDITOR_API TSharedRef< class SToolTip > CreateDocToolTip(const TAttribute<FText>& Text, const TSharedPtr<SWidget>& OverrideContent, const FString& Link= "", const FString& ExcerptName="");

	//RUNTIMEEDITOR_API TSharedRef< class SToolTip > CreateDocToolTip(const TAttribute<FText>& Text, const TSharedRef<SWidget>& OverrideContent, const TSharedPtr<SVerticalBox>& DocVerticalBox, const FString& Link="", const FString& ExcerptName="");

	/**
	 * Fetches a UClass from the string name of the class
	 *
	 * @param	ClassName		Name of the class we want the UClass for
	 * @return					UClass pointer if it exists
	 */
	RUNTIMEEDITOR_API UClass* GetClassFromString(const FString& ClassName);

	RUNTIMEEDITOR_API bool ValidateActorName(const FText& InName, FText& OutErrorMessage);


	RUNTIMEEDITOR_API FText GetCategoryText(const FField* InField);

	RUNTIMEEDITOR_API FText GetCategoryText(const UField* InField);


	RUNTIMEEDITOR_API FString GetCategory(const FField* InField);

	RUNTIMEEDITOR_API FString GetCategory(const UField* InField);

	RUNTIMEEDITOR_API FName GetCategoryFName(const FField* InField);

	RUNTIMEEDITOR_API FName GetCategoryFName(const UField* InField);

	RUNTIMEEDITOR_API bool CanCreateBlueprintOfClass(const UClass* Class);

	RUNTIMEEDITOR_API FString GetFriendlyName(const FProperty* Property, UStruct* OwnerStruct = NULL);



	/**
	 * Checks to see if the specified category is hidden from the supplied class.
	 *
	 * @param  Class	The class you want to query.
	 * @param  Category A category path that you want to check.
	 * @return True if the category is hidden, false if not.
	 */
	RUNTIMEEDITOR_API bool IsCategoryHiddenFromClass(const UStruct* Class, const FText& Category);

	/**
	 * Checks to see if the specified category is hidden from the supplied class.
	 *
	 * @param  Class	The class you want to query.
	 * @param  Category A category path that you want to check.
	 * @return True if the category is hidden, false if not.
	 */
	RUNTIMEEDITOR_API bool IsCategoryHiddenFromClass(const UStruct* Class, const FString& Category);

	/**
	 * Checks to see if the specified category is hidden from the supplied Class, avoids recalculation of ClassHideCategories.
	 * Useful when checking the same class over and over again with different categories.
	 *
	 * @param ClassHideCategories	The categories tht have been hidden for the class
	 * @param  Class	The class you want to query.
	 * @param  Category A category path that you want to check.
	 * @return True if the category is hidden, false if not.
	 */
	RUNTIMEEDITOR_API bool IsCategoryHiddenFromClass(const TArray<FString>& ClassHideCategories, const UStruct* Class, const FString& Category);

	/**
	 * Parses out the class's "HideCategories" metadata, and returns it
	 * segmented and sanitized.
	 *
	 * @param  Class			The class you want to pull data from.
	 * @param  CategoriesOut	An array that will be filled with a list of hidden categories.
	 * @param  bHomogenize		Determines if the categories should be ran through expansion and display sanitation (useful even when not being displayed, for comparisons)
	 */
	RUNTIMEEDITOR_API void GetClassHideCategories(const UStruct* Class, TArray<FString>& CategoriesOut, bool bHomogenize = true);

	/**
	 * Expands any keys found in the category string (any terms found in square
	 * brackets), and sanitizes the name (spacing individual words, etc.).
	 *
	 * @param  RootCategory	An id denoting the root category that you want prefixing the result.
	 * @param  SubCategory	A sub-category that you want postfixing the result.
	 * @return A concatenated text string, with the two categories separated by a pipe, '|', character.
	 */
	RUNTIMEEDITOR_API FText GetCategoryDisplayString(const FText& UnsanitizedCategory);

	/**
	 * Expands any keys found in the category string (any terms found in square
	 * brackets), and sanitizes the name (spacing individual words, etc.).
	 *
	 * @param  RootCategory	An id denoting the root category that you want prefixing the result.
	 * @param  SubCategory	A sub-category that you want postfixing the result.
	 * @return A concatenated string, with the two categories separated by a pipe, '|', character.
	 */
	RUNTIMEEDITOR_API FString GetCategoryDisplayString(const FString& UnsanitizedCategory);

	/**
	 * Parses out the class's "ShowCategories" metadata, and returns it
	 * segmented and sanitized.
	 *
	 * @param  Class			The class you want to pull data from.
	 * @param  CategoriesOut	An array that will be filled with a list of shown categories.
	 */
	RUNTIMEEDITOR_API void GetClassShowCategories(const UStruct* Class, TArray<FString>& CategoriesOut);

	/**
	 * Performs a lookup into the category key table, retrieving a fully
	 * qualified category path for the specified key.
	 *
	 * @param  Key	The key you want a category path for.
	 * @return The category display string associated with the specified key (an empty string if an entry wasn't found).
	 */
	RUNTIMEEDITOR_API const FText& GetCategory(const FString& Key);

	/**
	 * Returns whether the specified asset is a UBlueprint or UBlueprintGeneratedClass (or any of their derived classes)
	 *
	 * @param	InAssetData		Reference to an asset data entry
	 * @param	bOutIsBPGC		Outputs whether the asset is a BlueprintGeneratedClass (or any of its derived classes)
	 * @return					Whether the specified asset is a UBlueprint or UBlueprintGeneratedClass (or any of their derived classes)
	 */
	RUNTIMEEDITOR_API bool IsBlueprintAsset(const FAssetData& InAssetData, bool* bOutIsBPGC = nullptr);

	/**
	 * Gets the class path from the asset tag (i.e. GeneratedClassPath tag on blueprints)
	 *
	 * @param	InAssetData		Reference to an asset data entry.
	 * @return					Class path or None if the asset cannot or doesn't have a class associated with it
	 */
	RUNTIMEEDITOR_API FTopLevelAssetPath GetClassPathNameFromAssetTag(const FAssetData& InAssetData);

	/**
	 * Gets the object path of the class associated with the specified asset
	 * (i.e. the BlueprintGeneratedClass of a Blueprint asset or the BlueprintGeneratedClass asset itself)
	 *
	 * @param	InAssetData					Reference to an asset data entry
	 * @param	bGenerateClassPathIfMissing	Whether to generate a class path if the class is missing (and the asset can have a class associated with it)
	 * @return								Class path or None if the asset cannot or doesn't have a class associated with it
	 */
	RUNTIMEEDITOR_API FTopLevelAssetPath GetClassPathNameFromAsset(const FAssetData& InAssetData, bool bGenerateClassPathIfMissing = false);

	/**
	 * Fetches the set of interface class object paths from an asset data entry containing the appropriate asset tag(s).
	 *
	 * @param	InAssetData		Reference to an asset data entry.
	 * @param	OutClassPaths	One or more interface class object paths, or empty if the corresponding asset tag(s) were not found.
	 */
	RUNTIMEEDITOR_API void GetImplementedInterfaceClassPathsFromAsset(const FAssetData& InAssetData, TArray<FString>& OutClassPaths);


	/** Check to see if a given class is blueprint skeleton class. */
	RUNTIMEEDITOR_API bool IsClassABlueprintSkeleton(const UClass* Class);

	RUNTIMEEDITOR_API FString GetDisplayNameHelper(const UObject& Object);
	RUNTIMEEDITOR_API FString GetDisplayNameHelper(const FField& Object);

	/**
	* Fix disabled tooltip in SMultiBoxWidget in not editor modes. See SMultiBoxWidget::OnVisualizeTooltip(). I hope Epic Game will fix it in the future.
	*/
	RUNTIMEEDITOR_API TSharedRef< SWidget > MakeWidget_HackTooltip(FMultiBoxBuilder& MultiBoxBuilder, FMultiBox::FOnMakeMultiBoxBuilderOverride* InMakeMultiBoxBuilderOverride = nullptr, TAttribute<float> InMaxHeight = TAttribute<float>());
};

