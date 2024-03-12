// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#include "SPinWidget.h"
#include "Widgets/Views/STableRow.h"

namespace soda
{

void SPinWidget::Construct(const FArguments& InArgs)
{
	CheckedHoveredBrush = InArgs._CheckedHoveredBrush;
	CheckedNotHoveredBrush = InArgs._CheckedNotHoveredBrush;
	NotCheckedHoveredBrush = InArgs._NotCheckedHoveredBrush;
	NotCheckedNotHoveredBrush = InArgs._NotCheckedNotHoveredBrush;
	OnClickedDelegate = InArgs._OnClicked;
	IsChecked = InArgs._IsChecked;
	Row = InArgs._Row;
	bHideUnchecked = InArgs._bHideUnchecked;

	SImage::Construct(
		SImage::FArguments()
		.IsEnabled(this, &SPinWidget::IsEnabled)
		.ColorAndOpacity(this, &SPinWidget::GetForegroundColor)
		.Image(this, &SPinWidget::GetBrush)
	);
}

void SPinWidget::SetRow(TWeakPtr < ITableRow> InRow)
{
	check(InRow.IsValid());
	Row = InRow;
}

bool SPinWidget::IsEnabled() const 
{ 
	return true; 
}

const FSlateBrush* SPinWidget::GetBrush() const
{
	if (IsChecked.IsBound() && IsChecked.Get())
	{
		return IsHovered() ? CheckedHoveredBrush.Get() : CheckedNotHoveredBrush.Get();
	}
	else
	{
		return IsHovered() ? NotCheckedHoveredBrush.Get() : NotCheckedNotHoveredBrush.Get();
	}
}

FSlateColor SPinWidget::GetForegroundColor() const
{
	const bool bIsSelected = Row.IsValid() ? Row.Pin()->IsItemSelected() : false;
	const bool bIsChecked = IsChecked.IsBound() && IsChecked.Get();

	if (bHideUnchecked)
	{
		if (IsHovered())
		{
			if (bIsSelected)
			{
				return bIsChecked ? FAppStyle::Get().GetSlateColor("Colors.ForegroundHover") : FSlateColor(FLinearColor(1, 1, 1, 0.3));
			}
			else
			{
				return bIsChecked ? FAppStyle::Get().GetSlateColor("Colors.ForegroundHover") : FAppStyle::Get().GetSlateColor("Colors.Hover");
			}
		}
		else
		{
			return bIsChecked ? FSlateColor::UseForeground() : FLinearColor::Transparent;
		}		
	}
	else
	{

		// make the foreground brush transparent if it is not selected and it is visible
		/*
		if (bIsChecked && !IsHovered() && !bIsSelected)
		{
			return FLinearColor::Transparent;
		}
		else
		*/
		if (IsHovered() && !bIsSelected)
		{
			return FAppStyle::Get().GetSlateColor("Colors.ForegroundHover");
		}
	}

	return FSlateColor::UseForeground();
}

FReply SPinWidget::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (MouseEvent.GetEffectingButton() != EKeys::LeftMouseButton)
	{
		return FReply::Unhandled();
	}

	if (IsEnabled())
	{
		if (OnClickedDelegate.IsBound() == true)
		{
			OnClickedDelegate.Execute();
		}
		return FReply::Handled();
		
	}

	return FReply::Unhandled();
}

FReply SPinWidget::OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		return FReply::Handled();
	}

	return FReply::Unhandled();
}

FReply SPinWidget::OnMouseButtonDoubleClick(const FGeometry& InMyGeometry, const FPointerEvent& InMouseEvent)
{
	return FReply::Handled();
}


} // namespace soda
