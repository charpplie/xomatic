#ifndef __HUDCVARS_H__
#define __HUDCVARS_H__

#include "HUD/HUD.h"

//////////////////////////////////////////////////////////////////////////


class CHUDCVars
{

public:

	CHUDCVars();
	virtual     ~CHUDCVars();

	void        RegisterConsoleCommandsAndVars( void );
	void        UnregisterConsoleCommandsAndVars( void );


	static void	CmdReloadHUD(IConsoleCmdArgs *pArgs);
	static void	CmdActivateState(IConsoleCmdArgs *pArgs);	
	static void	CmdOnCrosshairSelected(ICVar* pCVar);

	static void CmdReloadUI(IConsoleCmdArgs* pArgs);
#if !defined(_RELEASE)
	static void CmdFrontEndSwitchPage(IConsoleCmdArgs* pArgs);
#endif
	static void CmdOnHideHUD(ICVar* pCVar);
	static void ShowHUD(const bool show);

#if ENABLE_HUD_EXTRA_DEBUG
	static void CmdToggleBattleAreaState(IConsoleCmdArgs* pArgs);
#endif

	void GetMemoryUsage(ICrySizer *pSizer) const
	{	
		pSizer->AddObject(this, sizeof(*this));
	}

public:

	// hide hud.
	int					hud_hide;

	int					hud_debugActiveStates;
	int					hud_debugDrawAssets;
	ICVar*			hud_debugDrawAssets_search;

	int					hud_drawSafeAreas;

	int					hud_Crosshair;
	float				hud_CrosshairSpreadMultiplier;
	float				hud_CrosshairMinSpread;
	int					hud_CrosshairUseSmoothSpread;
	int					hud_CrosshairHideOnReload;
#if ENABLE_HUD_EXTRA_DEBUG
	int					hud_CrosshairShowInfo;
#endif

	float				hud_InterestPointsRadius;
	float				hud_InterestPointsQueryRate;

	int					hud_radarDebug;
	int					hud_radar_ShowRadarBg;
	float				hud_radarOnShootTimeOnRadar;
	float				hud_radarShowVisibleEnemiesWithinConeSize;
	float				hud_radarOverrideScale;
	float				hud_radarOverrideMapRotation;
	float				hud_radarOverrideMapScale;

	int					hud_navPath_enable;
	float				hud_navPath_updateDistance;
	int					hud_navPath_autoClearAtEnd;
	float				hud_navPath_autoClearDistance;
	int					hud_navPath_maxPoints;
	float				hud_navPath_minSegmentLength;
	float				hud_navPath_segmentLengthIncrease;
	int					hud_navPath_smoothLevel;

	// Tag Names
	float	hud_mpTagNames_Duration;
	float	hud_mpTagNames_LockDuration;
	int		hud_mpTagNames_OffScreenTeammates;
	int		hud_mpTagNames_ForceDraw;
	int		hud_mpTagNames_ThroughGeom_friendies;
	int		hud_mpTagNames_ThroughGeom_enemies;
	int		hud_mpTagNames_ThroughGeom_dead;
	int		hud_mpTagNames_debug;
	float	hud_tagNames_healthBar_showBarFullWhenHealthIs;
	int		hud_tagNames_healthBar_numBlocks;
	float	hud_tagNames_healthBar_blockSizeX;
	float	hud_tagNames_healthBar_blockSizeY;
	float	hud_tagNames_healthBar_blockPositionOffsetY;
	float	hud_tagNames_healthBar_nearDistance;
	float	hud_tagNames_healthBar_farDistance;
	float hud_tagNames_showOverEnemiesWhenLookDirIsThisAccurate;
	int		hud_tagNames_showOverCorpses;
	float hud_mpTagNames_MinFontSize_nearSize;
	float hud_mpTagNames_MinFontSize_nearDist;
	float hud_mpTagNames_MinFontSize_midSize;
	float hud_mpTagNames_MinFontSize_midDist;
	float hud_mpTagNames_MinFontSize_farSize;
	float hud_mpTagNames_MinFontSize_farDist;
	float hud_mpTagNames_ZOffset_nearOffset;
	float hud_mpTagNames_ZOffset_nearDist;
	float hud_mpTagNames_ZOffset_midOffset;
	float hud_mpTagNames_ZOffset_midDist;
	float hud_mpTagNames_ZOffset_farOffset;
	float hud_mpTagNames_ZOffset_farDist;


	// HUD Testing
#if ENABLE_HUD_TESTS
	int hud_test_timeBetweenTestScreenShots;
	int hud_quitAfterTest;
	int hud_test_iterateResolutions;
	int hud_test_iterateSafeAreas;
	int hud_test_takeScreenShots;
#endif

	// Misc
	int					hud_drawCentreOfScreen;
	int					hud_hideMenus;
	int					hud_DMode;

#if !defined(_RELEASE)
	// Menus
	int					menu_debugScreenStack;
#endif
};


//////////////////////////////////////////////////////////////////////////

#endif

