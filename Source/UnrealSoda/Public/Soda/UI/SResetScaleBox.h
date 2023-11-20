// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "Widgets/SCompoundWidget.h"

namespace soda
{

class  SResetScaleBox : public SCompoundWidget
{
public:
	SResetScaleBox();
	virtual float GetRelativeLayoutScale(int32 ChildIndex, float LayoutScaleMultiplier) const override;
	virtual void OnArrangeChildren(const FGeometry& AllottedGeometry, FArrangedChildren& ArrangedChildren) const override;
	virtual FVector2D ComputeDesiredSize(float) const override;

protected:
	mutable float CashedScale = 1;
};

} // namespace soda