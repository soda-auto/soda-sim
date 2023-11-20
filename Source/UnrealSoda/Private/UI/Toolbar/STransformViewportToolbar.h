// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/Toolbar/Common/SViewportToolBar.h"
#include "Textures/SlateIcon.h"

class FExtender;
class FUICommandList;
class SSlider;

namespace soda
{

class SSodaViewport;
class SViewportToolbarMenu;

class STransformViewportToolBar : public SViewportToolBar
{

public:
	SLATE_BEGIN_ARGS( STransformViewportToolBar ){}
		SLATE_ARGUMENT( TSharedPtr<SSodaViewport>, Viewport )
		SLATE_ARGUMENT( TSharedPtr<FUICommandList>, CommandList )
		SLATE_ARGUMENT( TSharedPtr<FExtender>, Extenders )
	SLATE_END_ARGS()

	void Construct( const FArguments& InArgs );

private:
	TSharedRef<SWidget> MakeTransformToolBar(const TSharedPtr< FExtender > InExtenders);
	FSlateIcon GetLocalToWorldIcon() const;
	FSlateIcon GetSurfaceSnappingIcon() const;
	FReply OnCycleCoordinateSystem();

private:
	TWeakPtr<soda::SSodaViewport> Viewport;
	TSharedPtr<FUICommandList> CommandList;
};

} // namespace soda
