/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2007.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Class to handle button regions and navigation on flash menus

-------------------------------------------------------------------------
History:
- 25:09:2009  : Created by Ben Johnson

*************************************************************************/

#include "StdAfx.h"
#include "FlashMenuButtonHandler.h"

#include "IFlashPlayer.h"
#include "IHardwareMouse.h"
#include "GameCodeCoverage/GameCodeCoverageTracker.h"

//------------------------------------------------------------------------
CFlashMenuButtonHandler::CFlashMenuButtonHandler()
{
	CRY_TODO( 28, 09, 2009, "The way buttons are navigated needs further looking at, as it is currently not very flexible. Currently using the navigation used in Crysis 1 menus with a few tweaks for controller input.");

	m_lastButtonSnapTime = 0.f;
	m_lastPressedDeviceId = eDI_Unknown;
}

CFlashMenuButtonHandler::~CFlashMenuButtonHandler()
{
}

//-----------------------------------------------------------------------------------------------------
void CFlashMenuButtonHandler::UpdateButtonSnap(IFlashPlayer *flashPlayer, const Vec2& mouse)
{
	if (!flashPlayer)
		return;

	Vec2 mouseNew(mouse);
	HWMouse2Flash(flashPlayer, mouseNew);
	if(m_currentButtons.empty()) return;
	ButtonPosMap::iterator bestEst = m_currentButtons.end();
	float fBestDist = -1.0f;
	for(ButtonPosMap::iterator it = m_currentButtons.begin(); it != m_currentButtons.end(); ++it)
	{
		Vec2 pos = it->second;
		float dist = (mouseNew-pos).GetLength();
		if(dist<200.0f && (fBestDist<0.0f || dist<fBestDist))
		{
			fBestDist = dist;
			bestEst = it;
		}
	}
	if(bestEst != m_currentButtons.end())
		m_sCurrentButton = bestEst->first;
}

//-----------------------------------------------------------------------------------------------------
void CFlashMenuButtonHandler::SnapToNextButton(IFlashPlayer *flashPlayer, const Vec2& dir)
{
	if(m_currentButtons.empty()) return;
	ButtonPosMap::iterator current = m_currentButtons.find(m_sCurrentButton);

	if(current == m_currentButtons.end())
	{
		current = m_currentButtons.begin();
	}

	// The original implementation with four directions
	Vec2 curPos = current->second;

	ButtonPosMap::iterator bestEst = m_currentButtons.end();
	float fBestValue = -1.0f;
	for(ButtonPosMap::iterator it = m_currentButtons.begin(); it != m_currentButtons.end(); ++it)
	{
		if(it == current) continue;
		Vec2 btndir = it->second - curPos;
		float dist = btndir.GetLength();
		btndir = btndir.GetNormalizedSafe();
		float arc = dir.Dot(btndir);
		if(arc<=0.01) continue;
		float curValue = (dist/arc);
		if(fBestValue<0 || curValue<fBestValue)
		{
			fBestValue = curValue;
			bestEst = it;
			m_sCurrentButton = it->first;
		}
	}

	// Wrap around
	if(bestEst==m_currentButtons.end())
	{
		Vec2 round=dir*-1.0f;
		fBestValue = -1.0f;

		for(ButtonPosMap::iterator it = m_currentButtons.begin(); it != m_currentButtons.end(); ++it)
		{
			if(it == current) continue;
			Vec2 btndir = it->second - curPos;
			float dist = btndir.GetLength();
			btndir = btndir.GetNormalizedSafe();
			float arc = round.Dot(btndir);
			if(arc<=0.01) continue;
			if(dist>fBestValue)
			{
				fBestValue = dist;
				bestEst = it;
				m_sCurrentButton = it->first;
			}
		}
	}

	if(bestEst!=m_currentButtons.end())
	{
		HighlightButton(flashPlayer, bestEst);
		m_lastButtonSnapTime = gEnv->pTimer->GetFrameStartTime(ITimer::ETIMER_UI).GetMilliSeconds();
	}
	else if(current!=m_currentButtons.end())
		HighlightButton(flashPlayer, current);
}
//-----------------------------------------------------------------------------------------------------
void CFlashMenuButtonHandler::GetButtonClientPos(IFlashPlayer *flashPlayer, const ButtonPosMap::iterator& button, Vec2 &pos)
{
	pos = button->second;

	if(!flashPlayer)
		return;

	//IRenderer *pRenderer = gEnv->pRenderer;

	int flashOffsetX = 0, flashOffsetY = 0, flashWidth = 0, flashHeight = 0;
	float flashAspect = 0.f;
	flashPlayer->GetViewport(flashOffsetX, flashOffsetY, flashWidth, flashHeight, flashAspect);

	float movieWidth		= (float)flashPlayer->GetWidth();
	float movieHeight		= (float)flashPlayer->GetHeight();
	float movieRatio		=	movieWidth / movieHeight;

	//float renderWidth		=	(float)pRenderer->GetWidth();
	//float renderHeight	=	(float)pRenderer->GetHeight();
	//float renderRatio	=	renderWidth / renderHeight;

	float renderWidth		=	(float)flashWidth;
	float renderHeight	=	(float)flashHeight;
	float renderRatio	=	renderWidth / renderHeight;

	float offsetX = (float)flashOffsetX;
	float offsetY = (float)flashOffsetY;

	if(renderRatio != movieRatio)
	{
		if(renderRatio < 4.0f / 3.0f)
		{
			float visibleHeight = (renderWidth * 3.0f / 4.0f);
			offsetY = (renderHeight-visibleHeight) * 0.5f;
			renderHeight = visibleHeight;
		}
		offsetX += ( renderWidth - (renderHeight * movieRatio) ) * 0.5f;
	}
	pos*=renderHeight/movieHeight;
	pos.x+=offsetX;
	pos.y+=offsetY;
}

//-----------------------------------------------------------------------------------------------------
void CFlashMenuButtonHandler::HWMouse2Flash(IFlashPlayer *flashPlayer, Vec2 &pos)
{
	IRenderer *pRenderer = gEnv->pRenderer;

	float movieHeight		= (float)flashPlayer->GetHeight();

	float renderWidth		=	(float)pRenderer->GetWidth();
	float renderHeight	=	(float)pRenderer->GetHeight();
	float renderRatio	=	renderWidth / renderHeight;

	if(renderRatio < 4.0f / 3.0f)
	{
		float visibleHeight = (renderWidth * 3.0f / 4.0f);
		renderHeight = visibleHeight;
	}

	pos*=movieHeight/renderHeight;
}

//-----------------------------------------------------------------------------------------------------
void CFlashMenuButtonHandler::HighlightButton(IFlashPlayer *flashPlayer, const ButtonPosMap::iterator& button)
{
	if (flashPlayer)
	{
		Vec2 pos;
		GetButtonClientPos(flashPlayer, button, pos);

		SAFE_HARDWARE_MOUSE_FUNC(SetHardwareMouseClientPosition(pos.x, pos.y));

		SFlashVarValue flashArgs(button->first.c_str());
		flashPlayer->Invoke("highlightButton", &flashArgs, 1);
	}
}

//-----------------------------------------------------------------------------------------------------
void CFlashMenuButtonHandler::PushButton(IFlashPlayer *flashPlayer, const ButtonPosMap::iterator& button, bool press, bool force)
{
	if (button == m_currentButtons.end())
		return;

	Vec2 pos;
	GetButtonClientPos(flashPlayer, button, pos);

	if (!force)
	{
		SAFE_HARDWARE_MOUSE_FUNC(Event((int)pos.x, (int)pos.y, press ? HARDWAREMOUSEEVENT_LBUTTONDOWN : HARDWAREMOUSEEVENT_LBUTTONUP));
	}
	else if (!press)
	{
		string method=button->first;
		method.append(".pressButton");
		if (flashPlayer)
			flashPlayer->Invoke(method, NULL, 0);
	}
}

//-----------------------------------------------------------------------------------------------------
CFlashMenuButtonHandler::ButtonPosMap::iterator CFlashMenuButtonHandler::FindButton(const TKeyName &shortcut)
{
	if(m_currentButtons.empty())
		return m_currentButtons.end();

	// FIXME: Try to find a more elgant way to identify shortcuts
	string sc;
#if defined(PS3)
	if (shortcut == "pad_cross")
#else
	if (shortcut == "xi_a")
#endif
		sc="_a";
#if defined(PS3)
	else if (shortcut == "pad_circle")
#else
	else if (shortcut == "xi_b")
#endif
		sc="_b";
#if defined(PS3)
	else if (shortcut == "pad_triangle")
#else
	else if (shortcut == "xi_x")
#endif
		sc="_x";
#if defined(PS3)
	else if (shortcut == "pad_square")
#else
	else if (shortcut == "xi_y")
#endif
		sc="_y";
	else
		return m_currentButtons.end();


	for(ButtonPosMap::iterator it = m_currentButtons.begin(); it != m_currentButtons.end(); ++it)
	{
		if (it->first.substr(it->first.length()-sc.length(), sc.length()) == sc)
			return it;
	}

	return m_currentButtons.end();
}

//-----------------------------------------------------------------------------------------------------
bool CFlashMenuButtonHandler::CheckSnapToNextButton(const char *direction)
{
	if ((gEnv->pTimer->GetFrameStartTime(ITimer::ETIMER_UI).GetMilliSeconds() - m_lastButtonSnapTime) < 100.f)
		return false;

	// FIXME: Try to find a more elegant way of handling left/right overrides
	if(!m_currentButtons.empty())
	{
		if (!stricmp(direction,"left") || !stricmp(direction,"right"))
		{
			string sc("_lr");

			ButtonPosMap::iterator button = m_currentButtons.find(m_sCurrentButton);
			if (button != m_currentButtons.end())
			{
				if (button->first.substr(button->first.length()-sc.length(), sc.length()) == sc)
					return false;
			}
		}
		if (!stricmp(direction,"up") || !stricmp(direction,"down"))
		{
			string sc("_ud");

			ButtonPosMap::iterator button = m_currentButtons.find(m_sCurrentButton);
			if (button != m_currentButtons.end())
			{
				if (button->first.substr(button->first.length()-sc.length(), sc.length()) == sc)
					return false;
			}
		}
	}
	return true;
}

//-----------------------------------------------------------------------------------------------------
void CFlashMenuButtonHandler::CallbackClear()
{
	CCCPOINT(FrontEnd_MenuButtonCallbackClear);
	m_buttonPositions.clear();
	m_currentButtons.clear();
	m_currentButtonNames.clear();
}

//-----------------------------------------------------------------------------------------------------
void CFlashMenuButtonHandler::CallbackAddOnTop()
{
	m_buttonPositions.push_back(m_currentButtons);
	m_currentButtonNames.push_back(m_sCurrentButton);
	m_currentButtons.clear();
}

//-----------------------------------------------------------------------------------------------------
void CFlashMenuButtonHandler::CallbackRemoveFromTop(IFlashPlayer *flashPlayer)
{
	if(!m_buttonPositions.empty())
	{
		m_currentButtons = m_buttonPositions.back();
		m_buttonPositions.pop_back();

		string currentButtonName = m_currentButtonNames.back();
		m_currentButtonNames.pop_back();

		if (m_currentButtons.size()>=1)
		{
			if ((m_lastPressedDeviceId != eDI_Mouse) && (m_lastPressedDeviceId != eDI_Keyboard)) // If last press was using keyboard/mouse, don't auto snap to buttons
			{
				CFlashMenuButtonHandler::ButtonPosMap::iterator current = m_currentButtons.find(currentButtonName);
				if (current != m_currentButtons.end())
				{
					m_sCurrentButton=current->first;
					HighlightButton(flashPlayer, current);
				}
			}
		}
	}
	else
	{
		m_currentButtons.clear();
	}
}

//-----------------------------------------------------------------------------------------------------
void CFlashMenuButtonHandler::CallbackAddNewButton(IFlashPlayer *flashPlayer, const char *strDefinition)
{
	//CryLog("BtnCallBack %s %s", szCommand, szArgs);

	string sTemp(strDefinition);
	int iSep = sTemp.find("|");
	string sName = sTemp.substr(0,iSep);
	sTemp = sTemp.substr(iSep+1,sTemp.length());
	iSep = sTemp.find("|");
	string sX = sTemp.substr(0,iSep);
	string sY = sTemp.substr(iSep+1,sTemp.length());

	if (m_currentButtons.find(sName) == m_currentButtons.end())
	{
		m_currentButtons.insert(CFlashMenuButtonHandler::ButtonPosMap::iterator::value_type(sName, Vec2((float)atof(sX), (float)atof(sY))));

		// Highlight first button added
		//if (m_iGamepadsConnected && sName.substr(sName.length()-1, 1) == "1")
		if (m_currentButtons.size()==1)
		{
			if ((m_lastPressedDeviceId != eDI_Mouse) && (m_lastPressedDeviceId != eDI_Keyboard)) // If last press was using keyboard/mouse, don't auto snap to buttons
			{
				CFlashMenuButtonHandler::ButtonPosMap::iterator current = m_currentButtons.find(sName);
				m_sCurrentButton=current->first;
				HighlightButton(flashPlayer, current);
			}
		}
	}
}

//-----------------------------------------------------------------------------------------------------
void CFlashMenuButtonHandler::CallbackModifyButton(const char *strDefinition)
{
	//CryLog("BtnModifyCallBack %s %s", szCommand, szArgs);

	string sName(strDefinition);

	ButtonPosMap::iterator button = m_currentButtons.find(sName);
	if (button != m_currentButtons.end())
		m_currentButtons.erase(button);
}

//-----------------------------------------------------------------------------------------------------
void CFlashMenuButtonHandler::SetLastInputDevice(EDeviceId lastInputId)
{
	m_lastPressedDeviceId = lastInputId;
}