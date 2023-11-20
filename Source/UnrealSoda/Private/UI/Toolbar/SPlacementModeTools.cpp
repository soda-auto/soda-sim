// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "SPlacementModeTools.h"
#include "Application/SlateApplicationBase.h"
#include "Widgets/Layout/SSpacer.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SScrollBar.h"
#include "Widgets/Views/SListView.h"
#include "Widgets/Input/SCheckBox.h"
#include "SodaStyleSet.h"
#include "Widgets/Input/SSearchBox.h"
//#include "ClassIconFinder.h"
//#include "Widgets/Docking/SDockTab.h"
//#include "ThumbnailRendering/ThumbnailManager.h"
//#include "ActorFactories/ActorFactory.h"
#include "Framework/Application/SlateApplication.h"
#include "Styling/SlateIconFinder.h"

#include "SodaDragDropOp.h"

#define LOCTEXT_NAMESPACE "PlacementMode"


namespace soda
{

void SPlacementAssetMenuEntry::Construct(const FArguments& InArgs, const TSharedPtr<const FPlaceableItem>& InItem)
{	
	bIsPressed = false;

	check(InItem.IsValid());

	Item = InItem;

	AssetImage = nullptr;

	TSharedPtr< SHorizontalBox > ActorType = SNew( SHorizontalBox );

	//const bool bIsClass = Item->AssetData.GetClass() == UClass::StaticClass();
	//const bool bIsActor = bIsClass ? CastChecked<UClass>(Item->AssetData.GetAsset())->IsChildOf(AActor::StaticClass()) : false;

	//AActor* DefaultActor = nullptr;
	/*
	if (Item->Factory != nullptr)
	{
		DefaultActor = Item->Factory->GetDefaultActor(Item->AssetData);
	}
	else
	
	if (bIsActor)
	{
		DefaultActor = CastChecked<AActor>(CastChecked<UClass>(Item->AssetData.GetAsset())->ClassDefaultObject);
	}
	*/

	UClass* DocClass = nullptr;
	TSharedPtr<IToolTip> AssetEntryToolTip;
	if(Item->DefaultActor != nullptr)
	{
		//DocClass = DefaultActor->GetClass();
		//AssetEntryToolTip = FEditorClassUtils::GetTooltip(DefaultActor->GetClass());
	}

	if (!AssetEntryToolTip.IsValid())
	{
		AssetEntryToolTip = FSlateApplicationBase::Get().MakeToolTip(FText::FromName(Item->DisplayName));
	}
	
	const FButtonStyle& ButtonStyle = FAppStyle::Get().GetWidgetStyle<FButtonStyle>( "Menu.Button" );
	const float MenuIconSize = FAppStyle::Get().GetFloat("Menu.MenuIconSize");

	Style = &ButtonStyle;

	// Create doc link widget if there is a class to link to
	TSharedRef<SWidget> DocWidget = SNew(SSpacer);


	ChildSlot
	.HAlign(HAlign_Fill)
	.VAlign(VAlign_Fill)
	[
		SNew(SBorder)
		.BorderImage( this, &SPlacementAssetMenuEntry::GetBorder )
		.Cursor( EMouseCursor::GrabHand )
		.ToolTip( AssetEntryToolTip )
		.Padding(FMargin(27.f, 3.f, 5.f, 3.f))
		[
			SNew( SHorizontalBox )

			+ SHorizontalBox::Slot()
			.Padding(14.0f, 0.f, 10.f, 0.0f)
			.AutoWidth()
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			[
				SNew(SBox)
				.WidthOverride(MenuIconSize)
				.HeightOverride(MenuIconSize)
				[
					SNew(SImage)
					.Image(this, &SPlacementAssetMenuEntry::GetIcon)
					.ColorAndOpacity(FSlateColor::UseSubduedForeground())
				]
			]

			+ SHorizontalBox::Slot()
			.FillWidth(1.f)
			.Padding(1.f, 0.f, 0.f, 0.f)
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Left)
			[

				SNew( STextBlock )
				.ColorAndOpacity(FSlateColor::UseForeground())
				.Text(FText::FromName(Item->DisplayName) )
			]


			+ SHorizontalBox::Slot()
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Right)
			.AutoWidth()
			[
				SNew(SImage)
                .ColorAndOpacity(FSlateColor::UseSubduedForeground())
				.Image(FSodaStyle::Get().GetBrush("Icons.DragHandle"))
			]
		]
	];
}


FReply SPlacementAssetMenuEntry::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if ( MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton )
	{
		bIsPressed = true;

		return FReply::Handled().DetectDrag( SharedThis( this ), MouseEvent.GetEffectingButton() );
	}

	return FReply::Unhandled();
}

FReply SPlacementAssetMenuEntry::OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if ( MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton )
	{
		bIsPressed = false;

		AActor* NewActor = nullptr;

		/*
		FTransform TempTransform;
		//UActorFactory* Factory = Item->Factory;
		if (!Item->Factory)
		{
			// If no actor factory was found or failed, add the actor from the uclass
			UObject* ClassObject = Item->AssetData.GetClass()->GetDefaultObject();
			FActorFactoryAssetProxy::GetFactoryForAssetObject(ClassObject);
		}
		NewActor = FLevelEditorActionCallbacks::AddActor(Factory, Item->AssetData, &TempTransform);
		if (NewActor && GCurrentLevelEditingViewportClient)
		{
  			GEditor->MoveActorInFrontOfCamera(*NewActor, 
  				GCurrentLevelEditingViewportClient->GetViewLocation(), 
  				GCurrentLevelEditingViewportClient->GetViewRotation().Vector()
  			);
		}
		*/

		FSlateApplication::Get().DismissAllMenus();

		return FReply::Handled();
	}

	return FReply::Unhandled();
}

FReply SPlacementAssetMenuEntry::OnDragDetected(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	bIsPressed = false;

	/*
	if (FEditorDelegates::OnAssetDragStarted.IsBound())
	{
		TArray<FAssetData> DraggedAssetDatas;
		DraggedAssetDatas.Add( Item->AssetData );
		FEditorDelegates::OnAssetDragStarted.Broadcast( DraggedAssetDatas, Item->Factory );
		return FReply::Handled();
	}
	*/

	if( MouseEvent.IsMouseButtonDown( EKeys::LeftMouseButton ) )
	{
		FSlateApplication::Get().DismissAllMenus();
		return FReply::Handled().BeginDragDrop(FSodaActorDragDropOp::New(Item/*, Item->Factory*/));
	}
	else
	{
		FSlateApplication::Get().DismissAllMenus();
		return FReply::Handled();
	}
}

bool SPlacementAssetMenuEntry::IsPressed() const
{
	return bIsPressed;
}

const FSlateBrush* SPlacementAssetMenuEntry::GetBorder() const
{
	if ( IsPressed() )
	{
		return &(Style->Pressed);
	}
	else if ( IsHovered() )
	{
		return &(Style->Hovered);
	}
	else
	{
		return &(Style->Normal);
	}
}

FSlateColor SPlacementAssetMenuEntry::GetForegroundColor() const
{
	if (IsPressed())
	{
		return Style->PressedForeground;
	}
	else if (IsHovered())
	{
		return Style->HoveredForeground;
	}
	else
	{
		return Style->NormalForeground;
	}
}



} // namespace soda

#undef LOCTEXT_NAMESPACE