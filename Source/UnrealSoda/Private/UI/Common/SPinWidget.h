// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/Images/SImage.h"
#include "SodaStyleSet.h"
#include "Templates/SharedPointer.h"
#include "Framework/SlateDelegates.h"

class ITableRow;

namespace soda
{

class SPinWidget : public SImage
{
public:
	SLATE_BEGIN_ARGS(SPinWidget)
		: _CheckedHoveredBrush(FSodaStyle::Get().GetBrush(TEXT("Level.VisibleHighlightIcon16x")))
		, _CheckedNotHoveredBrush(FSodaStyle::Get().GetBrush(TEXT("Level.VisibleIcon16x")))
		, _NotCheckedHoveredBrush(FSodaStyle::Get().GetBrush(TEXT("Level.NotVisibleHighlightIcon16x")))
		, _NotCheckedNotHoveredBrush(FSodaStyle::Get().GetBrush(TEXT("Level.NotVisibleIcon16x")))
		, _IsChecked(false)
		, _bHideUnchecked(false)
	{}
		SLATE_ATTRIBUTE(const FSlateBrush*, CheckedHoveredBrush)
		SLATE_ATTRIBUTE(const FSlateBrush*, CheckedNotHoveredBrush)
		SLATE_ATTRIBUTE(const FSlateBrush*, NotCheckedHoveredBrush)
		SLATE_ATTRIBUTE(const FSlateBrush*, NotCheckedNotHoveredBrush)
		SLATE_ATTRIBUTE(bool, IsChecked)
		SLATE_ARGUMENT(TWeakPtr < ITableRow>, Row)
		SLATE_ARGUMENT(bool, bHideUnchecked)
		SLATE_EVENT(FOnClicked, OnClicked)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	void SetRow(TWeakPtr < ITableRow> InRow);

protected:
	virtual bool IsEnabled() const;
	virtual const FSlateBrush* GetBrush() const;
	virtual FSlateColor GetForegroundColor() const;

	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;

protected:
	bool bHideUnchecked = false;
	TWeakPtr < ITableRow> Row;
	FOnClicked OnClickedDelegate;
	TAttribute<bool> IsChecked;
	TAttribute<const FSlateBrush*> CheckedHoveredBrush;
	TAttribute<const FSlateBrush*> CheckedNotHoveredBrush;
	TAttribute<const FSlateBrush*> NotCheckedHoveredBrush;
	TAttribute<const FSlateBrush*> NotCheckedNotHoveredBrush;
};

} // namespace soda