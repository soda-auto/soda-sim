// © 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include "Soda/Vehicles/SodaVehicle.h"
#include "GameFramework/HUD.h"
#include "SodaHUD.generated.h"


UCLASS(ClassGroup = Soda, meta = (BlueprintSpawnableComponent))
class UNREALSODA_API ASodaHUD : public AHUD
{
	GENERATED_UCLASS_BODY()

public:
	bool bDrawVehicleDebugPanel = true;

public:
	virtual void DrawHUD() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	
};
