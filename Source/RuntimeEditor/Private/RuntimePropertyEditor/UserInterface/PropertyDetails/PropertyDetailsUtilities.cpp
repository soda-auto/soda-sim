// Copyright Epic Games, Inc. All Rights Reserved.
// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "RuntimePropertyEditor/UserInterface/PropertyDetails/PropertyDetailsUtilities.h"
#include "RuntimePropertyEditor/Presentation/PropertyEditor/PropertyEditor.h"
#include "RuntimePropertyEditor/IDetailsViewPrivate.h"

namespace soda
{

FPropertyDetailsUtilities::FPropertyDetailsUtilities(IDetailsViewPrivate& InDetailsView)
	: DetailsView( InDetailsView )
{
}

FNotifyHook* FPropertyDetailsUtilities::GetNotifyHook() const
{
	return DetailsView.GetNotifyHook();
}

bool FPropertyDetailsUtilities::AreFavoritesEnabled() const
{
	// not implemented
	return false;
}

void FPropertyDetailsUtilities::ToggleFavorite( const TSharedRef< FPropertyEditor >& PropertyEditor ) const
{
	// not implemented
}

void FPropertyDetailsUtilities::CreateColorPickerWindow( const TSharedRef< FPropertyEditor >& PropertyEditor, bool bUseAlpha ) const
{
	DetailsView.CreateColorPickerWindow( PropertyEditor, bUseAlpha );
}

void FPropertyDetailsUtilities::EnqueueDeferredAction( FSimpleDelegate DeferredAction )
{
	DetailsView.EnqueueDeferredAction( DeferredAction );
}

bool FPropertyDetailsUtilities::IsPropertyEditingEnabled() const
{
	return DetailsView.IsPropertyEditingEnabled();
}

void FPropertyDetailsUtilities::ForceRefresh()
{
	DetailsView.ForceRefresh();
}

void FPropertyDetailsUtilities::RequestRefresh()
{
	DetailsView.RefreshTree();
}
/*
TSharedPtr<class FAssetThumbnailPool> FPropertyDetailsUtilities::GetThumbnailPool() const
{
	return DetailsView.GetThumbnailPool();
}
*/

const TArray<TSharedRef<IClassViewerFilter>>& FPropertyDetailsUtilities::GetClassViewerFilters() const
{
	return DetailsView.GetClassViewerFilters();
}

void FPropertyDetailsUtilities::NotifyFinishedChangingProperties(const FPropertyChangedEvent& PropertyChangedEvent)
{
	DetailsView.NotifyFinishedChangingProperties(PropertyChangedEvent);
}

bool FPropertyDetailsUtilities::DontUpdateValueWhileEditing() const
{
	return DetailsView.DontUpdateValueWhileEditing();
}

const TArray<TWeakObjectPtr<UObject>>& FPropertyDetailsUtilities::GetSelectedObjects() const
{
	return DetailsView.GetSelectedObjects();
}

bool FPropertyDetailsUtilities::HasClassDefaultObject() const
{
	return DetailsView.HasClassDefaultObject();
}

} // namespace soda