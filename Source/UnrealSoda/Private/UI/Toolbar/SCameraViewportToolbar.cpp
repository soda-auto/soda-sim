// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "SCameraViewportToolbar.h"
#include "Styling/SlateTypes.h"
#include "Framework/Commands/UIAction.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SSpacer.h"
#include "Widgets/Input/SSlider.h"
#include "Widgets/Input/SNumericEntryBox.h"
#include "SodaStyleSet.h"
#include "Soda/UI/SodaViewportCommands.h"
#include "UI/Toolbar/Common/SViewportToolBarMenu.h"
#include "UI/Toolbar/Common/SViewportToolBarButton.h"
#include "Soda/UI/SSodaViewport.h"
#include "Soda/SodaGameViewportClient.h"
#include "Soda/SodaSpectator.h"
#include "Soda/SodaSubsystem.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "RuntimeEditorUtils.h"

#define LOCTEXT_NAMESPACE "TransformToolBar"

namespace soda
{
void SCameraViewportToolbar::Construct( const FArguments& InArgs )
{
	Viewport = InArgs._Viewport;
	CommandList = InArgs._CommandList;

	ChildSlot
	[
		GenerateCameraToolBar(InArgs._Extenders)
	];

	SViewportToolBar::Construct(SViewportToolBar::FArguments());
}

TSharedRef< SWidget > SCameraViewportToolbar::GenerateCameraToolBar(const TSharedPtr< FExtender > InExtenders)
{
	FSlimHorizontalToolBarBuilder ToolbarBuilder(Viewport.Pin()->GetCommandList().ToSharedRef(), FMultiBoxCustomization::None, InExtenders);
	ToolbarBuilder.SetStyle(&FSodaStyle::Get(), "EditorViewportToolBar");
	ToolbarBuilder.SetIsFocusable(false);

	ToolbarBuilder.BeginSection("Projection");
	{
		ToolbarBuilder.AddWidget(
			SNew(SViewportToolbarMenu)
			.ParentToolBar(SharedThis(this))
			.ToolTipText(LOCTEXT("CameraSpeed_ToolTip", "Projection"))
			.LabelIcon(this, &SCameraViewportToolbar::GetProjectionIcon)
			.OnGetMenuContent(this, &SCameraViewportToolbar::GenerateProjectionMenu),
			FName(TEXT("Projection")),
			false
		);
	}
	ToolbarBuilder.EndSection();

	ToolbarBuilder.BeginSection("CameraSpeed");
	{
		ToolbarBuilder.AddWidget(
			SNew(SViewportToolbarMenu)
			.ParentToolBar(SharedThis(this))
			.ToolTipText(LOCTEXT("CameraSpeed_ToolTip", "Camera Speed"))
			.LabelIcon(FSodaStyle::Get().GetBrush("SodaViewport.CamSpeedSetting"))
			.Label(this, &SCameraViewportToolbar::GetCameraSpeedLabel)
			.OnGetMenuContent(this, &SCameraViewportToolbar::FillCameraSpeedMenu),
			FName(TEXT("CameraSpeed")),
			false,
			HAlign_Fill
		);
	}
	ToolbarBuilder.EndSection();

	TSharedRef < SWidget > Widget = FRuntimeEditorUtils::MakeWidget_HackTooltip(ToolbarBuilder);


	return Widget;
}

TSharedRef<SWidget> SCameraViewportToolbar::FillCameraSpeedMenu()
{
	TSharedRef<SWidget> ReturnWidget = SNew(SBorder)
	.BorderImage(FSodaStyle::GetBrush(TEXT("Menu.Background")))
	[
		SNew( SVerticalBox )
		//Camera Speed
		+SVerticalBox::Slot()
		.AutoHeight()
		.Padding( FMargin(8.0f, 2.0f, 60.0f, 2.0f) )
		.HAlign( HAlign_Left )
		[
			SNew( STextBlock )
			.Text( LOCTEXT("MouseSettingsCamSpeed", "Camera Speed")  )
			.Font(FSodaStyle::GetFontStyle( TEXT( "MenuItem.Font" ) ) )
		]
		+SVerticalBox::Slot()
		.AutoHeight()
		.Padding( FMargin(8.0f, 4.0f) )
		[	
			SNew( SHorizontalBox )
			+SHorizontalBox::Slot()
			.FillWidth(1)
			.Padding( FMargin(0.0f, 2.0f) )
			[
				SAssignNew(CamSpeedSlider, SSlider)
				.Value(this, &SCameraViewportToolbar::GetCamSpeedSliderPosition)
				.OnValueChanged(this, &SCameraViewportToolbar::OnSetCamSpeed)
			]
			+SHorizontalBox::Slot()
			.AutoWidth()
			.Padding( 8.0f, 2.0f, 0.0f, 2.0f)
			[
				SNew( STextBlock )
				.Text(this, &SCameraViewportToolbar::GetCameraSpeedLabel )
				.Font(FSodaStyle::GetFontStyle( TEXT( "MenuItem.Font" ) ) )
			]
		] 
	];

	return ReturnWidget;
}

TSharedRef<SWidget> SCameraViewportToolbar::GenerateProjectionMenu() const
{
	const bool bInShouldCloseWindowAfterMenuSelection = true;

	FMenuBuilder CameraMenuBuilder(bInShouldCloseWindowAfterMenuSelection, CommandList);

	CameraMenuBuilder.AddMenuEntry(FSodalViewportCommands::Get().Perspective);

	CameraMenuBuilder.BeginSection("LevelViewportCameraType_Ortho", NSLOCTEXT("BlueprintEditor", "CameraTypeHeader_Ortho", "Orthographic"));
	CameraMenuBuilder.AddMenuEntry(FSodalViewportCommands::Get().OrthographicFree);
	CameraMenuBuilder.AddMenuEntry(FSodalViewportCommands::Get().Top);
	CameraMenuBuilder.AddMenuEntry(FSodalViewportCommands::Get().Bottom);
	CameraMenuBuilder.AddMenuEntry(FSodalViewportCommands::Get().Left);
	CameraMenuBuilder.AddMenuEntry(FSodalViewportCommands::Get().Right);
	CameraMenuBuilder.AddMenuEntry(FSodalViewportCommands::Get().Front);
	CameraMenuBuilder.AddMenuEntry(FSodalViewportCommands::Get().Back);
	CameraMenuBuilder.EndSection();

	return FRuntimeEditorUtils::MakeWidget_HackTooltip(CameraMenuBuilder);
}

FText SCameraViewportToolbar::GetCameraSpeedLabel() const
{
	ESodaSpectatorMode Mode = ESodaSpectatorMode::Perspective;
	USodaSubsystem* SodaSubsystem = USodaSubsystem::Get();
	if (SodaSubsystem && SodaSubsystem->SpectatorActor)
	{
		return FText::AsNumber(SodaSubsystem->SpectatorActor->GetSpeedProfile() + 1);
	}
	return FText::FromString("#");
}

float SCameraViewportToolbar::GetCamSpeedSliderPosition() const
{
	ESodaSpectatorMode Mode = ESodaSpectatorMode::Perspective;
	USodaSubsystem* SodaSubsystem = USodaSubsystem::Get();
	if (SodaSubsystem && SodaSubsystem->SpectatorActor)
	{
		const float MaxValue = SodaSubsystem->SpectatorActor->SpeedProfile.Num();
		return (float)SodaSubsystem->SpectatorActor->GetSpeedProfile() / (MaxValue - 1.f);
	}

	return 0;
}

void SCameraViewportToolbar::OnSetCamSpeed(float NewValue)
{
	ESodaSpectatorMode Mode = ESodaSpectatorMode::Perspective;
	USodaSubsystem* SodaSubsystem = USodaSubsystem::Get();
	if (SodaSubsystem && SodaSubsystem->SpectatorActor)
	{
		const int32 MaxValue = SodaSubsystem->SpectatorActor->SpeedProfile.Num();
		const int32 NewSpeedSetting = NewValue * ((float)MaxValue - 1) + 0.5;
		SodaSubsystem->SpectatorActor->SetSpeedProfile(NewSpeedSetting);
	}
}

const FSlateBrush * SCameraViewportToolbar::GetProjectionIcon() const
{
	ESodaSpectatorMode Mode = ESodaSpectatorMode::Perspective;
	USodaSubsystem* SodaSubsystem = USodaSubsystem::Get();
	if (SodaSubsystem && SodaSubsystem->SpectatorActor)
	{
		Mode = SodaSubsystem->SpectatorActor->GetMode();
	}
	else
	{
		return FSodaStyle::GetBrush(NAME_None);
	}

	static FName PerspectiveIcon("SodaViewport.Perspective");
	static FName TopIcon("SodaViewport.Top");
	static FName LeftIcon("SodaViewport.Left");
	static FName FrontIcon("SodaViewport.Front");
	static FName BottomIcon("SodaViewport.Bottom");
	static FName RightIcon("SodaViewport.Right");
	static FName BackIcon("SodaViewport.Back");

	FName Icon = NAME_None;

	switch (Mode)
	{
	case ESodaSpectatorMode::Perspective:
		Icon = PerspectiveIcon;
		break;

	case ESodaSpectatorMode::OrthographicFree:
		Icon = PerspectiveIcon;
		break;

	case ESodaSpectatorMode::Top:
		Icon = TopIcon;
		break;

	case ESodaSpectatorMode::Left:
		Icon = LeftIcon;
		break;

	case ESodaSpectatorMode::Front:
		Icon = FrontIcon;
		break;

	case ESodaSpectatorMode::Bottom:
		Icon = BottomIcon;
		break;

	case ESodaSpectatorMode::Right:
		Icon = RightIcon;
		break;

	case ESodaSpectatorMode::Back:
		Icon = BackIcon;
		break;
	}

	return FSodaStyle::GetBrush(Icon);
}

} // namespace soda

#undef LOCTEXT_NAMESPACE 
