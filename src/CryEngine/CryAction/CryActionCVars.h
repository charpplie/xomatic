/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2007.
-------------------------------------------------------------------------

Rip off NetworkCVars, general CryAction CVars should be here...

-------------------------------------------------------------------------
History:
- 6:6:2007   Created by Benito G.R.

*************************************************************************/
#ifndef __CRYACTIONCVARS_H__
#define __CRYACTIONCVARS_H__

#pragma once

#include "IConsole.h"

class CCryActionCVars
{
public:

	float playerInteractorRadius;  //Controls CInteractor action radius
	int		debugItemMemStats;				//Displays item mem stats

	int		g_debug_stats;
	int		g_statisticsMode;
	float g_distanceForceNoIk;
	int		useCurrentUserNameAsDefault;

#if !defined(_RELEASE)
	int		g_userNeverAutoSignsIn;
#endif

	ICVar* g_TweakProfile;

#ifdef AI_LOG_SIGNALS
	int aiLogSignals;
	float aiMaxSignalDuration;
#endif
	int	aiFlowNodeAlertnessCheck;

	// Disable HUD debug text
	int		cl_DisableHUDText;

	// Disable logging
	int		cl_DisableCombatLog;

	int g_gameplayAnalyst;

	// Cooperative Animation System
	int		co_coopAnimDebug;
	int		co_usenewcoopanimsystem;
	// ~Cooperative Animation System

	int g_syncClassRegistry;

	// AI stances
	ICVar* ag_defaultAIStance; 

	int g_usePlatformSavingAPI;

	

	static ILINE CCryActionCVars& Get()
	{
		CRY_ASSERT(s_pThis);
		return *s_pThis;
	}

private:
	friend class CCryAction; // Our only creator

	CCryActionCVars(); // singleton stuff
	~CCryActionCVars();

	static CCryActionCVars* s_pThis;

};

#endif // __CRYACTIONCVARS_H__
