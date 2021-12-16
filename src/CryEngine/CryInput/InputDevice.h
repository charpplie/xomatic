/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2006.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description:	Some useful and shared functionality for input devices.
-------------------------------------------------------------------------
History:
- Dec 16,2005:	Created by Marco Koegler

*************************************************************************/
#ifndef __INPUTDEVICE_H__
#define __INPUTDEVICE_H__
#pragma once

#include <IInput.h>

class CInputDevice : public IInputDevice
{
public:
	CInputDevice(IInput& input, const char* deviceName);
	virtual ~CInputDevice();

	// IInputDevice
	virtual const char* GetDeviceName() const	{	return	m_deviceName.c_str();	}
	virtual EDeviceId GetDeviceId() const	{ return m_deviceId; };
	virtual bool	Init()	{	return true;	}
	virtual void	Update(bool bFocus);
	virtual bool	SetForceFeedback(IFFParams params){	return false; };
	virtual bool	InputState(const TKeyName& key, EInputState state);
	virtual bool	SetExclusiveMode(bool value)	{ return true;	}
	virtual void	ClearKeyState();
	virtual void	ClearAnalogKeyState();
	virtual const char *GetKeyName(const SInputEvent& event, bool bGUI=0);
	virtual const wchar_t *GetOSKeyName(const SInputEvent& event);
	virtual SInputSymbol* LookupSymbol(EKeyId id) const;
	virtual void Enable(bool enable);
	virtual bool IsEnabled() const {	return m_enabled;	}
	virtual void OnLanguageChange() {};
	virtual void SetDeadZone(float fThreshold) {};
	virtual void RestoreDefaultDeadZone() {};
	// ~IInputDevice


protected:
	IInput& GetIInput() const	{	return	m_input;	}

	// device dependent id management
	//const TKeyName&	IdToName(TKeyId id) const;
	SInputSymbol*		IdToSymbol(EKeyId id) const;
	uint32					NameToId(const TKeyName& name) const;
	SInputSymbol*		NameToSymbol(const TKeyName& name) const;
	SInputSymbol*		DevSpecIdToSymbol(uint32 devSpecId) const;
	SInputSymbol*		MapSymbol(uint32 deviceSpecificId, EKeyId keyId, const TKeyName& name, SInputSymbol::EType type = SInputSymbol::Button, uint32 user = 0);

protected:
	EDeviceId                 m_deviceId;
	bool											m_enabled;
private:
	IInput&										m_input;				// point to input system in use
	string										m_deviceName;		// name of the device (used for input binding)

	typedef std::map<TKeyName, uint32>				TNameToIdMap;
	typedef std::map<TKeyName, SInputSymbol*>	TNameToSymbolMap;
	typedef std::map<EKeyId, SInputSymbol*>		TIdToSymbolMap;
	typedef std::map<uint32, SInputSymbol*>		TDevSpecIdToSymbolMap;

	TNameToIdMap			m_nameToId;
	TNameToSymbolMap	m_nameToInfo;
	TIdToSymbolMap		m_idToInfo;
	TDevSpecIdToSymbolMap	m_devSpecIdToSymbol;
};

#endif //__INPUTDEVICE_H__
