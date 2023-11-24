#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "IInputDeviceModule.h"

//Joystick Plugin log
DECLARE_LOG_CATEGORY_EXTERN(SodaJoystick, Log, All);

class SODAJOYSTICK_API ISodaJoystickPlugin : public IInputDeviceModule
{
public:
/**
	 * Singleton-like access to this module's interface.  This is just for convenience!
	 * Beware of calling this during the shutdown phase, though.  Your module might have been unloaded already.
	 *
	 * @return Returns singleton instance, loading the module on demand if needed
	 */
	static inline ISodaJoystickPlugin& Get()
	{
		return FModuleManager::LoadModuleChecked< ISodaJoystickPlugin >( "SodaJoystick" );
	}

	/**
	 * Checks to see if this module is loaded and ready.  It is only valid to call Get() if IsAvailable() returns true.
	 *
	 * @return True if the module is loaded and ready to use
	 */
	static inline bool IsAvailable()
	{
		return FModuleManager::Get().IsModuleLoaded( "SodaJoystick" );
	}

	virtual void RotateToPosition(float position) = 0; // -1.f to 1.f
	virtual void ApplyConstantForce(int ForceVal) = 0; // -32768 to 32767
	virtual void StopConstantForceEffect() = 0;
	virtual void ApplyBumpEffect(int ForceVal, int Len) = 0; // -32768 to 32767
	virtual int GetJoyNum() const = 0;
	virtual void Reset() = 0;
	virtual void UpdateSettings() = 0;
};
