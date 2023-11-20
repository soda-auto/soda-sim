// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "SodaDragDropOp.h"
#include "Engine/Level.h"
#include "GameFramework/Actor.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Layout/SBox.h"
//#include "ClassIconFinder.h"

namespace soda
{

FSodaActorDragDropOp::~FSodaActorDragDropOp()
{
}

TSharedPtr<SWidget> FSodaActorDragDropOp::GetDefaultDecorator() const
{
	return FSodaDecoratedDragDropOp::GetDefaultDecorator();
}

FText FSodaActorDragDropOp::GetDecoratorText() const
{
	return CurrentHoverText;
}

void FSodaActorDragDropOp::Init(TSharedPtr<const FPlaceableItem>& InItem)
{
	MouseCursor = EMouseCursor::GrabHandClosed;
	ThumbnailSize = 32;
	Item = InItem;
}

} // namespace soda