/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2009.
-------------------------------------------------------------------------
Description: Perk 'Sonar Vision' functionality
-------------------------------------------------------------------------
History:
- 30:4:2009: Created by Tim Furnish

*************************************************************************/

#ifndef __PERK_SONAR_VISION_H__
#define __PERK_SONAR_VISION_H__

#include "GameMechanismManager/GameMechanismBase.h"

class CPerkSonarVisionObserver
{
	public:
	CPerkSonarVisionObserver ();

	void								InformCalculatedSonarVisionScale (float scale);
	void								Update (float frameTime, float lowerSpeed, float recoverSpeed);

	ILINE float				GetRangeReductionValue()
	{
		return m_globalSonarVisorVolumeReduction;
	}

	ILINE float				GetLargestScaleThisFrame()
	{
		return m_largestScaleThisFrame;
	}

	private:
	float								m_largestScaleThisFrame;
	float								m_globalSonarVisorVolumeReduction;
};

class CPerkSonarVisionManager : public CGameMechanismBase
{
	public:
	CPerkSonarVisionManager ();
	~CPerkSonarVisionManager ();

	void								InformCalculatedSonarVisionScale (float scale);
	
	static ILINE CPerkSonarVisionManager	* GetMgr()
	{
		return s_mgr;
	}

	ILINE CPerkSonarVisionObserver * GetObserver()
	{
		return & m_sonarVisionObserver;
	}

	private:
	static CPerkSonarVisionManager * s_mgr;

	// NB: If we're to support split-screen mode we need one of these per view. We'd also need to refactor code a great deal
	// to create n sets of sonar vision particles and make each set only visible in one view, which could get expensive and
	// may not be possible with current particle system...? [TF]
	CPerkSonarVisionObserver m_sonarVisionObserver;

	virtual void				Update (float frameTime);
};

#endif // __PERK_SONAR_VISION_H__
