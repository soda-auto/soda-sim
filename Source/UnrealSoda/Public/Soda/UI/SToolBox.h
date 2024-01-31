// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

namespace soda
{


class UNREALSODA_API SToolBox : public SCompoundWidget
{
public:
	
	SLATE_BEGIN_ARGS(SToolBox)
	{}
		SLATE_DEFAULT_SLOT(FArguments, Content)
		SLATE_ATTRIBUTE(FText, Caption)
	SLATE_END_ARGS()
	
	//SToolBox();
	//virtual ~SToolBox();

	void Construct( const FArguments& InArgs );

	const FText& GetCaption() const { return Caption; }

	virtual void OnPush() {};
	virtual void OnPop() {};
	
protected:
	FText Caption;
};

} // namespace soda