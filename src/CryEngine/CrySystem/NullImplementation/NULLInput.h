#ifndef NULL_INPUT_H
#define NULL_INPUT_H

#pragma once
#include <IInput.h>

class CNullInput : public IInput
{
public:
	virtual void AddEventListener( IInputEventListener *pListener ) {}
	virtual void RemoveEventListener( IInputEventListener *pListener ) {}

	virtual void AddConsoleEventListener( IInputEventListener *pListener ) {}
	virtual void RemoveConsoleEventListener( IInputEventListener *pListener ) {}

	virtual void SetExclusiveListener( IInputEventListener *pListener ) {}
	virtual IInputEventListener *GetExclusiveListener()  { return NULL; }

	virtual bool AddInputDevice(IInputDevice* pDevice) { return false; }

	virtual void EnableEventPosting ( bool bEnable ) {}
	virtual void PostInputEvent( const SInputEvent &event ) {}

	virtual void ForceFeedbackEvent( const SFFOutputEvent &event ) {}

	virtual bool	Init() { return true; }
	virtual void	Update(bool bFocus) {}
	virtual void	ShutDown() {}

	virtual void	SetExclusiveMode(EDeviceId deviceId, bool exclusive,void *hwnd=0) {}

	virtual bool	InputState(const TKeyName& key, EInputState state) { return false; }

	virtual const char *GetKeyName(const SInputEvent& event, bool bGUI = 0) { return NULL; }

	virtual SInputSymbol* LookupSymbol( EDeviceId deviceId, EKeyId keyId ) { return NULL; }

	virtual const wchar_t* GetOSKeyName(const SInputEvent& event) { return NULL; }

	virtual void ClearKeyState() {}

	virtual void ClearAnalogKeyState() {}

	virtual void RetriggerKeyState() {}

	virtual bool Retriggering() { return false; }

	virtual bool HasInputDeviceOfType( EInputDeviceType type ) { return false; }

	virtual int GetModifiers() const { return 0; }

	virtual void EnableDevice( EDeviceId deviceId, bool enable) {}

	virtual void SetDeadZone(float fThreshold) {};

	virtual void RestoreDefaultDeadZone() {};

	//Diesel new 3203
		virtual uint32 GetPlatformFlags() const { return 0; }
};

#endif
