// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Soda/UI/SMenuWindow.h"

class ASodaVehicle;

namespace soda
{

class  SSaveVehicleRequestWindow : public SMenuWindowContent
{
public:
	SLATE_BEGIN_ARGS(SSaveVehicleRequestWindow)
	{}
	SLATE_END_ARGS()
	
	virtual ~SSaveVehicleRequestWindow() {}
	void Construct( const FArguments& InArgs, TWeakObjectPtr<ASodaVehicle> Vehicle);

protected:
	FReply OnResave();
	FReply OnSaveAs();
	FReply OnCancel();

	TWeakObjectPtr<ASodaVehicle> Vehicle;
};


} // namespace