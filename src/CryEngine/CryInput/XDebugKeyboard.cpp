// XDebugKeyboard.cpp
//
//////////////////////////////////////////////////////////////////////


#include "StdAfx.h"
#include <ILog.h>
#include <stdio.h>

#ifdef DEBUG_KEYBOARD

#include "XDebugKeyboard.h"

#define		TOGGLE_CAPSLOCK			(1 << 0)
#define		TOGGLE_NUMLOCK			(1 << 1)
#define		TOGGLE_SCROLLLOCK		(1 << 2)

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
CXDebugKeyboard::CXDebugKeyboard(CXenonInput& input) : CXenonInputDevice(input, "keyboard")
{
  memset(m_cKeysState, 0, sizeof(m_cKeysState));
	memset(m_cOldKeysState, 0, sizeof(m_cOldKeysState));
  memset(m_bKeys, 0, sizeof(m_bKeys));
  m_hDebugKeyboard = 0;
	m_iToggleState = 0;
	m_modifiers = 0;
}

CXDebugKeyboard::~CXDebugKeyboard()
{

}

bool CXDebugKeyboard::Init()
{
	//Acquire();	

	return true;
}

//////////////////////////////////////////////////////////////////////////
void CXDebugKeyboard::SetExclusive(bool value,void *pUser)
{ 

}

//////////////////////////////////////////////////////////////////////////
void CXDebugKeyboard::Update(bool bFocus)
{
  //BOOL bReturn;
  DWORD dwInsertions = 0, dwRemovals = 0;
  DWORD dwReturn;




















































































































































































































































































































}

//////////////////////////////////////////////////////////////////////////
void CXDebugKeyboard::PressKey( int key,float val )
{
	if (m_cKeysState[key] == 0)
	{
		m_cKeysState[key] = 1;
		SInputEvent event;
		event.type = KEY_PRESS;
		event.keyId = key;
		event.value = val;
		event.keyName = GetKeyName( event );
		m_pInput->PostInputEvent( event );

		CryLogAlways( "KeyPress  %s, value=%f",event.keyname,event.value );
	}
}

//////////////////////////////////////////////////////////////////////////
void CXDebugKeyboard::ReleaseKey( int key )
{
	if (m_cKeysState[key])
	{
		m_cKeysState[key] = 0;
		SInputEvent event;
		event.type = KEY_RELEASE;
		event.key = key;
		event.keyname = m_pInput->GetKeyName( event );
		m_pInput->PostInputEvent( event );
	}
}

//////////////////////////////////////////////////////////////////////////
void CXDebugKeyboard::ShutDown()
{
}

//////////////////////////////////////////////////////////////////////////
const char *CXDebugKeyboard::EnumerateKey(int nKeyCode)
{
	SInputEvent event;
	event.key = nKeyCode;

	return GetKeyName(event);
}

//////////////////////////////////////////////////////////////////////////
void CXDebugKeyboard::ClearKeyState()
{
	memset(m_cKeysState,0,sizeof(m_cKeysState));
}

//////////////////////////////////////////////////////////////////////////
unsigned char CXDebugKeyboard::XKEY2ASCII(unsigned short nCode,int modifiers)
{
	if (nCode >= XKEY_0 && nCode <= XKEY_9)
		return '0' + (nCode-XKEY_0);

	if (nCode >= XKEY_A && nCode <= XKEY_Z)
	{
		bool bCaps = m_iToggleState & TOGGLE_CAPSLOCK;
		if (modifiers & (XKEY_MOD_LSHIFT|XKEY_MOD_RSHIFT))
			bCaps = !bCaps;

		if (bCaps)
			return 'A' + (nCode-XKEY_A);
		else
			return 'a' + (nCode-XKEY_A);
	}
	return 0;
}

//////////////////////////////////////////////////////////////////////////
bool CXDebugKeyboard::GetOSKeyName(int nKey, wchar_t *szwKeyName, int iBufSize)
{
	SInputEvent event;
	event.key = nKey;
	event.modifiers = m_modifiers;

	static wchar_t sName[1];
	const char *s = m_pInput->GetKeyName(event);
	sName[0] = s[0];
  szwKeyName = sName;
	return true;
}


#endif // USE_DEBUGKEYBOARD