// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "SMenuWindow.h"

namespace soda
{

enum class  EMessageBoxType : uint8
{
	OK = 0,
	OK_CANCEL = 1,
	YES_NO = 2,
	YES_NO_CANCEL = 3
};

enum class  EMessageBoxButton : uint8
{
	OK = 0,
	CANCEL = 1,
	YES = 2,
	NO = 3
};

DECLARE_DELEGATE_OneParam(FOnMessageBox, EMessageBoxButton);

class  SMessageBox: public SMenuWindow
{
public:
	SLATE_BEGIN_ARGS(SMessageBox)
	{}
		SLATE_ATTRIBUTE(FText, Caption)
		SLATE_ATTRIBUTE(FText, TextContent)
		SLATE_ATTRIBUTE(EMessageBoxType, Type)
		SLATE_EVENT(FOnMessageBox, OnMessageBox)
		SLATE_EVENT(FSimpleDelegate, OnClose)
	SLATE_END_ARGS()
	
	SMessageBox();
	virtual ~SMessageBox();

	void Construct( const FArguments& InArgs );
	void SetOnMessageBox(FOnMessageBox InOnMessageBox);

protected:
	void ExecuteOnMessageBox(EMessageBoxButton Button);
	FOnMessageBox OnMessageBox;

};

} // namespace soda