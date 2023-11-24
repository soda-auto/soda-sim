// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "Input/DragAndDrop.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Text/STextBlock.h"
#include "SodaStyleSet.h"

namespace soda
{

struct FPlaceableItem;

/**
 * FSodaDecoratedDragDropOp
 */
class FSodaDecoratedDragDropOp : public FDragDropOperation
{
public:
	DRAG_DROP_OPERATOR_TYPE(FSodaDecoratedDragDropOp, FDragDropOperation)

	/** String to show as hover text */
	FText								CurrentHoverText;

	/** Icon to be displayed */
	const FSlateBrush*					CurrentIconBrush;

	/** The color of the icon to be displayed. */
	FSlateColor							CurrentIconColorAndOpacity;

	FSodaDecoratedDragDropOp()
		: CurrentIconBrush(FSodaStyle::GetBrush(TEXT("Icons.Warning")))
		, CurrentIconColorAndOpacity(FLinearColor::White)
		, DefaultHoverIcon(FSodaStyle::GetBrush(TEXT("Icons.Warning")))
		, DefaultHoverIconColorAndOpacity(FLinearColor::White)
	{ }

	/** Overridden to provide public access */
	virtual void Construct() override
	{
		FDragDropOperation::Construct();
	}

	/** Set the decorator back to the icon and text defined by the default */
	virtual void ResetToDefaultToolTip()
	{
		CurrentHoverText = DefaultHoverText;
		CurrentIconBrush = DefaultHoverIcon;
		CurrentIconColorAndOpacity = DefaultHoverIconColorAndOpacity;
	}

	/** The widget decorator to use */
	virtual TSharedPtr<SWidget> GetDefaultDecorator() const override
	{
		// Create hover widget
		return SNew(SBorder)
			.BorderImage(FSodaStyle::GetBrush("Brushes.Recessed"))
			.Content()
			[			
				SNew(SHorizontalBox)
				
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding( 0.0f, 0.0f, 3.0f, 0.0f )
				.VAlign( VAlign_Center )
				[
					SNew( SImage )
					.Image( this, &FSodaDecoratedDragDropOp::GetIcon )
					.ColorAndOpacity( this, &FSodaDecoratedDragDropOp::GetIconColorAndOpacity)
				]

				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign( VAlign_Center )
				[
					SNew(STextBlock) 
					.Text( this, &FSodaDecoratedDragDropOp::GetHoverText )
				]
			];
	}

	FText GetHoverText() const
	{
		return CurrentHoverText;
	}

	const FSlateBrush* GetIcon( ) const
	{
		return CurrentIconBrush;
	}

	FSlateColor GetIconColorAndOpacity() const
	{
		return CurrentIconColorAndOpacity;
	}

	/** Set the text and icon for this tooltip */
	void SetToolTip(const FText& Text, const FSlateBrush* Icon)
	{
		CurrentHoverText = Text;
		CurrentIconBrush = Icon;
	}

	/** Setup some default values for the decorator */
	void SetupDefaults()
	{
		DefaultHoverText = CurrentHoverText;
		DefaultHoverIcon = CurrentIconBrush;
		DefaultHoverIconColorAndOpacity = CurrentIconColorAndOpacity;
	}

	/** Gets the default hover text for this drag drop op. */
	FText GetDefaultHoverText() const
	{
		return DefaultHoverText;
	}

protected:

	/** Default string to show as hover text */
	FText								DefaultHoverText;

	/** Default icon to be displayed */
	const FSlateBrush*					DefaultHoverIcon;

	/** Default color and opacity for the default icon to be displayed. */
	FSlateColor							DefaultHoverIconColorAndOpacity;
};


/**
 * FSodaActorDragDropOp
 */
class FSodaActorDragDropOp : public FSodaDecoratedDragDropOp
{
public:
	DRAG_DROP_OPERATOR_TYPE(FSodaActorDragDropOp, FSodaDecoratedDragDropOp)

	static TSharedRef<FSodaActorDragDropOp> New(TSharedPtr<const FPlaceableItem> & InItem)
	{
		TSharedRef<FSodaActorDragDropOp> Operation = MakeShared<FSodaActorDragDropOp>();
		Operation->Init(InItem/*, ActorFactory*/);
		Operation->Construct();
		return Operation;
	}

public:
	virtual ~FSodaActorDragDropOp();

	virtual TSharedPtr<SWidget> GetDefaultDecorator() const override;

	FText GetDecoratorText() const;

	TSharedPtr<const FPlaceableItem> GetPlaceableItem() const { return Item; }

protected:
	void Init(TSharedPtr<const FPlaceableItem>& InItem/* TArray<FAssetData> InAssetData, TArray<FString> InAssetPaths , UActorFactory* InActorFactory*/);

protected:
	int32 ThumbnailSize = 0;
	TSharedPtr<const FPlaceableItem> Item;
};

} // namespace soda