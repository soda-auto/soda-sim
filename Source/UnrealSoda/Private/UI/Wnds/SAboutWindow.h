// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Soda/UI/SMenuWindow.h"

namespace soda
{

class  SAboutWindow : public SMenuWindowContent
{
public:
	SLATE_BEGIN_ARGS(SAboutWindow)
	{}
	SLATE_END_ARGS()
	
	virtual ~SAboutWindow() {}
	void Construct( const FArguments& InArgs);
};

} // namespace