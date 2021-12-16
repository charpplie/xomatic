#include "StdAfx.h"
#include "HUDEventDispatcher.h"

#include "HUD/HUD.h"

//////////////////////////////////////////////////////////////////////////


/*static*/ CHUDEventDispatcher::TEventMap CHUDEventDispatcher::s_eventMap;

typedef TKeyValuePair<string, EHUDEventType> THUDEventLookup;
static THUDEventLookup s_eventLookUpTable[] = {
	// HUD State & resolutions
	{"OnHUDUnload",                  eHUDEvent_OnHUDUnload},
	{"OnHUDReload",                  eHUDEvent_OnHUDReload},
	{"OnUpdate",                     eHUDEvent_OnUpdate},
	{"OnPostHUDDraw",                eHUDEvent_OnPostHUDDraw},
	{"OnResolutionChange",           eHUDEvent_OnResolutionChange},
	{"HUDElementVisibility",				 eHUDEvent_HUDElementVisibility	},
	// Local Player
	{"OnInitPlayer",                 eHUDEvent_OnInitPlayer},
	{"OnHealthChanged",              eHUDEvent_OnHealthChanged},
	{"OnPlayerDeath",                eHUDEvent_OnPlayerDeath},
	{"OnViewDistanceChanged",        eHUDEvent_OnViewDistanceChanged},
	{"OnSpawn",                      eHUDEvent_OnSpawn},
	{"PlayerSwitchTeams",            eHUDEvent_PlayerSwitchTeams},
	{"ShowReviveCycle",              eHUDEvent_ShowReviveCycle},
	{"OnShowHitIndicator",           eHUDEvent_OnShowHitIndicator},
	{"OnViewUpdate",								 eHUDEvent_OnViewUpdate},
	{"OnPhysicalImpulse",						 eHUDEvent_OnPhysicalImpulse},
		// Remote player
	{"OnEnterGame_RemotePlayer",     eHUDEvent_OnEnterGame_RemotePlayer},
	{"OnLeaveGame_RemotePlayer",     eHUDEvent_OnLeaveGame_RemotePlayer},
	//{"OnOnSpawn_RemotePlayer",       eHUDEvent_OnSpawn_RemotePlayer},
	//{"OnLeave_RemotePlayer",         eHUDEvent_OnLeave_RemotePlayer},
	// Score and progression
	{"OnNewScore",                   eHUDEvent_OnNewScore},
	{"OnNewXP",                      eHUDEvent_OnNewXP},
	{"OnPlayerPromotion",            eHUDEvent_OnPlayerPromotion},
	{"OnDogTagCollected",            eHUDEvent_OnDogTagCollected},
	// SuitMode & Menu
	{"OnSuitModeChanged",            eHUDEvent_OnSuitModeChanged},
	{"OnEnergyChanged",              eHUDEvent_OnEnergyChanged},
	{"OnSuitStateChanged",           eHUDEvent_OnSuitStateChanged},
	{"OnSuitMenuOpened",             eHUDEvent_OnSuitMenuOpened},
	{"OnSuitMenuClosed",             eHUDEvent_OnSuitMenuClosed},
	{"OnSuitPowerActivated",         eHUDEvent_OnSuitPowerActivated},
	// Weapons, Weapon Feedback & Crosshair
	{"OnStartReload",                eHUDEvent_OnStartReload},
	{"OnReloaded",                   eHUDEvent_OnReloaded},
	{"OnAmmoPickUp",                 eHUDEvent_OnAmmoPickUp},
	{"OnSetAmmoCount",               eHUDEvent_OnSetAmmoCount},
	{"OnShoot",                      eHUDEvent_OnShoot},
	{"OnItemSelected",               eHUDEvent_OnItemSelected},
	{"OnFireModeChanged",            eHUDEvent_OnFireModeChanged},
	{"OnCrosshairSelected",          eHUDEvent_OnCrosshairSelected},
	{"OnHitTarget",                  eHUDEvent_OnHitTarget},
	{"OnHit",                        eHUDEvent_OnHit},
	{"OnZoom",                       eHUDEvent_OnZoom},
	{"OnExplosiveSpawned",           eHUDEvent_OnExplosiveSpawned},
	// Prompts (pickups and interactions)
	{"OnLookAtChanged",              eHUDEvent_OnLookAtChanged},
	{"OnInteractionRequest",         eHUDEvent_OnInteractionRequest},
	{"OnUsableChanged",              eHUDEvent_OnUsableChanged},
	{"ShowingUsablePrompt",          eHUDEvent_ShowingUsablePrompt},
	{"HidingUsablePrompt",           eHUDEvent_HidingUsablePrompt},
	{"AddOnScreenMessage",					 eHUDEvent_AddOnScreenMessage},
	// MP Gamemodes
	{"GameEnded",                    eHUDEvent_GameEnded},
	{"OnGamemodeChange",             eHUDEvent_OnGamemodeChange},
	{"OnInitGameRules",              eHUDEvent_OnInitGameRules},
	{"OnUpdateGameStartMessage",     eHUDEvent_OnUpdateGameStartMessage},
	{"OnWaitingForPlayers",          eHUDEvent_OnWaitingForPlayers},
	{"OnGameStart",                  eHUDEvent_OnGameStart},
	{"OnSetGameStateMessage",        eHUDEvent_OnSetGameStateMessage},
	{"OnRoundStart",								 eHUDEvent_OnRoundStart},
	{"OnUpdateGameResumeMessage",    eHUDEvent_OnUpdateGameResumeMessage},
	// CTF Specific
	{"OnStartCarryingFlag",          eHUDEvent_OnStartCarryingFlag},
	{"OnOwnFlagStatusChanged",       eHUDEvent_OnOwnFlagStatusChanged},
	{"OnEnemyFlagStatusChanged",     eHUDEvent_OnEnemyFlagStatusChanged},
	// SP Objectives
	{"OnObjectiveChanged",           eHUDEvent_OnObjectiveChanged},
	// MP Objectives
	{"OnNewObjective",               eHUDEvent_OnNewObjective},
	{"OnRemoveObjective",            eHUDEvent_OnRemoveObjective},
	{"OnSiteBeingCaptured",          eHUDEvent_OnSiteBeingCaptured},
	{"OnSiteCaptured",               eHUDEvent_OnSiteCaptured},
	{"OnCaptureObjectiveNumChanged", eHUDEvent_OnCaptureObjectiveNumChanged},
	{"OnNewCaptureObjectiveWave",    eHUDEvent_OnNewCaptureObjectiveWave},
	{"OnOverallCaptureProgressUpdate", eHUDEvent_OnOverallCaptureProgressUpdate},
	// Death cam replay
	{"OnDeathcamStartPlay",          eHUDEvent_OnDeathcamStartPlay},
	{"OnDeathcamStopPlay",           eHUDEvent_OnDeathcamStopPlay},
	// Blind
	{"OnBlind",                      eHUDEvent_OnBlind},
	{"OnEndBlind",                   eHUDEvent_OnEndBlind},
	// Radar & Map
	{"AlignToRadarAsset",            eHUDEvent_AlignToRadarAsset},
	{"RadarRotation",                eHUDEvent_RadarRotation},
	{"MapInfo",                      eHUDEvent_MapInfo},
	{"ToggleMap",                    eHUDEvent_ToggleMap},
	{"TemporarilyTrackEntity",       eHUDEvent_TemporarilyTrackEntity},
	{"RescanActors",                 eHUDEvent_RescanActors},
	{"AddEntity",                    eHUDEvent_AddEntity},
	{"RemoveEntity",                 eHUDEvent_RemoveEntity},
	// Battle area
	{"LeavingBattleArea",            eHUDEvent_LeavingBattleArea},
	{"ReturningToBattleArea",        eHUDEvent_ReturningToBattleArea},
	// Host migration
	{"ShowHostMigrationScreen",      eHUDEvent_ShowHostMigrationScreen},
	{"HideHostMigrationScreen",      eHUDEvent_HideHostMigrationScreen},
	{"HostMigrationOnNewPlayer",     eHUDEvent_HostMigrationOnNewPlayer},
	// Perks
	{"OnScanPerkChanged",            eHUDEvent_OnScanPerkChanged},
	{"OnAIAwarenessChanged",         eHUDEvent_OnAIAwarenessChanged},
	{"OnTeamRadarChanged",					 eHUDEvent_OnTeamRadarChanged},
	// Tagnames
	{"OnIgnoreEntity",               eHUDEvent_OnIgnoreEntity},
	{"OnStopIgnoringEntity",         eHUDEvent_OnStopIgnoringEntity},
	// Scanning/Tactical info
	{"OnScanningStart",              eHUDEvent_OnScanningStart},
	{"OnEntityScanned",              eHUDEvent_OnEntityScanned},
	{"OnEntitySpotted",              eHUDEvent_OnEntitySpotted},
	{"OnScanningStop",               eHUDEvent_OnScanningStop},
	{"OnCenterEntityChanged",        eHUDEvent_OnCenterEntityChanged},
	{"OnVisorChanged",               eHUDEvent_OnVisorChanged},
	{"OnLeanChanged",                eHUDEvent_OnLeanChanged},
	{"OnTacticalInfoChanged",        eHUDEvent_OnTacticalInfoChanged},
	{"OnNewBattleLogMessage",        eHUDEvent_OnNewBattleLogMessage},
	// Menus & Scoreboards
	{"ShowScoreboard",               eHUDEvent_ShowScoreboard},
	{"HideScoreboard",               eHUDEvent_HideScoreboard},
	{"ToggleSelectMenu",             eHUDEvent_ToggleSelectMenu},
	{"ShowSelectMenu",               eHUDEvent_ShowSelectMenu},
	{"HideSelectMenu",               eHUDEvent_HideSelectMenu},
	{"ShowSelectMenuMap",            eHUDEvent_ShowSelectMenuMap},
	{"HideSelectMenuMap",            eHUDEvent_HideSelectMenuMap},
	{"OpenedIngameMenu",             eHUDEvent_OpenedIngameMenu},
	{"ClosedIngameMenu",             eHUDEvent_ClosedIngameMenu},
	// Save / Load
	{"OnGameSave",                   eHUDEvent_OnGameSave},
	{"OnGameLoad",                   eHUDEvent_OnGameLoad},
};


/*static*/ void CHUDEventDispatcher::AddHUDEventListener( IHUDEventListener* pListener, const char* in_eventName )
{
	const EHUDEventType eventType = GetEvent(in_eventName);

	if( eventType == eHUDEvent_None )
		CryHUDWarning(VALIDATOR_MODULE_GAME, VALIDATOR_WARNING, string().Format("HUD: Event '%s' not found/registered! (whilst adding HUD Event listener for it!)", in_eventName).c_str());

	std::pair<TEventMap::iterator, bool> result = s_eventMap.insert( TEventMap::value_type(eventType, THUDEventListeners()) );

	THUDEventListeners& listeners = result.first->second;

	stl::push_back_unique(listeners, pListener);

#if IS_HUD_DEV
	CryLog( "HUD: Adding listener for '%s' \t%p", in_eventName, pListener );
#endif // IS_HUD_DEV
}



/*static*/ void CHUDEventDispatcher::RemoveHUDEventListener( const IHUDEventListener* pListener )
{
	TEventMap::iterator mapIt = s_eventMap.begin();
	TEventMap::iterator mapEnd = s_eventMap.end();
	for(; mapIt!=mapEnd; ++mapIt)
	{
		THUDEventListeners& listeners = mapIt->second;
		THUDEventListeners::iterator it = listeners.begin();
		THUDEventListeners::iterator end = listeners.end();
		for(; it!=end; ++it)
		{
			if((*it)==pListener)
			{
				listeners.erase(it);
				break;
			}
		}
	}
}



/*static*/ void CHUDEventDispatcher::CallEvent( const SHUDEvent& event )
{
	TEventMap::iterator found = s_eventMap.find(event.eventType);
	if(found==s_eventMap.end())
		return;

	THUDEventListeners& listeners = found->second;
	THUDEventListeners::const_iterator it = listeners.begin();
	THUDEventListeners::const_iterator end = listeners.end();
	for(; it!=end; ++it)
	{
		IHUDEventListener* listener = *it;
		listener->OnHUDEvent(event);
	}
}



/*static*/ EHUDEventType CHUDEventDispatcher::GetEvent(const char* eventName)
{
	unsigned int regdEventCount = sizeof( s_eventLookUpTable )/sizeof(THUDEventLookup);
	for(int i=0; i<regdEventCount; ++i )
	{
		if( 0==stricmp(eventName,s_eventLookUpTable[i].key.c_str()))
		{
			return s_eventLookUpTable[i].value;
		}
	}

	return eHUDEvent_None;
}

/*static*/ void CHUDEventDispatcher::CheckRegisteredEvents( void )
{
	unsigned int regdEventCount = sizeof( s_eventLookUpTable )/sizeof(THUDEventLookup);

	// Check for delta sizes
	CRY_ASSERT_MESSAGE( regdEventCount == eHUDEvent_LAST-1, string().Format("HUD: Wrong number of events registered (%d) when compared to the number defined (%d)! /FH", regdEventCount, (int)eHUDEvent_LAST - 1 ).c_str() );

	// Check for unreg'd
	for(int defdEventId=eHUDEvent_None+1; defdEventId<eHUDEvent_LAST; ++defdEventId )
	{
		bool definedEventFound = false;
		for( int regdEventId=0; regdEventId<regdEventCount; ++regdEventId )
		{
			if( s_eventLookUpTable[regdEventId].value == defdEventId )
			{
				definedEventFound = true;
			}
		}
		CRY_ASSERT_MESSAGE( definedEventFound, string().Format("HUD: A defined event id was not found when looking for it in registered events. ID was %d. /FH", defdEventId).c_str() );
	}

	// Check for dupes
	for(int i=0; i<regdEventCount; ++i )
	{
		const char* regdName = s_eventLookUpTable[i].key.c_str();
		const EHUDEventType redgType = s_eventLookUpTable[i].value;
		for(int j=i+1; j<regdEventCount; ++j )
		{
			const char* checkName = s_eventLookUpTable[j].key.c_str();
			const EHUDEventType checkType = s_eventLookUpTable[j].value;
			CRY_ASSERT_MESSAGE( stricmp(regdName, checkName), string().Format("HUD: Duplicate event names found for %s(id%d). /FH", regdName, redgType ).c_str() );
			CRY_ASSERT_MESSAGE( redgType != checkType, string().Format("HUD: Duplicate event ID entries found names found for ID%d(%s). /FH", redgType, regdName).c_str() );
		}
	}
}

//////////////////////////////////////////////////////////////////////////
