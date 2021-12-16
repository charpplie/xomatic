/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2009.
-------------------------------------------------------------------------
Description: Perk 'Sonar Vision' functionality
-------------------------------------------------------------------------
History:
- 30:4:2009: Created by Tim Furnish

*************************************************************************/

#include "StdAfx.h"
#include "PerkSonarVision.h"
#include "GameCVars.h"
#include "Player.h"
#include "PerkDbgDisplay.h"

#if 0 && !defined(PROFILE) && !defined(_RELEASE)
#define ENABLE_SONAR_VISION_DEBUGGING 1
#else
#define ENABLE_SONAR_VISION_DEBUGGING 0
#endif

CPerkSonarVisionManager * CPerkSonarVisionManager::s_mgr = NULL;

CPerkSonarVisionObserver::CPerkSonarVisionObserver()
{
	m_largestScaleThisFrame = 0.f;
	m_globalSonarVisorVolumeReduction = 0.f;
}

void CPerkSonarVisionObserver::InformCalculatedSonarVisionScale(float scale)
{
	m_largestScaleThisFrame = max(m_largestScaleThisFrame, min(scale, 1.f));
}

void CPerkSonarVisionObserver::Update(float frameTime, float lowerSpeed, float recoverSpeed)
{
	float difference = m_largestScaleThisFrame - m_globalSonarVisorVolumeReduction;
	float speed = (float) __fsel(difference, lowerSpeed, recoverSpeed);
	difference *= expf(speed * -frameTime);

	m_globalSonarVisorVolumeReduction = m_largestScaleThisFrame - difference;
	m_largestScaleThisFrame = 0.f;
}

CPerkSonarVisionManager::CPerkSonarVisionManager() :
	REGISTER_GAME_MECHANISM(CPerkSonarVisionManager)
{
	assert (s_mgr == NULL);
	s_mgr = this;
}

CPerkSonarVisionManager::~CPerkSonarVisionManager()
{
	assert (s_mgr == this);
	s_mgr = NULL;
}

void CPerkSonarVisionManager::Update(float frameTime)
{
	IActor* pActor = gEnv->pGame->GetIGameFramework()->GetClientActor();
	if(pActor && frameTime > 0.f)
	{
#if ENABLE_SONAR_VISION_DEBUGGING
		float largestScale = m_sonarVisionObserver.GetLargestScaleThisFrame();
#endif

		m_sonarVisionObserver.Update(frameTime, CPerk::GetInstance()->GetVars()->perk_sonarVision_VolumeControlLowerSpeed, CPerk::GetInstance()->GetVars()->perk_sonarVision_VolumeControlRecoverSpeed);

#if ENABLE_SONAR_VISION_DEBUGGING
		PerkDbgDisplay("Sonar vision: volume=%d%%", (int)(0.5f + 100 * (1.f - m_sonarVisionObserver.GetRangeReductionValue())));
#endif
	}
}

void CPerkSonarVisionManager::InformCalculatedSonarVisionScale(float scale)
{
	m_sonarVisionObserver.InformCalculatedSonarVisionScale(scale);
}
