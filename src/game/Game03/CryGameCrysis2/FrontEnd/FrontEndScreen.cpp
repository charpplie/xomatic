#include "StdAfx.h"

#include "FrontEnd.h"
#include "FrontEndElement.h"
#include "FrontEndScreen.h"

#include "IRenderer.h"

//////////////////////////////////////////////////////////////////////////


CFrontEndScreen::CFrontEndScreen() :
m_name(""),
m_currentElementIndex(-1),
m_backgroundTexture(NULL),
m_fadingOutTarget(-1),
m_fading(false)
{
	m_elements.resize(0);
}



CFrontEndScreen::~CFrontEndScreen()
{
	if(m_backgroundTexture)
		gEnv->pRenderer->RemoveTexture(m_backgroundTexture->GetTextureID());

	while(m_elements.size()>0)
	{
		CFrontEndElement* pElement = m_elements.back(); m_elements.pop_back();
		SAFE_DELETE(pElement);
	}
}



void CFrontEndScreen::Initialize(XmlNodeRef screen)
{
	
	m_currentElementIndex = -1;

	for(int attr = 0; attr < screen->getNumAttributes(); ++attr)		
	{
		const char *key, *value;
		if(screen->getAttributeByIndex(attr, &key, &value))
		{
			if(!stricmp(key, "name"))
				m_name = string(value);
			else if(!stricmp(key, "background"))
				SetBackgroundTexture(value);
		}
	}

	for(int i = 0; i < screen->getChildCount(); ++i)
	{
		XmlNodeRef element = screen->getChild(i);
		if(!stricmp(element->getTag(),"Button"))
		{
			CFrontEndButton* pButton = new CFrontEndButton();
			pButton->Initialize(element);
			m_elements.push_back(pButton);
		}
		else if(!stricmp(element->getTag(),"Pane"))
		{
			CFrontEndPane* pPane = new CFrontEndPane();
			pPane->Initialize(element);
			m_elements.push_back(pPane);
		}
		else if(!stricmp(element->getTag(),"Slider"))
		{
			CFrontEndSlider* pSlider = new CFrontEndSlider();
			pSlider->Initialize(element);
			m_elements.push_back(pSlider);
		}
		else if(!stricmp(element->getTag(),"Switch"))
		{
			CFrontEndSwitch* pSwitch = new CFrontEndSwitch();
			pSwitch->Initialize(element);
			m_elements.push_back(pSwitch);
		}
		else if(!stricmp(element->getTag(),"ProgressBar"))
		{
			CFrontEndProgressBar* pBar = new CFrontEndProgressBar();
			pBar->Initialize(element);
			m_elements.push_back(pBar);
		}
		else if(!stricmp(element->getTag(),"RandomLoadingMessage"))
		{
			CFrontEndRandomLoadingMessage* pBar = new CFrontEndRandomLoadingMessage();
			pBar->Initialize(element);
			m_elements.push_back(pBar);
		}
	}

	bool found = false;
	Elements::const_iterator it = m_elements.begin();
	Elements::const_iterator end = m_elements.end();
	for(; it!=end; ++it)
	{
		++m_currentElementIndex;
		if((*it)->IsTabActive())
		{
			CFrontEndInteractive* pCurrent = static_cast<CFrontEndInteractive*>(*it);
			if(pCurrent && pCurrent->GetEnabled())
			{
				pCurrent->SetActive(true);
				found = true;
				break;
			}
		}
	}
	if(!found)
		m_currentElementIndex = -1;
}



CFrontEndInteractive* CFrontEndScreen::GetCurrentActiveElement()
{
	int size = m_elements.size();
	if(!size)
		return NULL;

	if(m_currentElementIndex<0 || m_currentElementIndex>=size)
		return NULL;

	return static_cast<CFrontEndInteractive*>(m_elements[m_currentElementIndex]);
}



bool CFrontEndScreen::Input(int input, int state)
{
	if(m_fadingOutTarget != -1)
		return false;

	bool handled = false;
	if(state==eFEIS_Press)
	{
		if(input==eFEIT_Up)
		{
			NextElement(false);
			handled = true;
		}
		else if(input==eFEIT_Down)
		{
			NextElement(true);
			handled = true;
		}
	}
	if(!handled)
	{
		CFrontEndInteractive* pCurrent = GetCurrentActiveElement();
		if(pCurrent)
			handled = pCurrent->Input(input, state);
	}
	return handled;
}



void CFrontEndScreen::NextElement(bool forward)
{
	CFrontEndInteractive* pOld = GetCurrentActiveElement();
	if(!pOld)
		return;

	int border = m_elements.size();
	int tries = 0;
	while(tries<int(m_elements.size()))
	{
		++tries;
		m_currentElementIndex += forward?1:-1;

		if(m_currentElementIndex>=border)
			m_currentElementIndex = 0;

		if(m_currentElementIndex<0)
			m_currentElementIndex = border-1;

		CFrontEndElement* pCurrent = m_elements[m_currentElementIndex];
		if(pCurrent->IsTabActive())
		{
			CFrontEndInteractive* pInteractive = static_cast<CFrontEndInteractive*>(pCurrent);
			if(pInteractive && pInteractive->GetEnabled())
			{
				pOld->SetActive(false);
				pInteractive->SetActive(true);
				CFrontEnd::Get()->PlaySound(eFES_Move,true);
				break;
			}
		}
	}
}



void CFrontEndScreen::SetBackgroundTexture(const char* texture)
{
	if(!texture || !texture[0])
		return;
	m_backgroundTexture = gEnv->pRenderer->EF_LoadTexture(texture, FT_DONT_STREAM|FT_DONT_RESIZE,eTT_2D);
}



void CFrontEndScreen::Fade(bool fadeOut, int targetIndex)
{
	Elements::const_iterator it = m_elements.begin();
	Elements::const_iterator end = m_elements.end();
	for(; it!=end; ++it)
	{
		(*it)->Fade(fadeOut);
	}
	m_fadingOutTarget = targetIndex;
}



void CFrontEndScreen::Update(float frameTime)
{
	bool fadeInProgress = (m_fadingOutTarget != -1);
	SetFading(false);

	Elements::const_iterator it = m_elements.begin();
	Elements::const_iterator end = m_elements.end();
	for(; it!=end; ++it)
	{
		CFrontEndElement* pElement = (*it);
		pElement->Update(frameTime);
		if(pElement->IsFading())
		{
			SetFading(true);
		}
	}

	if(fadeInProgress && !IsFading())
	{
		CFrontEnd::Get()->SetCurrentScreen(m_fadingOutTarget, frameTime);
		m_fadingOutTarget = -1;
	}
}



void CFrontEndScreen::Render()
{
	if(m_backgroundTexture)
	{
		float renderW = (float)gEnv->pRenderer->GetWidth();
		float renderH = (float)gEnv->pRenderer->GetHeight();
		CFrontEnd::Get()->DrawTexture(m_backgroundTexture, 0.0f, 0.0f, renderW, renderH);
	}

	Elements::const_iterator it = m_elements.begin();
	Elements::const_iterator end = m_elements.end();
	for(; it!=end; ++it)
	{
		(*it)->Render();
	}
}



void CFrontEndScreen::GetMemoryStatistics(ICrySizer * s)
{
	s->Add(*this);
}


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////


CFrontEndScreenStart::CFrontEndScreenStart()
{
}



CFrontEndScreenStart::~CFrontEndScreenStart()
{
}



bool CFrontEndScreenStart::Input(int input, int state)
{
	bool handled = false;
	if(state==eFEIS_Press)
	{
		if(input==eFEIT_Start)
		{
			CFrontEnd::Get()->PlaySound(eFES_Start,true);
			CFrontEndInteractive* pCurrent = GetCurrentActiveElement();
			if(pCurrent)
				handled = pCurrent->Input(eFEIT_Ok, state);
		}
	}
	return handled;
}


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////


CFrontEndScreenLoading::CFrontEndScreenLoading()
: m_wasCommandExecuted(false)
{
}



CFrontEndScreenLoading::~CFrontEndScreenLoading()
{
}



void CFrontEndScreenLoading::Update(float frameTime)
{
	CFrontEndScreen::Update(frameTime);
	if(!IsFading() && !m_wasCommandExecuted)
	{
		CFrontEnd::Get()->Execute(eFECMD_new_game, "");
		m_wasCommandExecuted = true;
	}
}


//////////////////////////////////////////////////////////////////////////
