// XDebugKeyboard.h
// XBox Debug Keyboard
//
//////////////////////////////////////////////////////////////////////

#ifndef __XDEBUGKEYBOARD_H__
#define __XDEBUGKEYBOARD_H__
#pragma once

#include "XenonInputDevice.h"

struct ILog;

#define KEYFLAG_BUFFERSIZE 32

class CXenonInput;

class CXDebugKeyboard : public CXenonInputDevice
{
public:
  // IInputDevice overrides
  virtual bool Init();
  virtual void Update(bool bFocus);
  virtual const char *GetKeyName(const SInputEvent& event, bool bGUI);
  // ~IInputDevice

	CXDebugKeyboard(CXenonInput& input);
	virtual ~CXDebugKeyboard();

////////////////////////////////////////////////////////////
	void ShutDown();
	//! return the name (key name list) of the key specified in kKeyCode
	const char *EnumerateKey(int nKeyCode);
	//! set/unset directinput to exclusive mode
	void SetExclusive(bool value,void *pUser);
	//! clear the key (pressed) state
	void ClearKeyState();
	//! return the name (ascii name mapped to the virtual keyboard layout) of the key specified in kKeyCode
	//const char *GetKeyName(unsigned short nKey);
	//!
/////////////////////////////////////////////////////////////

public:
	unsigned char XKEY2ASCII(unsigned short nCode,int modifiers);
	bool GetOSKeyName(int nKey, wchar_t *szwKeyName, int iBufSize);
	int GetModifiers() const { return m_modifiers; };

	void PressKey( int key,float val=1.0f );
	void ReleaseKey( int key );
	
protected:
	//bool Acquire();
	//bool UnAcquire();
	unsigned char	m_cKeysState[0x0000011F];
	unsigned char	m_cOldKeysState[0x0000011F];	
  
private:
	ILog *m_pLog;

  IInput * m_pInput;

  HANDLE m_hDebugKeyboard;
  bool m_bKeys[256];

	int m_iToggleState;
	int m_modifiers;
};

#endif // __XDEBUGKEYBOARD_H__
