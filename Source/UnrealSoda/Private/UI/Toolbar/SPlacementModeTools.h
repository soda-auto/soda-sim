// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Layout/Visibility.h"
#include "Input/Reply.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SWidget.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Layout/SUniformWrapPanel.h"
#include "Widgets/Views/STableViewBase.h"
#include "Widgets/Views/STableRow.h"
#include "Misc/TextFilter.h"
#include "Styling/SlateIconFinder.h"
#include "Soda/ISodaActor.h"
#include "Soda/SodaGameViewportClient.h"

namespace soda
{

struct FPlaceableItem
{
	FPlaceableItem(
		TSoftClassPtr<AActor> InDefaultActor,
		FName InClassThumbnailBrushOverride = NAME_None,
		FName InClassIconBrushOverride = NAME_None,
		TOptional<FLinearColor> InAssetTypeColorOverride = TOptional<FLinearColor>(),
		TOptional<FName> InDisplayName = TOptional<FName>()
	)
		: DefaultActor(InDefaultActor)
		, ClassThumbnailBrushOverride(InClassThumbnailBrushOverride)
		, ClassIconBrushOverride(InClassIconBrushOverride)
		, AssetTypeColorOverride(InAssetTypeColorOverride)
	{
		if (InDisplayName.IsSet())
		{
			DisplayName = InDisplayName.GetValue();
		}
		else if(InDefaultActor.Get())
		{
			DisplayName = InDefaultActor.Get()->GetDefaultObject()->GetFName();
		}
		else
		{
			DisplayName = *InDefaultActor.GetAssetName();
		}
	}

	FPlaceableItem(
		const FSpawnCastomActorDelegate & InSpawnDelegate,
		FName InClassThumbnailBrushOverride = NAME_None,
		FName InClassIconBrushOverride = NAME_None,
		TOptional<FLinearColor> InAssetTypeColorOverride = TOptional<FLinearColor>(),
		TOptional<FName> InDisplayName = TOptional<FName>()
	)
		: SpawnCastomActorDelegate(InSpawnDelegate)
		, ClassThumbnailBrushOverride(InClassThumbnailBrushOverride)
		, ClassIconBrushOverride(InClassIconBrushOverride)
		, AssetTypeColorOverride(InAssetTypeColorOverride)
	{
		if (InDisplayName.IsSet())
		{
			DisplayName = InDisplayName.GetValue();
		}
		else
		{
			DisplayName = TEXT("Undefined");
		}
	}

public:
	TSoftClassPtr<AActor> DefaultActor;

	FSpawnCastomActorDelegate SpawnCastomActorDelegate;

	/** This item's display name */
	FName DisplayName;

	/** Optional override for the thumbnail brush (passed to FClassIconFinder::FindThumbnailForClass in the form ClassThumbnail.<override>) */
	FName ClassThumbnailBrushOverride;

	/** Optional override for the small icon brush */
	FName ClassIconBrushOverride;

	/** Optional overridden color tint for the asset */
	TOptional<FLinearColor> AssetTypeColorOverride;
};

/**
 * SPlacementAssetMenuEntry
 */
class SPlacementAssetMenuEntry : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SPlacementAssetMenuEntry){}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const TSharedPtr<const FPlaceableItem>& InItem);

	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnDragDetected(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;

	bool IsPressed() const;

	TSharedPtr<const FPlaceableItem> Item;

	virtual FSlateColor GetForegroundColor() const override;

private:
	const FSlateBrush* GetBorder() const;
	const FSlateBrush* GetIcon() const
	{
		if (AssetImage != nullptr)
		{
			return AssetImage;
		}

		if (Item->ClassIconBrushOverride != NAME_None)
		{
			AssetImage = FSlateIconFinder::FindCustomIconBrushForClass(nullptr, TEXT("ClassIcon"), Item->ClassIconBrushOverride);
		}

		if (!AssetImage)
		{
			AssetImage = FSlateIconFinder::FindCustomIconBrushForClass(nullptr, TEXT("ClassIcon"), FName(TEXT("ClassIcon.Actor")));
		}

		return AssetImage;
	}

	bool bIsPressed;

	const FButtonStyle* Style;
	
	mutable const FSlateBrush* AssetImage;
};


} //namespace soda
