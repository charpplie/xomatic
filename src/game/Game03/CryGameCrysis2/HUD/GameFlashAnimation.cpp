/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2005.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description:

-------------------------------------------------------------------------
History:
- 07:11:2005: Created by Julien Darre

*************************************************************************/
#include "StdAfx.h"
#include "GameFlashAnimation.h"
#include "GameFlashLogic.h"
#include "IFlashPlayer.h"
#include "HUD.h"
#include "Game.h"

//-----------------------------------------------------------------------------------------------------

CGameFlashAnimation::CGameFlashAnimation()
{
	m_flags = 0;
	m_preAllocatedFlashLogics = NULL;
	m_preAllocatedSize = 0;
}

//-----------------------------------------------------------------------------------------------------

CGameFlashAnimation::~CGameFlashAnimation()
{
	// deregister us from the HUD Updates and Rendering
	SAFE_HUD_FUNC(Remove(this));

	bool deleteNow = true;
	for(TGameFlashLogicsList::iterator iter=m_gameFlashLogicsList.begin(); iter!=m_gameFlashLogicsList.end(); ++iter)
	{
		deleteNow = true;
		for(int i = 0; i < m_preAllocatedSize; ++i)
		{
			if(&(m_preAllocatedFlashLogics[i]) == *iter)
			{
				deleteNow = false;
				break;
			}
		}
		if(deleteNow)
			SAFE_DELETE(*iter);
	}

	delete []m_preAllocatedFlashLogics;
}

//-----------------------------------------------------------------------------------------------------
uint32 CGameFlashAnimation::GetFlags() const
{
	return m_flags;
}
//-----------------------------------------------------------------------------------------------------

void CGameFlashAnimation::Init(const char *strFileName, EFlashDock docking, uint32 flags, int preAllocate)
{
	Unload();
	m_fileName = strFileName;
	SetDock(docking);
	m_flags = flags;

	if(preAllocate > 0 && !m_preAllocatedFlashLogics)
	{
		m_preAllocatedFlashLogics = new CGameFlashLogic[preAllocate];
		m_preAllocatedIndex = 0;
		m_preAllocatedSize = preAllocate;
	}
}
//-----------------------------------------------------------------------------------------------------
bool CGameFlashAnimation::Reload(bool forceUnload)
{
	if (forceUnload)
		Unload();

	if (!m_fileName.empty() && !IsLoaded())
	{
		if (LoadAnimation(m_fileName.c_str()))
		{
			IRenderer *pRenderer = gEnv->pRenderer;
			GetFlashPlayer()->SetViewport(0,0,pRenderer->GetWidth(),pRenderer->GetHeight());
			GetFlashPlayer()->SetBackgroundAlpha(0.0f);

			SAFE_HUD_FUNC(Register(this));

			if (m_flags & eFAF_ThisHandler)
				GetFlashPlayer()->SetFSCommandHandler(/*g_pGame?g_pGame->GetHUD():*/NULL);

			SAFE_HUD_FUNC(RepositionFlashAnimation(this));
			SAFE_HUD_FUNC(SetFlashColor(this));

			if (!(m_flags & eFAF_Visible))
				SetVisible(false);

			return true;
		}
	}

	return false;
}

//-----------------------------------------------------------------------------------------------------

bool CGameFlashAnimation::Load(const char *strFileName, EFlashDock docking, uint32 flags, int preAllocate)
{
	Init(strFileName, docking, flags, preAllocate);
	return Reload();
}

void CGameFlashAnimation::Unload()
{
	// early out
	if (!IsLoaded())
		return;

	CFlashAnimation::Unload();

	// remove us from the HUD updates and rendering
	SAFE_HUD_FUNC(Remove(this));
}

//-----------------------------------------------------------------------------------------------------

void CGameFlashAnimation::AddVariable(const char *strControl,const char *strVariable,const char *strToken,float fScale,float fOffset)
{
	// make sure we don't already have this variable
	TGameFlashLogicsList::const_iterator endIt = m_gameFlashLogicsList.end();
	for (TGameFlashLogicsList::const_iterator i = m_gameFlashLogicsList.begin(); i != endIt; ++i)
	{
		if (!stricmp((*i)->GetToken(), strToken))
			return;
	}

	// it's unique ... so we create and add it
	CGameFlashLogic *pGFVariable = NULL;
	if(m_preAllocatedFlashLogics && m_preAllocatedIndex < m_preAllocatedSize)
	{
		pGFVariable = &(m_preAllocatedFlashLogics[m_preAllocatedIndex]);
		m_preAllocatedIndex++;
	}
	else
		pGFVariable = new CGameFlashLogic(this);
	pGFVariable->Init(strControl, strVariable, strToken, fScale, fOffset, this);
	m_gameFlashLogicsList.push_back(pGFVariable);
}

//-----------------------------------------------------------------------------------------------------

void CGameFlashAnimation::ReInitVariables()
{
	TGameFlashLogicsList::const_iterator endIt = m_gameFlashLogicsList.end();
	for (TGameFlashLogicsList::const_iterator i = m_gameFlashLogicsList.begin(); i != endIt; ++i)
	{
		(*i)->ReInit();
	}
}

//-----------------------------------------------------------------------------------------------------

void CGameFlashAnimation::GetMemoryStatistics(ICrySizer * s) const
{
	s->AddObject(m_gameFlashLogicsList);
	for (TGameFlashLogicsList::const_iterator iter = m_gameFlashLogicsList.begin(); iter != m_gameFlashLogicsList.end(); ++iter)
	{
		(*iter)->GetMemoryUsage(s);
	}
}
