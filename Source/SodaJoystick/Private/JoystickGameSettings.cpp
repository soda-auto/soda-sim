// Copyright 2015 Moritz Wundke. All Rights Reserved.
// Released under MIT.

#include "JoystickGameSettings.h"

UJoystickGameSettings::UJoystickGameSettings(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer),
	VirtualAxesNum(18),
	VirtualButtonsNum(16)
{
	SetAnalogAxesNum(VirtualAxesNum);
}

FAxisStruct* UJoystickGameSettings::GetAxisSettings(int i)
{
	if (AnalogAxes.Num() > i)
	{
		return &AnalogAxes[i];
	}
	else
	{
		return NULL;
	}
}

void UJoystickGameSettings::SetAnalogAxesNum(int num)
{
	if (num > VirtualAxesNum)
	{
		VirtualAxesNum = num;
	}
	AnalogAxes.SetNum(VirtualAxesNum);
	CurrentAxesValues.SetNum(VirtualAxesNum);
}
