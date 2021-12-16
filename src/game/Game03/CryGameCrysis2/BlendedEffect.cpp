/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2008.
-------------------------------------------------------------------------
History:
- 23:1:2008   Created by Benito G.R. - Refactor'd from John N. ScreenEffects.h/.cpp

*************************************************************************/

#include "StdAfx.h"
#include "BlendedEffect.h"

#include "Player.h"

//-------------FOV EFFECT-------------------------------

CFOVEffect::CFOVEffect(float goalFOV)
{
	m_goalFOV = goalFOV;
}

//---------------------------------
void CFOVEffect::Init()
{
	//TODO: Do not modify actor params this way...
	IActor *pClientActor = gEnv->pGame->GetIGameFramework()->GetClientActor();
	if (pClientActor)
	{
		CPlayer *pPlayer = (CPlayer *)pClientActor;
		SActorParams *pParams = pPlayer->GetActorParams();
		m_startFOV = pParams->viewFoVScale;
		m_currentFOV = m_startFOV;
	}
}

//---------------------------------
void CFOVEffect::Update(float point)
{
	m_currentFOV = (point * (m_goalFOV - m_startFOV)) + m_startFOV;
	IActor *pClientActor = gEnv->pGame->GetIGameFramework()->GetClientActor();
	if (pClientActor)
	{
		CPlayer *pPlayer = (CPlayer *)pClientActor;
		SActorParams *pParams = pPlayer->GetActorParams();
		pParams->viewFoVScale = m_currentFOV;
	}
}

//-------------------POST PROCESS FX--------------------

CPostProcessEffect::CPostProcessEffect(string paramName, float goalVal)
{
	m_paramName = paramName;
	m_goalVal = goalVal;
}

//---------------------------------
void CPostProcessEffect::Init()
{
	gEnv->p3DEngine->GetPostEffectParam(m_paramName, m_currentVal);
	m_startVal = m_currentVal;
}

//---------------------------------
void CPostProcessEffect::Update(float point)
{
	m_currentVal = (point * (m_goalVal - m_startVal)) + m_startVal;
	gEnv->p3DEngine->SetPostEffectParam(m_paramName, m_currentVal);
}