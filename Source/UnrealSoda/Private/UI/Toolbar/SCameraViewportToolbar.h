// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SWidget.h"
#include "Textures/SlateIcon.h"
#include "UI/Toolbar/Common/SViewportToolBar.h"

class FExtender;
class FUICommandList;
class SSlider;

namespace soda
{
class SSodaViewport;
class STransformViewportToolBar;
struct FProjectionComboEntry;


class SCameraViewportToolbar : public SViewportToolBar
{

public:
	SLATE_BEGIN_ARGS(SCameraViewportToolbar){}
		SLATE_ARGUMENT( TSharedPtr<SSodaViewport>, Viewport )
		SLATE_ARGUMENT( TSharedPtr<FUICommandList>, CommandList )
		SLATE_ARGUMENT( TSharedPtr<FExtender>, Extenders )
	SLATE_END_ARGS()

	void Construct( const FArguments& InArgs );

private:
	TSharedRef<SWidget> GenerateCameraToolBar(const TSharedPtr< FExtender > InExtenders);
	TSharedRef<SWidget> FillCameraSpeedMenu();
	FText GetCameraSpeedLabel() const;
	float GetCamSpeedSliderPosition() const;
	void OnSetCamSpeed(float NewValue);

	TSharedRef<SWidget> GenerateProjectionMenu() const;
	const FSlateBrush * GetProjectionIcon() const;

private:
	TSharedPtr<SSlider> CamSpeedSlider;
	TWeakPtr<soda::SSodaViewport> Viewport;
	TSharedPtr<FUICommandList> CommandList;
};

} // namespace soda
