// Copyright Epic Games, Inc. All Rights Reserved.
// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
//#include "EditorConfigBase.h"

#include "DetailsViewConfig2.generated.h"

USTRUCT()
struct FDetailsSectionSelection2
{
	GENERATED_BODY()

public:

	UPROPERTY()
	TSet<FName> SectionNames;
};

USTRUCT()
struct FDetailsViewConfig2
{
	GENERATED_BODY()

public:
	
	/** If we should show the favorites category. */
	UPROPERTY()
	bool bShowFavoritesCategory { false };

	/** When enabled, the Advanced Details will always auto expand. */
	UPROPERTY()
	bool bShowAllAdvanced { false };

	/** When Playing or Simulating, shows all properties (even non-visible and non-editable properties), if the object belongs to a simulating world.  This is useful for debugging. */
	UPROPERTY()
	bool bShowHiddenPropertiesWhilePlaying { false };

	/** Show all category children if the category matches the filter. */
	UPROPERTY()
	bool bShowAllChildrenIfCategoryMatches { false };

	/** Show only keyable properties. */
	UPROPERTY()
	bool bShowOnlyKeyable { false };

	/** Show only animated properties. */
	UPROPERTY()
	bool bShowOnlyAnimated { false };

	/** Show only modified properties. */
	UPROPERTY()
	bool bShowOnlyModified { false };

	/** Show sections. */
	UPROPERTY()
	bool bShowSections { true };

	/** Width of the value column in the details view (0.0-1.0). */
	UPROPERTY()
	float ValueColumnWidth { 0 };

	/** A map of class name to a set of selected sections for that class. */
	UPROPERTY()
	TMap<FName, FDetailsSectionSelection2> SelectedSections;
};

UCLASS(EditorConfig="DetailsView")
class UDetailsConfig2 : public UObject /*UEditorConfigBase*/
{
	GENERATED_BODY()
	
public:

	UPROPERTY(meta=(EditorConfig))
	TMap<FName, FDetailsViewConfig2> Views;
};