// Copyright Epic Games, Inc. All Rights Reserved.
// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"

class SButton;

namespace soda
{

class SDetailTableRowBase;

class SDetailExpanderArrow : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SDetailExpanderArrow) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, TSharedRef<SDetailTableRowBase> DetailsRow);

private:

	EVisibility GetExpanderVisibility() const;
	const FSlateBrush* GetExpanderImage() const;
	FReply OnExpanderClicked();
	
private:
	TWeakPtr<SDetailTableRowBase> Row;
	TSharedPtr<SButton> ExpanderArrow;
};

} // namespace soda