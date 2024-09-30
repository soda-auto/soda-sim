// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Soda/UI/SMenuWindow.h"

namespace soda
{

class  SQuickStartWindow : public SMenuWindowContent
{
public:
	SLATE_BEGIN_ARGS(SQuickStartWindow)
	{}
	SLATE_END_ARGS()
	
	virtual ~SQuickStartWindow() {}
	void Construct( const FArguments& InArgs);
};

} // namespace