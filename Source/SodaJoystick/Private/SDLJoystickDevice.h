#pragma once

#include "IInputDevice.h"
#include "InputCoreTypes.h"
#include <vector>
#include "UAxisStruct.h"
#include "JoystickGameSettings.h"
#if defined(_MSC_VER)
#	pragma warning( push )
#	pragma warning(disable: 4668)
#endif
#include "ThirdParty/SDL2/SDL-gui-backend/include/SDL.h"
#if defined(_MSC_VER)
#	pragma warning( pop )
#endif

DECLARE_LOG_CATEGORY_EXTERN(SodaSDLJoystickDevice, Log, All);

class FSDLJoystickDevice : public IInputDevice
{
	struct UAxis
	{
		FKey Key;
		int DeviceIndex;
		int Index;
		FString Name;
		FAxisStruct* SettingsStruct;

		UAxis(FString NewName = "", int NewDeviceIndex = 0, int NewIndex = 0) : 
			Key(*NewName),
			DeviceIndex(NewDeviceIndex),
			Index(NewIndex),
			Name(NewName),
			SettingsStruct(NULL)
		{};
	};

public:
	FSDLJoystickDevice(const TSharedRef<FGenericApplicationMessageHandler>& InMessageHandler);
	~FSDLJoystickDevice();

	/** Tick the interface (e.g. check for new controllers) */
	virtual void Tick(float DeltaTime) override;

	/** Poll for controller state and send events if needed */
	virtual void SendControllerEvents() override;

	/** Set which MessageHandler will get the events from SendControllerEvents. */
	virtual void SetMessageHandler(const TSharedRef< FGenericApplicationMessageHandler >& InMessageHandler) override;

	/** Exec handler to allow console commands to be passed through for debugging */
	virtual bool Exec(UWorld* InWorld, const TCHAR* Cmd, FOutputDevice& Ar) override;

	/** IForceFeedbackSystem pass through functions **/
	virtual void SetChannelValue(int32 ControllerId, FForceFeedbackChannelType ChannelType, float Value) override;
	virtual void SetChannelValues(int32 ControllerId, const FForceFeedbackValues &values) override;

	void SetCentringForce(int percent); // Set power of CentringForce effect in percent (0 to 100). Set 0 to disable CentringForce effect.

	void RotateToPosition(float position); // Rotate steering wheel to given position (-1.f to 1.f) applying Constant Force effect. Call on each frame.

	void ApplyConstantForce(int ForceVal); // -32768 to 32767
	void StopConstantForceEffect();

	void ApplyBumpEffect(int ForceVal, int Len); // -32768 to 32767

	void CloseDevice();

	void ActualizeAxesNum();
	void ActualizeButtonsNum();

	int GetJoyNum() const { return Joys.Num(); }

private:
	bool SDLStartup();
	void UpdateEKeys(int NumAxes, int NumButtons);

private:
	/* Message handler */
	TSharedRef<FGenericApplicationMessageHandler> MessageHandler;

	TArray<SDL_Joystick *> Joys;

	TArray < SDL_Haptic *> Haptics;

	SDL_HapticEffect EffectConstant;
	SDL_HapticEffect EffectBump;
	TArray<int> EffectConstantIds;
	TArray<int> EffectBumpIds;

	float CentringForceMax = 0;

	int AxesNum = 0;
	int ButtonsNum = 0;

	std::vector <UAxis> Buttons;
	std::vector <UAxis> AnalogAxes;

	UJoystickGameSettings* Settings;

	float NoJoyWaitTimer = 0;
	const float NoJoyWaitTime = 1.f;

	int RegisteredAxesNum = 0;
	int RegisteredButtonsNum = 0;
};