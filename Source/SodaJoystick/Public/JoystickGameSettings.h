#pragma once


#include "UAxisStruct.h"
#include "JoystickGameSettings.generated.h"

/**
 * Setting object used to hold both config settings and editable ones in one place
 * To ensure the settings are saved to the specified config file make sure to add
 * props using the globalconfig or config meta.
 */
UCLASS(config = SodaUserSettings)
class SODAJOYSTICK_API UJoystickGameSettings : public UObject
{
	GENERATED_BODY()

public:
	UJoystickGameSettings(const FObjectInitializer& ObjectInitializer);

	/** Or add min, max or clamp values to the settings */
	UPROPERTY(config, EditAnywhere, BlueprintReadWrite, Category = Custom, meta = (UIMin = 0, ClampMin = 0, UIMax = 100, ClampMax = 100, EditInRuntime))
	int32 CentringForcePercent;

	UPROPERTY(config, EditAnywhere, BlueprintReadWrite, Category = Custom, meta = (UIMin = -32768, ClampMin = -32768, UIMax = 32767, ClampMax = 32767, EditInRuntime))
	int32 RotatingForce;

	UPROPERTY(config, EditAnywhere, BlueprintReadWrite, Category = Custom, meta = (UIMin = 0, ClampMin = 0, UIMax = 32, ClampMax = 32, EditInRuntime))
	int32 VirtualAxesNum;

	UPROPERTY(config, EditAnywhere, BlueprintReadWrite, Category = Custom, meta = (UIMin = 0, ClampMin = 0, UIMax = 32, ClampMax = 32, EditInRuntime))
	int32 VirtualButtonsNum;

	UPROPERTY(config, EditAnywhere, BlueprintReadWrite, EditFixedSize, Category = Custom, meta = (EditInRuntime))
	TArray<FAxisStruct> AnalogAxes;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, EditFixedSize, Category = Custom, meta = (EditInRuntime))
	TArray<float> CurrentAxesValues;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, EditFixedSize, Category = Custom, meta = (EditInRuntime))
	FText LastButtonDown;

	void SetAnalogAxesNum(int num);
	int GetAnalogAxesNum() { return AnalogAxes.Num(); }

	void SetCurrentAxesValuesNum(int NewNum)
	{
		CurrentAxesValues.SetNum(NewNum);
	}

	FAxisStruct* GetAxisSettings(int i);
};
