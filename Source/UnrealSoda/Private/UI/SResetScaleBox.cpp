// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "Soda/UI/SResetScaleBox.h"
#include "Soda/SodaGameMode.h"
#include "Soda/SodaApp.h"
#include "Soda/SodaUserSettings.h"
#include "Layout/ArrangedChildren.h"

namespace soda
{

SResetScaleBox::SResetScaleBox()
{
	bHasRelativeLayoutScale = true;
}

float SResetScaleBox::GetRelativeLayoutScale(int32 ChildIndex, float LayoutScaleMultiplier) const
{
	USodaGameModeComponent* GameMode = USodaGameModeComponent::GetChecked();

	CashedScale = 1 / LayoutScaleMultiplier * GameMode->GetWorld()->GetGameViewport()->GetDPIScale() * SodaApp.GetSodaUserSettings()->DPIScale;
	return CashedScale;
}

void SResetScaleBox::OnArrangeChildren(const FGeometry& AllottedGeometry, FArrangedChildren& ArrangedChildren) const 
{
	const EVisibility MyVisibility = this->GetVisibility();
	if (ArrangedChildren.Accepts(MyVisibility))
	{
		ArrangedChildren.AddWidget(AllottedGeometry.MakeChild(
			this->ChildSlot.GetWidget(),
			FVector2D::ZeroVector,
			AllottedGeometry.GetLocalSize() / CashedScale,
			CashedScale
		));
	}
}

FVector2D SResetScaleBox::ComputeDesiredSize(float) const
{
	if (ensure(CashedScale > 0.f))
	{
		return CashedScale * ChildSlot.GetWidget()->GetDesiredSize();
	}
	return ChildSlot.GetWidget()->GetDesiredSize();
}

} // namespace soda