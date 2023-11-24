// Copyright Epic Games, Inc. All Rights Reserved.
// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "RuntimePropertyEditor/CategoryPropertyNode.h"
#include "Misc/ConfigCacheIni.h"
#include "RuntimePropertyEditor/ItemPropertyNode.h"
//#include "ObjectEditorUtils.h"
#include "RuntimePropertyEditor/ObjectPropertyNode.h"
#include "RuntimePropertyEditor/PropertyEditorHelpers.h"

#include "RuntimeEditorUtils.h"
#include "RuntimeMetaData.h"

namespace soda
{

FCategoryPropertyNode::FCategoryPropertyNode(void)
	: FPropertyNode()
{
}

FCategoryPropertyNode::~FCategoryPropertyNode(void)
{
}

bool FCategoryPropertyNode::IsSubcategory() const
{
	return GetParentNode() != nullptr && GetParentNode()->AsCategoryNode() != nullptr;
}

FText FCategoryPropertyNode::GetDisplayName() const 
{
	FString SubcategoryName = GetSubcategoryName();
	return FText::FromString( SubcategoryName );
}	
/**
 * Overridden function for special setup
 */
void FCategoryPropertyNode::InitBeforeNodeFlags()
{

}
/**
 * Overridden function for Creating Child Nodes
 */
void FCategoryPropertyNode::InitChildNodes()
{
	const bool bShowHiddenProperties = !!HasNodeFlags( EPropertyNodeFlags::ShouldShowHiddenProperties );
	const bool bShouldShowDisableEditOnInstance = !!HasNodeFlags(EPropertyNodeFlags::ShouldShowDisableEditOnInstance);
	const bool bGameModeOnlyVisible = !!HasNodeFlags(EPropertyNodeFlags::GameModeOnlyVisible);

	TArray<FProperty*> Properties;
	TSet<FProperty*> SparseProperties;
	// The parent of a category window has to be an object window.
	FComplexPropertyNode* ComplexNode = FindComplexParent();
	if (ComplexNode)
	{
		FObjectPropertyNode* ObjectNode = FindObjectItemParent();
		
		// Get a list of properties that are in the same category
		for (const UStruct* Structure : ComplexNode->GetAllStructures())
		{
			const bool bIsSparseStruct = (ObjectNode && ObjectNode->IsSparseDataStruct(Cast<const UScriptStruct>(Structure)));
			for (TFieldIterator<FProperty> It(Structure); It; ++It)
			{
				bool bMetaDataAllowVisible = true;
				if (!bShowHiddenProperties)
				{
					static const FName Name_bShowOnlyWhenTrue("bShowOnlyWhenTrue");
					const FString& MetaDataVisibilityCheckString = FRuntimeMetaData::GetMetaData(*It, Name_bShowOnlyWhenTrue);
					if (MetaDataVisibilityCheckString.Len())
					{
						//ensure that the metadata visibility string is actually set to true in order to show this property
						// @todo Remove this
						GConfig->GetBool(TEXT("UnrealEd.PropertyFilters"), *MetaDataVisibilityCheckString, bMetaDataAllowVisible, GEditorPerProjectIni);
					}
				}

				if (bMetaDataAllowVisible)
				{
					// Add if we are showing non-editable props and this is the 'None' category, 
					// or if this is the right category, and we are showing non-editable
					if (FRuntimeEditorUtils::GetCategoryFName(*It) == CategoryName && PropertyEditorHelpers::ShouldBeVisible(*this, *It))
					{
						if (bIsSparseStruct)
						{
							SparseProperties.Add(*It);
						}

						Properties.Add(*It);
					}
				}
			}
		}
	}

	PropertyEditorHelpers::OrderPropertiesFromMetadata(Properties);

	for( int32 PropertyIndex = 0 ; PropertyIndex < Properties.Num() ; ++PropertyIndex )
	{
		TSharedPtr<FItemPropertyNode> NewItemNode( new FItemPropertyNode );

		FPropertyNodeInitParams InitParams;
		InitParams.ParentNode = SharedThis(this);
		InitParams.Property = Properties[PropertyIndex];
		InitParams.ArrayOffset = 0;
		InitParams.ArrayIndex = INDEX_NONE;
		InitParams.bAllowChildren = true;
		InitParams.bForceHiddenPropertyVisibility = bShowHiddenProperties;
		InitParams.bCreateDisableEditOnInstanceNodes = bShouldShowDisableEditOnInstance;
		InitParams.IsSparseProperty = SparseProperties.Contains(Properties[PropertyIndex]) ? FPropertyNodeInitParams::EIsSparseDataProperty::True : FPropertyNodeInitParams::EIsSparseDataProperty::Inherit;
		InitParams.bGameModeOnlyVisible = bGameModeOnlyVisible;

		NewItemNode->InitNode( InitParams );

		AddChildNode(NewItemNode);
	}
}


/**
 * Appends my path, including an array index (where appropriate)
 */
bool FCategoryPropertyNode::GetQualifiedName( FString& PathPlusIndex, const bool bWithArrayIndex, const FPropertyNode* StopParent, bool bIgnoreCategories ) const
{
	bool bAddedAnything = false;

	const TSharedPtr<FPropertyNode> ParentNode = ParentNodeWeakPtr.Pin();
	if (ParentNode && StopParent != ParentNode.Get())
	{
		bAddedAnything = ParentNode->GetQualifiedName(PathPlusIndex, bWithArrayIndex, StopParent, bIgnoreCategories );
	}
	
	if (!bIgnoreCategories)
	{
		if (bAddedAnything)
		{
			PathPlusIndex += TEXT(".");
		}

		GetCategoryName().AppendString(PathPlusIndex);
		bAddedAnything = true;
	}

	return bAddedAnything;
}

FString FCategoryPropertyNode::GetSubcategoryName() const 
{
	FString SubcategoryName;
	{
		// The category name may actually contain a path of categories.  When displaying this category
		// in the property window, we only want the leaf part of the path
		const FString& CategoryPath = GetCategoryName().ToString();
		FString CategoryDelimiterString;
		CategoryDelimiterString.AppendChar( FPropertyNodeConstants::CategoryDelimiterChar );  
		const int32 LastDelimiterCharIndex = CategoryPath.Find( CategoryDelimiterString, ESearchCase::IgnoreCase, ESearchDir::FromEnd );
		if( LastDelimiterCharIndex != INDEX_NONE )
		{
			// Grab the last sub-category from the path
			SubcategoryName = CategoryPath.Mid( LastDelimiterCharIndex + 1 );
		}
		else
		{
			// No sub-categories, so just return the original (clean) category path
			SubcategoryName = CategoryPath;
		}
	}
	return SubcategoryName;
}


} // namespace soda