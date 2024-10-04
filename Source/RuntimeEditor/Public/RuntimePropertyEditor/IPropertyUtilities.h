// Copyright Epic Games, Inc. All Rights Reserved.
// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
//#include "AssetThumbnail.h"

struct FPropertyChangedEvent;
class FNotifyHook;

namespace soda
{

class FEditConditionParser;
class IClassViewerFilter;

/**
 * Settings for property editor widgets that call up to the base container for the widgets
 * without knowing information about that container
 */
class IPropertyUtilities
{
public:
	virtual ~IPropertyUtilities(){}
	virtual FNotifyHook* GetNotifyHook() const = 0;
	virtual bool AreFavoritesEnabled() const = 0;
	virtual void ToggleFavorite( const TSharedRef< class FPropertyEditor >& PropertyEditor ) const = 0;
	virtual void CreateColorPickerWindow( const TSharedRef< class FPropertyEditor >& PropertyEditor, bool bUseAlpha ) const = 0;
	virtual void EnqueueDeferredAction( FSimpleDelegate DeferredAction ) = 0;
	virtual bool IsPropertyEditingEnabled() const = 0;
	virtual void ForceRefresh() = 0;
	virtual void RequestRefresh() = 0;
	//virtual TSharedPtr<class FAssetThumbnailPool> GetThumbnailPool() const = 0;
	virtual void NotifyFinishedChangingProperties(const FPropertyChangedEvent& PropertyChangedEvent) = 0;
	virtual bool HasClassDefaultObject() const = 0;
	virtual const TArray<TWeakObjectPtr<UObject>>& GetSelectedObjects() const = 0;
	/** If a customization standalone widget is used, the value should be update only once, when its window is closed */
	virtual bool DontUpdateValueWhileEditing() const = 0;
	virtual const TArray<TSharedRef<IClassViewerFilter>>& GetClassViewerFilters() const = 0;
};

} // namespace soda
