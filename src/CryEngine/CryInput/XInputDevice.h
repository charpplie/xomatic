#ifndef __XINPUTDEVICE_H__
#define __XINPUTDEVICE_H__
#pragma once

#include"InputDevice.h"

#if defined(USE_DXINPUT) || defined(USE_XENONINPUT)

#if defined(USE_DXINPUT)
#include <xinput.h>
#pragma comment(lib, "xinput.lib")


#endif

struct ICVar;

/*
Device: 'XBOX 360 For Windows (Controller)' - 'XBOX 360 For Windows (Controller)'
Product GUID:  {028E045E-0000-0000-0000-504944564944}
Instance GUID: {65429170-3725-11DA-8001-444553540000}
*/

class CXInputDevice : public CInputDevice
{
public:
	CXInputDevice(IInput& pInput, int deviceNo);
	virtual ~CXInputDevice();

	// IInputDevice overrides
	virtual bool Init();
	virtual void Update(bool bFocus);
	virtual void ClearKeyState();
	virtual void ClearAnalogKeyState();
	virtual bool SetForceFeedback(IFFParams params);
	virtual bool IsOfDeviceType( EInputDeviceType type ) const { return type == eIDT_Gamepad && m_connected; }
	virtual void SetDeadZone(float fThreshold);
	virtual void RestoreDefaultDeadZone();
	// ~IInputDevice overrides

private:
	void UpdateConnectedState(bool isConnected);
	//void AddInputItem(const IInput::InputItem& item);

	//triggers the speed of the vibration motors -> leftMotor is for low frequencies, the right one is for high frequencies
	bool SetVibration(USHORT leftMotor = 0, USHORT rightMotor = 0, float timing = 0, EFFEffectId effectId = eFF_Rumble_Basic);
	void ProcessAnalogStick(SInputSymbol* pSymbol, SHORT prev, SHORT current, SHORT threshold);

	ILINE float GetClampedLeftMotorAccumulatedVibration() const { return clamp(m_basicLeftMotorRumble + m_frameLeftMotorRumble, 0.0f, 65535.0f); }
	ILINE float GetClampedRightMotorAccumulatedVibration() const { return clamp(m_basicRightMotorRumble + m_frameRightMotorRumble, 0.0f, 65535.0f); }

	int							m_deviceNo; //!< device number (from 0 to 3) for this XInput device
	bool						m_connected;
	XINPUT_STATE		m_state;

	float						m_basicLeftMotorRumble, m_basicRightMotorRumble;
	float						m_frameLeftMotorRumble, m_frameRightMotorRumble;
	float						m_fVibrationTimer;
	float						m_fDeadZone;
	//std::vector<IInput::InputItem>	mInputQueue;	//!< queued inputs

public:
	static GUID		ProductGUID;
};

#endif //defined(USE_DXINPUT) || defined(USE_XENONINPUT)

#endif //XINPUTDEVICE_H__
