// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Soda/UI/SResetScaleBox.h"

namespace soda
{

class SMenuWindowContent;

/**
 * SMenuWindow
 */
class UNREALSODA_API SMenuWindow : public SResetScaleBox
{
public:
	SLATE_BEGIN_ARGS(SMenuWindow)
	{}
		SLATE_ARGUMENT(TSharedPtr<SMenuWindowContent>, Content);
		SLATE_ATTRIBUTE(FText, Caption)
	SLATE_END_ARGS()
	
	SMenuWindow();
	virtual ~SMenuWindow();

	void Construct( const FArguments& InArgs );

	virtual bool CloseWindow();

protected:
	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override { return FReply::Handled(); }
	virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override { return FReply::Handled(); }
	virtual FReply OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override { return FReply::Handled(); }
	virtual FReply OnMouseWheel(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override { return FReply::Handled(); }
};

/**
 * SMenuWindowContent
 */
class UNREALSODA_API SMenuWindowContent : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SMenuWindowContent)
	{}
		SLATE_DEFAULT_SLOT(FArguments, Content)
	SLATE_END_ARGS()
	
	//SMenuWindowContent() {}
	//virtual ~SMenuWindowContent() {}

	void Construct( const FArguments& InArgs )
	{
		ChildSlot
		[
			InArgs._Content.Widget
		];
	}

	void SetParentMenuWindow(TSharedPtr<SMenuWindow> InParentMenuWindow)
	{
		ParentMenuWindow = InParentMenuWindow;
	}

	bool CloseWindow()
	{
		if (ParentMenuWindow.IsValid())
		{
			return ParentMenuWindow.Pin()->CloseWindow();
		}
		return false;
	}

private:
	TWeakPtr<SMenuWindow> ParentMenuWindow = nullptr;
};

} // namespace soda