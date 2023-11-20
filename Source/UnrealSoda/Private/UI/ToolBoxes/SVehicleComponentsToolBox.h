// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/WeakInterfacePtr.h"
#include "Soda/UI/SToolBox.h"

class ASodaVehicle;

namespace soda
{
class  SVehicleComponentsToolBox : public SToolBox
{
public:
	SLATE_BEGIN_ARGS(SVehicleComponentsToolBox)
	{ }
	SLATE_END_ARGS()

	void Construct( const FArguments& InArgs, ASodaVehicle* Vehicle);

protected:
	TWeakObjectPtr<ASodaVehicle> Vehicle;
};

} // namespace soda