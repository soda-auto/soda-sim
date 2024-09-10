// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Soda/UI/SMenuWindow.h"

namespace soda
{

class  SPakWindow : public SMenuWindowContent
{
public:
	SLATE_BEGIN_ARGS(SPakWindow)
	{}
	SLATE_END_ARGS()
	
	virtual ~SPakWindow() {}
	void Construct( const FArguments& InArgs);
};

} // namespace