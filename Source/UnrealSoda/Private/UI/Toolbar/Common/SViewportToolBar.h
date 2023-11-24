// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineBaseTypes.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"
#include "Animation/CurveSequence.h"

class SMenuAnchor;

namespace soda
{

/**
 * A level viewport toolbar widget that is placed in a viewport
 */
class SViewportToolBar : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS( SViewportToolBar ){}
	SLATE_END_ARGS()

	void Construct( const FArguments& InArgs );

	/**
	 * @return The currently open pull down menu if there is one
	 */
	TWeakPtr<SMenuAnchor> GetOpenMenu() const;

	/**
	 * Sets the open menu to a new menu and closes any currently opened one
	 *
	 * @param NewMenu The new menu that is opened
	 */
	void SetOpenMenu( TSharedPtr< SMenuAnchor >& NewMenu );

private:
	/** The pulldown menu that is open if any */
	TWeakPtr< SMenuAnchor > OpenedMenu;
	/** True if the mouse is inside the toolbar */
	bool bIsHovered;
};

}

