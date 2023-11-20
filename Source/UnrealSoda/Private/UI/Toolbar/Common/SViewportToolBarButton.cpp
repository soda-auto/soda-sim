// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "SViewportToolBarButton.h"
#include "Styling/SlateTypes.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "SodaStyleSet.h"

namespace soda
{

void SViewportToolBarButton::Construct( const FArguments& Declaration)
{
	OnClickedDelegate = Declaration._OnClicked;
	IsChecked = Declaration._IsChecked;
	const TSharedRef<SWidget>& ContentSlotWidget = Declaration._Content.Widget;

	bool bContentOverride = ContentSlotWidget != SNullWidget::NullWidget;

	EUserInterfaceActionType ButtonType = Declaration._ButtonType;
	
	// The style of the image to show in the button
	const FName ImageStyleName = Declaration._Image.Get();

	TSharedPtr<SWidget> ButtonWidget;

	if( ButtonType == EUserInterfaceActionType::Button )
	{
		const FSlateBrush* Brush = FSodaStyle::GetBrush( ImageStyleName );

		ButtonWidget =
			SNew( SButton )
			.ButtonStyle(Declaration._ButtonStyle)
			.OnClicked( OnClickedDelegate )
			.HAlign( HAlign_Center )
			.VAlign( VAlign_Center )
			.ContentPadding(FMargin(4.f, 0.f))
			.IsFocusable(false)
			[
				// If we have a content override use it instead of the default image
				bContentOverride
					? ContentSlotWidget
					: TSharedRef<SWidget>( SNew( SImage ) .Image( Brush ) )
			];
	}
	else
	{
		// Cache off checked/unchecked image states
		NormalBrush = FSodaStyle::GetBrush( ImageStyleName, ".Normal" );
		CheckedBrush = FSodaStyle::GetBrush( ImageStyleName, ".Checked" );

		if( CheckedBrush->GetResourceName() == FName("Default") )
		{
			// A different checked brush was not specified so use the normal image when checked
			CheckedBrush = NormalBrush;
		}

		ButtonWidget = 
			SNew( SCheckBox )
			.IsFocusable(false)
			.Style(Declaration._CheckBoxStyle)
			.OnCheckStateChanged( this, &SViewportToolBarButton::OnCheckStateChanged )
			.IsChecked( this, &SViewportToolBarButton::OnIsChecked )
			[
				bContentOverride ? ContentSlotWidget :
				TSharedRef<SWidget>(
					SNew( SBox )
					.Padding(0.0f)
					.VAlign( VAlign_Center )
					.HAlign( HAlign_Center )
					[
						SNew( SImage )
						.Image( this, &SViewportToolBarButton::OnGetButtonImage )
						.ColorAndOpacity(FSlateColor::UseForeground())
					])
			];
	}

	ChildSlot
	[
		ButtonWidget.ToSharedRef()
	];
}

void SViewportToolBarButton::OnCheckStateChanged( ECheckBoxState NewCheckedState )
{
	// When the check state changes (can only happen during clicking in this case) execute our on clicked delegate
	if(OnClickedDelegate.IsBound() == true)
	{
		OnClickedDelegate.Execute();
	}
}

const FSlateBrush* SViewportToolBarButton::OnGetButtonImage() const
{
	return IsChecked.Get() == true ? CheckedBrush : NormalBrush;
}

ECheckBoxState SViewportToolBarButton::OnIsChecked() const
{
	return IsChecked.Get() == true ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}

}