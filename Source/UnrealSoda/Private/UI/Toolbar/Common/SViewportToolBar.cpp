// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "SViewportToolBar.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/Input/SMenuAnchor.h"
#include "SodaStyleSet.h"

#define LOCTEXT_NAMESPACE "ViewportToolBar"

namespace soda
{

void SViewportToolBar::Construct( const FArguments& InArgs )
{
	bIsHovered = false;
}

TWeakPtr<SMenuAnchor> SViewportToolBar::GetOpenMenu() const
{
	return OpenedMenu;
}

void SViewportToolBar::SetOpenMenu( TSharedPtr< SMenuAnchor >& NewMenu )
{
	if( OpenedMenu.IsValid() && OpenedMenu.Pin() != NewMenu )
	{
		// Close any other open menus
		OpenedMenu.Pin()->SetIsOpen( false );
	}
	OpenedMenu = NewMenu;
}


} // namespace soda

#undef LOCTEXT_NAMESPACE
