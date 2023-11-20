// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Soda/UI/SResetScaleBox.h"

namespace soda
{


/**
 * SWaitingPanel
 */
class UNREALSODA_API SWaitingPanel : public SResetScaleBox
{
public:
	SLATE_BEGIN_ARGS(SWaitingPanel)
	{}
		SLATE_ATTRIBUTE(FText, Caption)
		SLATE_ATTRIBUTE(FText, SubCaption)
	SLATE_END_ARGS()

	void Construct( const FArguments& InArgs );

	virtual bool CloseWindow();
};

} // namespace soda