/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2005.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: 

-------------------------------------------------------------------------
History:
- 18:08:2005: Created by Julien Darre

*************************************************************************/
#ifndef __GAMEFLASHLOGIC_H__
#define __GAMEFLASHLOGIC_H__

//-----------------------------------------------------------------------------------------------------

#include "GameFlashAnimation.h"
#include "IGameTokens.h"

//-----------------------------------------------------------------------------------------------------

#define HUD_MAX_STRING_SIZE 256

//-----------------------------------------------------------------------------------------------------

class CGameFlashLogic : public IGameTokenEventListener
{
public:
	// IGameTokenEventListener
	virtual void OnGameTokenEvent( EGameTokenEvent event,IGameToken *pGameToken );
	// ~IGameTokenEventListener

	void Init(const char *strASControl,const char *strVariable,const char *strToken,float fScale,float fOffset,CGameFlashAnimation *pGameFlashAnimation = NULL);
	void ReInit();

		CGameFlashLogic();
		CGameFlashLogic(CGameFlashAnimation *pGameFlashAnimation);
	~	CGameFlashLogic();


	const char* GetToken() const {	return m_token.c_str();	}
	void GetMemoryUsage(class ICrySizer *pSizer) const; 

private:

	CGameFlashAnimation*	m_pGameFlashAnimation;

	CryFixedStringT<32> m_token;
	CryFixedStringT<32> m_invoke;

	float m_fScale;
	float m_fOffset;
};

//-----------------------------------------------------------------------------------------------------

inline void ConvertWString(const wstring& s, string& d)
{
  d.resize(wstring::_strlen(s));
  char* dst = d.begin();
  const wchar_t* src = s;

  while (char c=(char)(*src++ & 0xFF))
  {
    *dst++ = c;
  }
}

//-----------------------------------------------------------------------------------------------------

#endif

//-----------------------------------------------------------------------------------------------------