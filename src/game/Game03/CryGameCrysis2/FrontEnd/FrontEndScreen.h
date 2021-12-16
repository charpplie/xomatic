#ifndef __FRONTENDSCREEN_H__
#define __FRONTENDSCREEN_H__

//////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"

#include "FrontEnd.h"


//////////////////////////////////////////////////////////////////////////


class CFrontEndElement;
class CFrontEndInteractive;


//////////////////////////////////////////////////////////////////////////


class CFrontEndScreen
{
public:

	void GetMemoryStatistics( ICrySizer * );

						CFrontEndScreen();
	virtual ~	CFrontEndScreen();

	virtual void Update(float frameTime);
	void Render();

	void Fade(bool fadeOut, int fadeTarget = -1);

	void Initialize(XmlNodeRef screen);
	void SetBackgroundTexture(const char* texture);

	const char* GetName() { return m_name.c_str(); }
	
	CFrontEndInteractive* GetCurrentActiveElement();
	virtual bool Input(int input, int state);

	void				SetFading(bool fading) { m_fading = fading; }
	const bool	IsFading() { return m_fading; }

private:

	void NextElement(bool forward);

	typedef std::vector<CFrontEndElement*> Elements;
	string		m_name;
	int				m_fadingOutTarget;
	ITexture*	m_backgroundTexture;
	Elements	m_elements;
	int				m_currentElementIndex;
	bool			m_fading;
};


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////


class CFrontEndScreenStart : public CFrontEndScreen
{
public:

	void GetMemoryStatistics( ICrySizer * );

	CFrontEndScreenStart();
	virtual ~	CFrontEndScreenStart();

	virtual bool Input(int input, int state);

private:

};


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////


class CFrontEndScreenLoading : public CFrontEndScreen
{
public:

	void GetMemoryStatistics( ICrySizer * );

	CFrontEndScreenLoading();
	virtual ~	CFrontEndScreenLoading();

	virtual void Update(float frameTime);

private:
	bool m_wasCommandExecuted;
};


//////////////////////////////////////////////////////////////////////////

#endif
