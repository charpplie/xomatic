/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2009.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: 

-------------------------------------------------------------------------
History:
	- 11:09:2009  : Created by Thomas Houghton

*************************************************************************/

#include "StdAfx.h"

#include "GameRulesMPSpectator.h"
#include "Game.h"
#include "HUD/HUD.h"
#include "HUD/HUD_Impl.h"
#include "GameCVars.h"
#include "Actor.h"
#include "Player.h"
#include "GameRulesMPSpawning.h"
#include "IGameRulesPlayerStatsModule.h"

#include "Utility/CryWatch.h"

// -----------------------------------------------------------------------

#define ASSERT_IS_CLIENT if (!gEnv->bClient) { assert(gEnv->bClient); return; }
#define ASSERT_IS_SERVER if (!gEnv->bServer) { assert(gEnv->bServer); return; }
#define ASSERT_IS_SERVER_RETURN_FALSE if (!gEnv->bServer) { assert(gEnv->bServer); return false; }

// -----------------------------------------------------------------------

static const float  kTagTime = 15.f;
static const float  kCctvTagZoomFxTime = 0.4f;

// -----------------------------------------------------------------------
CGameRulesMPSpectator::CGameRulesMPSpectator()
{
	m_pGameRules = NULL;
	m_pGameFramework = NULL;
	m_pGameplayRecorder = NULL;
	m_pActorSys = NULL;
	m_pUIDraw = NULL;

	m_eatsActorActions = 0;
	m_enableFree = 0;
	m_enableFollow = 0;
	m_enableCCTV = 0;

	m_timeFromDeathTillAutoSpectate = 0.f;

	m_specCctvCursorTexId = 0;
}

// -----------------------------------------------------------------------
CGameRulesMPSpectator::~CGameRulesMPSpectator()
{
	if (m_specCctvCursorTexId)
	{
		m_pUIDraw->DeleteTexture(m_specCctvCursorTexId);
		m_specCctvCursorTexId = 0;
	}
}

// -----------------------------------------------------------------------
void CGameRulesMPSpectator::Init(XmlNodeRef xml)
{
	CryLog("CGameRulesMPSpectator::Init()");

	m_pGameRules = g_pGame->GetGameRules();
	assert(m_pGameRules);
	m_pGameFramework = g_pGame->GetIGameFramework();
	assert(m_pGameFramework);
	m_pGameplayRecorder = m_pGameFramework->GetIGameplayRecorder();
	assert(m_pGameplayRecorder);
	m_pActorSys = gEnv->pGame->GetIGameFramework()->GetIActorSystem();
	assert(m_pActorSys);
	m_pUIDraw = gEnv->pGame->GetIGameFramework()->GetIUIDraw();
	assert(m_pUIDraw);

	bool  boo;
	float  f;

	if (xml->getAttr("eatsActorActions", boo))
		m_eatsActorActions = boo;
	else
		CRY_ASSERT_MESSAGE(0, "CGameRulesMPSpectator failed to find valid eatsActorActions param");
	
	if (xml->getAttr("enableFree", boo))
		m_enableFree = boo;
	else
		CRY_ASSERT_MESSAGE(0, "CGameRulesMPSpectator failed to find valid enableFree param");

	if (xml->getAttr("enableFollow", boo))
		m_enableFollow = boo;
	else
		CRY_ASSERT_MESSAGE(0, "CGameRulesMPSpectator failed to find valid enableFollow param");

	if (xml->getAttr("enableCCTV", boo))
		m_enableCCTV = boo;
	else
		CRY_ASSERT_MESSAGE(0, "CGameRulesMPSpectator failed to find valid enableCCTV param");

	if (xml->getAttr("timeFromDeathTillAutoSpectate", f))
		m_timeFromDeathTillAutoSpectate = f;
	else
		CRY_ASSERT_MESSAGE(0, "CGameRulesMPSpectator failed to find valid timeFromDeathTillAutoSpectate param");

	CryLog("Init() set params to eatsActorActions=%d; enableFree=%d; enableFollow=%d enableCCTV=%d, timeFromDeathTillAutoSpectate=%.1f", m_eatsActorActions, m_enableFree, m_enableFollow, m_enableCCTV, m_timeFromDeathTillAutoSpectate);

	assert (!m_specCctvCursorTexId);
	m_specCctvCursorTexId = m_pUIDraw->CreateTexture("Textures\\UI2\\cw2_spec_cctv_cursor.tif", false);
}

// -----------------------------------------------------------------------
void CGameRulesMPSpectator::PostInit()
{
	CryLog("CGameRulesMPSpectator::PostInit()");
}

// -----------------------------------------------------------------------
void CGameRulesMPSpectator::Update(float frameTime)
{
	PostUpdateSpectatorCCTVTagging();
}

// -----------------------------------------------------------------------
bool CGameRulesMPSpectator::ModeIsEnabled(const int mode) const
{
	bool  is = false;
	switch (mode)
	{
		case CActor::eASM_Fixed:	is = true; break;
		case CActor::eASM_Free:		is = (m_enableFree   && !g_pGame->GetCVars()->g_spectate_DisableFree);   break;
		case CActor::eASM_Follow:	is = (m_enableFollow && !g_pGame->GetCVars()->g_spectate_DisableFollow); break;
		case CActor::eASM_CCTV:		is = (m_enableCCTV   && !g_pGame->GetCVars()->g_spectate_DisableCCTV);   break;
		default: assert(0);
	}
	return is;
}

// -----------------------------------------------------------------------
bool CGameRulesMPSpectator::ModeIsAvailable(const EntityId playerId, const int mode, EntityId* outOthEnt)
{
	bool  is = false;
	EntityId  othEnt = 0;

	if (ModeIsEnabled(mode))
	{
		switch (mode)
		{
			case CActor::eASM_Fixed:
				if (!ModeIsAvailable(playerId, CActor::eASM_CCTV, NULL))  // only let Fixed be chosen if CCTV is not available
				{
					othEnt = GetInterestingSpectatorLocation();
					if (othEnt)
					{
						is = true;
					}
				}
				break;
			case CActor::eASM_Free:
				is = true;
				break;
			case CActor::eASM_Follow:
				othEnt = GetNextSpectatorTarget(playerId, 1);
				if (othEnt)
					is = true;
				break;
			case CActor::eASM_CCTV:
				//if (pGameRules && (pGameRules->GetTeam(playerId) != 0))  // CCTV used to be only enabled in team games, but i don't see why that should be the case anymore, and if a game mode wanted to restrict CCTV to just team games then an xml paramater should be added to allow that
				{
					othEnt = GetInterestingCCTVLocation();
					if (othEnt)
					{
						is = true;
					}
				}
				break;
			default:
				assert(0);
		}
	}

	if (is && outOthEnt)
		(*outOthEnt) = othEnt;
	return is;
}

// -----------------------------------------------------------------------
// TODO it'd be cool if the priority order of the modes could be specified in the xml somehow?
int CGameRulesMPSpectator::GetBestAvailableMode(const EntityId playerId, EntityId* outOthEnt)
{
	int  mode = 0;
	if (ModeIsAvailable(playerId, CActor::eASM_CCTV, outOthEnt))
	{
		mode = CActor::eASM_CCTV;
	}
	else if (ModeIsAvailable(playerId, CActor::eASM_Follow, outOthEnt))
	{
		mode = CActor::eASM_Follow;
	}
	else if (ModeIsAvailable(playerId, CActor::eASM_Fixed, outOthEnt))
	{
		mode = CActor::eASM_Fixed;
	}
	else
	{
		CRY_ASSERT_MESSAGE(ModeIsAvailable(playerId, CActor::eASM_Free, outOthEnt), "DESIGNER ASSERT: all levels need at least one of either a spectator point or CCTV point");
		mode = CActor::eASM_Free;
	}
	assert(mode);
	return mode;
}

// -----------------------------------------------------------------------
int CGameRulesMPSpectator::GetNextMode(EntityId playerId, int inc, EntityId* outOthEid)
{
	CRY_ASSERT(inc==-1 || inc==1);
	//inc = ((inc < 0) ? -1 : 1);
	int  mode = (int) CActor::eASM_None;
	EntityId  othEntId = 0;
	if (IActor* pActor=g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(playerId))
	{
		CActor*  pActorImpl = static_cast< CActor* >( pActor );
		const CActor::EActorSpectatorMode  curMode = (CActor::EActorSpectatorMode) pActorImpl->GetSpectatorMode();
		mode = (int) curMode;
		bool  modeok;
		do
		{
			if (inc >= 0)
			{
				mode++;
				if (mode > CActor::eASM_LastMPMode)
				{
					mode = CActor::eASM_FirstMPMode;
				}
			}
			else
			{
				mode--;
				if(mode < CActor::eASM_FirstMPMode)
				{
					mode = CActor::eASM_LastMPMode;
				}
			}
			othEntId = 0;
			modeok = ModeIsAvailable(playerId, mode, &othEntId);
		}
		while (!modeok && (mode != curMode));
	}
	if (outOthEid)
		(*outOthEid) = othEntId;
	return mode;
}

// -----------------------------------------------------------------------
void CGameRulesMPSpectator::SvOnChangeSpectatorMode( EntityId playerId, int mode, EntityId targetId, bool resetAll )
{
	CryLogAlways("CGameRulesMPSpectator::SvOnChangeSpectatorMode");
	ASSERT_IS_SERVER;

	CActor* pActor = static_cast<CActor*>(m_pActorSys->GetActor(playerId));
	if (!pActor)
		return;

	if (mode>0)
	{
		if(resetAll)
		{
			//TODO: Destroy inventory
		}
		
		if (mode == CActor::eASM_Free)
		{
			Vec3 pos(0.f, 0.f, 0.f);
			Ang3 angles(0.f, 0.f, 0.f);

			pActor->SetSpectatorModeAndOtherEntId(CActor::eASM_Free, 0);

			EntityId locationId = GetInterestingSpectatorLocation();
			if (locationId)
			{
				IEntity *location = gEnv->pEntitySystem->GetEntity(locationId);
				if (location)
				{
					pos = location->GetWorldPos();
					angles = location->GetWorldAngles();
					
					m_pGameRules->MovePlayer(pActor, pos, Quat(angles));
				}
			}
		}
		else if (mode == CActor::eASM_Fixed)
		{
			if (targetId)
			{
				pActor->SetSpectatorModeAndOtherEntId(CActor::eASM_Fixed, targetId);
			}
			else
			{
				EntityId newLoc = GetInterestingSpectatorLocation();
				if(newLoc)
				{
					pActor->SetSpectatorModeAndOtherEntId(CActor::eASM_Fixed, newLoc);
				}
			}
		}
		else if (mode == CActor::eASM_Follow)
		{
			if (targetId)
			{
				pActor->SetSpectatorModeAndOtherEntId(CActor::eASM_Follow, targetId);
			}
			else
			{
				EntityId newTargetId = GetNextSpectatorTarget(playerId, 1);
				if(newTargetId)
				{
					pActor->SetSpectatorModeAndOtherEntId(CActor::eASM_Follow, newTargetId);
				}
			}
		}
		else if (mode == CActor::eASM_CCTV)
		{
			if (targetId)
			{
				pActor->SetSpectatorModeAndOtherEntId(CActor::eASM_CCTV, targetId);
			}
			else
			{
				EntityId newCCTV = GetRandomCCTVLocation();
				if(newCCTV)
				{
					pActor->SetSpectatorModeAndOtherEntId(CActor::eASM_CCTV, newCCTV);
				}
			}
		}
	}
	else
	{
	//	if (self:CanRevive(playerId)) then	
		pActor->SetSpectatorModeAndOtherEntId(CActor::eASM_None, 0);
	//	end
	}
	
	TChannelSpectatorModeMap::iterator it = m_channelSpectatorModes.find(pActor->GetChannelId());
	if (it != m_channelSpectatorModes.end())
	{
		it->second = mode;
	}
	else
	{
		m_channelSpectatorModes.insert(TChannelSpectatorModeMap::value_type(pActor->GetChannelId(), mode));
	}
}

// -----------------------------------------------------------------------
void CGameRulesMPSpectator::SvRequestSpectatorTarget( EntityId playerId, int change )
{
	CryLogAlways("CGameRulesMPSpectator::SvRequestSpectatorTarget");
	ASSERT_IS_SERVER;

}

// -----------------------------------------------------------------------
void CGameRulesMPSpectator::ClOnChangeSpectatorMode( EntityId playerId, int mode, EntityId targetId, bool resetAll )
{
	CryLogAlways("CGameRulesMPSpectator::ClOnChangeSpectatorMode");
	ASSERT_IS_CLIENT;

}

// -----------------------------------------------------------------------
EntityId CGameRulesMPSpectator::GetNextSpectatorTarget( EntityId playerId, int change )
{
	if(change >= 1) change = 1;
	if(change <= 0) change = -1;

	CActor* pActor = static_cast<CActor*>(m_pActorSys->GetActor(playerId));
	if(pActor)
	{
		CGameRules::TPlayers  players;
		players.reserve(m_pGameRules->GetNumChannels());

		int  teamCount = m_pGameRules->GetTeamCount();
		int  friendlyTeam = m_pGameRules->GetTeam(playerId);

		if ((teamCount == 0) || (friendlyTeam == 0))
		{
			IGameRulesPlayerStatsModule*  pPlayStatsMod = m_pGameRules->GetPlayerStatsModule();
			assert(pPlayStatsMod);
			int  numStats = pPlayStatsMod->GetNumPlayerStats();
			for (int i=0; i<numStats; i++)
			{
				const SGameRulesPlayerStat*  s = pPlayStatsMod->GetNthPlayerStats(i);
				CPlayer*  loopPlr = static_cast< CPlayer* >( m_pActorSys->GetActor(s->playerId) );
				if (!loopPlr)
				{
					CryLog(" > skipping entity because its actor is NULL");
					continue;
				}
				IEntity  *loopEnt = loopPlr->GetEntity();
				assert(loopEnt);
				players.push_back(loopEnt->GetId());
			}
		}
		else
		{
			for (int i=1; i<=teamCount; i++)
			{
				if (g_pGame->GetCVars()->g_spectate_TeamOnly && (i != friendlyTeam))
				{
					continue;
				}
				CGameRules::TPlayers  teamPlayers;
				m_pGameRules->GetTeamPlayers(i, teamPlayers);
				CGameRules::TPlayers::const_iterator  it = teamPlayers.begin();
				CGameRules::TPlayers::const_iterator  end = teamPlayers.end();
				for (; it!=end; ++it)
				{
					CPlayer  *loopPlr = static_cast<CPlayer*>(m_pActorSys->GetActor(*it));
					if (!loopPlr)
					{
						CryLog(" > skipping entity because its actor is NULL");
						continue;
					}
					IEntity  *loopEnt = loopPlr->GetEntity();
					assert(loopEnt);
					players.push_back(loopEnt->GetId());
				}
			}
		}
		
		int  numPlayers = players.size();

		// work out which one we are currently watching
		EntityId  specTarg = ((pActor->GetSpectatorMode() == CActor::eASM_Follow) ? pActor->GetSpectatorTarget() : 0);
		int index = (specTarg ? 0 : numPlayers);  // (only do the for-loop if we've actually got something to find)
		for(; index < numPlayers; ++index)
		{
			if(players[index] == specTarg)
			{
				break;
			}
		}

		// loop through the players to find a valid one.
		bool found = false;
		if(numPlayers > 0)
		{
			int newTargetIndex = index;
			int numAttempts = numPlayers;
			do
			{
				newTargetIndex += change;
				--numAttempts;

				// wrap around
				if(newTargetIndex < 0)
					newTargetIndex = numPlayers-1;
				if(newTargetIndex >= numPlayers)
					newTargetIndex = 0;

				// skip ourself
				if(players[newTargetIndex] == playerId)
					continue;

				// skip dead players and spectating players
				CActor* pTarget = static_cast<CActor*>(m_pActorSys->GetActor(players[newTargetIndex]));
				if (!pTarget || (pTarget->GetHealth() <= 0) || (pTarget->GetSpectatorMode() != CActor::eASM_None))
					continue;				

				// otherwise this one will do.
				found = true;
			} while(!found && numAttempts > 0);

			if(found)
			{
				return players[newTargetIndex];
			}
		}
	}
	return 0;
}

//------------------------------------------------------------------------
// [tlh] Gets localisable string label that represents name of the spectator mode 'mode'
const char* CGameRulesMPSpectator::GetActorSpectatorModeName(uint8 mode)
{
	static const char*  modenames[] = {
		"@ui_hud_spec_mode_fixed",
		"@ui_hud_spec_mode_free",
		"@ui_hud_spec_mode_follow",
		"@ui_hud_spec_mode_cctv"};

	const char*  name = NULL;

	if (mode>=CActor::eASM_FirstMPMode && mode <=CActor::eASM_LastMPMode)
	{
		name = modenames[mode-CActor::eASM_FirstMPMode];
	}

	return name;
}

//------------------------------------------------------------------------
// TODO this might eventually need to consider current HUD state (map, scoreboard, etc.) etc.
bool CGameRulesMPSpectator::CanChangeSpectatorMode(EntityId playerId) const
{
	if (IActor* pActor=g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(playerId))
	{
		CActor*  pActorImpl = static_cast< CActor* >( pActor );
		if (pActorImpl->GetSpectatorState() == CActor::eASS_None)
		{
			return false;
		}
	}
	return true;
}

//------------------------------------------------------------------------
void CGameRulesMPSpectator::ChangeSpectatorMode(IActor *pActor, uint8 mode, EntityId targetId, bool resetAll)
{
	CryLog("[tlh] @ CGameRulesMPSpectator::ChangeSpectatorMode(), actor=%p, server=%d", pActor, gEnv->bServer);

	if (!pActor)
		return;

	if (pActor->GetSpectatorMode()==mode && mode!=CActor::eASM_Follow && mode!=CActor::eASM_CCTV)  // [tlh] the Follow mode must be allowed thru here because it uses the ChangeSpectatorMode mechanism to also change the target it is following, and the CCTV must too because it also uses it to track which camera it is using
		return;

	if (gEnv->bEditor)		// Editor doesn't use spectator mode
	{
		return;
	}

	if (gEnv->bServer)
	{
		SvOnChangeSpectatorMode(pActor->GetEntityId(), mode, targetId, resetAll);
		m_pGameplayRecorder->Event(pActor->GetEntity(), GameplayEvent(eGE_Spectator, 0, (float)mode));
	}
	else if (pActor->IsClient())
	{
		ClOnChangeSpectatorMode(pActor->GetEntityId(), mode, targetId, resetAll);
		m_pGameRules->SendRMI_SvRequestSpectatorMode(pActor->GetEntityId(), mode, targetId, resetAll, eRMI_ToServer);
	}
}

// -----------------------------------------------------------------------
void CGameRulesMPSpectator::ChangeSpectatorModeBestAvailable(IActor *pActor, bool resetAll)
{
	EntityId  othEnt;
	int  mode = GetBestAvailableMode(pActor->GetEntityId(), &othEnt);
	assert(mode);
	ChangeSpectatorMode(pActor, mode, othEnt, resetAll);
}

//------------------------------------------------------------------------
void CGameRulesMPSpectator::RequestNextSpectatorTarget(IActor* pActor, int change)
{
	if(pActor->GetSpectatorMode() != CActor::eASM_Follow)
		return;

	if(gEnv->bServer && pActor)
	{
		SvRequestSpectatorTarget(pActor->GetEntityId(), change);
	}
}

//------------------------------------------------------------------------
void CGameRulesMPSpectator::AddSpectatorLocation(EntityId location)
{
	IEntity* ent = gEnv->pEntitySystem->GetEntity(location);
	assert (ent);

	ent->SetFlags(ent->GetFlags() | ENTITY_FLAG_NO_PROXIMITY);

	stl::push_back_unique(m_spectatorLocations, location);
}

//------------------------------------------------------------------------
void CGameRulesMPSpectator::RemoveSpectatorLocation(EntityId id)
{
	stl::find_and_erase(m_spectatorLocations, id);
}

//------------------------------------------------------------------------
int CGameRulesMPSpectator::GetSpectatorLocationCount() const
{
	return (int)m_spectatorLocations.size();
}

//------------------------------------------------------------------------
EntityId CGameRulesMPSpectator::GetSpectatorLocation(int idx) const
{
	if (idx>=0 && idx<m_spectatorLocations.size())
		return m_spectatorLocations[idx];
	return 0;
}

//------------------------------------------------------------------------
void CGameRulesMPSpectator::GetSpectatorLocations(TSpawnLocations &locations) const
{
	locations.resize(0);
	locations = m_spectatorLocations;
}

//------------------------------------------------------------------------
EntityId CGameRulesMPSpectator::GetRandomSpectatorLocation() const
{
	EntityId  e = 0;
	if (int count=GetSpectatorLocationCount())
		e = GetSpectatorLocation(Random(count));
	return e;
}

//------------------------------------------------------------------------
EntityId CGameRulesMPSpectator::GetInterestingSpectatorLocation() const
{
	return GetRandomSpectatorLocation();
}

//------------------------------------------------------------------------
void CGameRulesMPSpectator::AddCCTVLocation(EntityId location)
{
	stl::push_back_unique(m_cctvLocations, location);
}

//------------------------------------------------------------------------
void CGameRulesMPSpectator::RemoveCCTVLocation(EntityId id)
{
	stl::find_and_erase(m_cctvLocations, id);
}

//------------------------------------------------------------------------
int CGameRulesMPSpectator::GetCCTVLocationCount() const
{
	return (int)m_cctvLocations.size();
}

//------------------------------------------------------------------------
EntityId CGameRulesMPSpectator::GetCCTVLocationAtIdx(int idx) const
{
	if (idx>=0 && idx<m_cctvLocations.size())
		return m_cctvLocations[idx];
	return 0;
}

//------------------------------------------------------------------------
EntityId CGameRulesMPSpectator::GetCurCCTVLocation(const CPlayer *pPlayer) const
{
	return pPlayer->GetSpectatorCCTVCam();
}

//------------------------------------------------------------------------
EntityId CGameRulesMPSpectator::GetNextCCTVLocation(const CActor* pPlayer) const
{
	CryLog("[tlh] @ CGameRulesMPSpectator::GetNextCCTVLocation()");
	const int  sz = m_cctvLocations.size();
	if (sz > 0)
	{
		EntityId  cur = pPlayer->GetSpectatorCCTVCam();
		CryLog("[tlh] > has cur CCTV entity %d...", cur);
		int  idx = (cur ? 0 : sz);  // no need to search for cur if haven't even got one
		for (; idx<sz; idx++)
		{
			if (m_cctvLocations[idx] == cur)
			{
				break;
			}
		}
		if (idx < sz)  // ie. found, so increment to next
		{
			CryLog("[tlh] > cur %d found at idx %d...", cur, idx);
			idx++;
		}
		if (idx == sz)
		{
			EntityId  ret = m_cctvLocations[0];
			CryLog("[tlh] > cur at end, returning first entry, %d", ret);
			return ret;
		}
		else
		{
			EntityId  ret = m_cctvLocations[idx];
			CryLog("[tlh] > cur NOT at end, returning next entry, %d", ret);
			return ret;
		}
	}
	CryLog("[tlh] > no cctv locs in level or gamemode, returning 0");
	return 0;
}

//------------------------------------------------------------------------
EntityId CGameRulesMPSpectator::GetPrevCCTVLocation(const CActor* pPlayer) const
{
	CryLog("[tlh] @ CGameRulesMPSpectator::GetPrevCCTVLocation()");
	const int  sz = m_cctvLocations.size();
	if (sz > 0)
	{
		EntityId  cur = pPlayer->GetSpectatorCCTVCam();
		CryLog("[tlh] > has cur CCTV entity %d...", cur);
		int  idx = (cur ? 0 : sz);  // no need to search for cur if haven't even got one
		for (; idx<sz; idx++)
		{
			if (m_cctvLocations[idx] == cur)
			{
				break;
			}
		}
		idx--;  // (can do this regardless of whether cur was found or not, because if it wasn't then idx will be at one past the end, so decrementing it will put it at the end)
		if (idx < 0)
		{
			EntityId  ret = m_cctvLocations[sz - 1];
			CryLog("[tlh] > cur at beginning, returning last entry, %d", ret);
			return ret;
		}
		else
		{
			EntityId  ret = m_cctvLocations[idx];
			CryLog("[tlh] > cur NOT at beginning, returning previous entry, %d", ret);
			return ret;
		}
	}
	CryLog("[tlh] > no cctv locs in level or gamemode, returning 0");
	return 0;
}

//------------------------------------------------------------------------
void CGameRulesMPSpectator::GetCCTVLocations(TSpawnLocations &locations) const
{
	locations.resize(0);
	locations = m_cctvLocations;
}

//------------------------------------------------------------------------
EntityId CGameRulesMPSpectator::GetRandomCCTVLocation() const
{
	EntityId  e = 0;
	if (int count=GetCCTVLocationCount())
		e = GetCCTVLocationAtIdx(Random(count));
	return e;
}

//------------------------------------------------------------------------
EntityId CGameRulesMPSpectator::GetInterestingCCTVLocation() const
{
	return GetRandomCCTVLocation();
}

// -----------------------------------------------------------------------
bool CGameRulesMPSpectator::GetModeFromChannelSpectatorMap(int channelId, int* outMode) const
{
	bool  found = false;
	TChannelSpectatorModeMap::const_iterator  it = m_channelSpectatorModes.find(channelId);
	if (it != m_channelSpectatorModes.end())
	{
		if (outMode)
			(*outMode) = it->second;
		found = true;
	}
	return true;
}

// -----------------------------------------------------------------------
void CGameRulesMPSpectator::FindAndRemoveFromChannelSpectatorMap(int channelId)
{
	TChannelSpectatorModeMap::iterator  it = m_channelSpectatorModes.find(channelId);
	if (it != m_channelSpectatorModes.end())
	{
		m_channelSpectatorModes.erase(it);
	}
}

// -----------------------------------------------------------------------
// [tlh] TODO? copy-pasted from HUDTagNames.cpp (where it's implicitly static - ie. not in any header files) ... shouldn't this be in the engine alongside the ProjectToScreen funcs?
static bool CoordsOnScreen(const Vec3& vScreenSpace)
{
	bool bResult = true;

	if(vScreenSpace.z < 0.0f || vScreenSpace.z > 1.0f)
	{
		bResult = false;
	}

	if(vScreenSpace.y < 0.0f || vScreenSpace.y > 100.0f)
	{
		bResult = false;
	}

	if(vScreenSpace.x < 0.0f || vScreenSpace.x > 100.0f)
	{
		bResult = false;
	}

	return bResult;
}

// -----------------------------------------------------------------------
// NOTE this gets undef'd at the end of this file
struct SScreenBounds
{
	Vec2  min;
	Vec2  max;
};

// -----------------------------------------------------------------------
// [tlh] note: can only be called during certain points within a frame - see comment in function body
static int ProjectBoundsPointToScreen(const float &pointX, const float &pointY, const float &pointZ, SScreenBounds *ioScrBounds)
{
	Vec3  screenPos;
	gEnv->pRenderer->ProjectToScreen(pointX, pointY, pointZ, &screenPos.x, &screenPos.y, &screenPos.z);  // [tlh] note: this func only works properly when called during certain points within a frame - for example, it works when called from a PostUpdate function but NOT from an OnActionXXX function

	if (screenPos.x < ioScrBounds->min.x)
		ioScrBounds->min.x = screenPos.x;
	else if (screenPos.x > ioScrBounds->max.x)
		ioScrBounds->max.x = screenPos.x;

	if (screenPos.y < ioScrBounds->min.y)
		ioScrBounds->min.y = screenPos.y;
	else if (screenPos.y > ioScrBounds->max.y)
		ioScrBounds->max.y = screenPos.y;

	return (CoordsOnScreen(screenPos) ? 1 : 0);
}

// -----------------------------------------------------------------------
void CGameRulesMPSpectator::PostUpdateSpectatorCCTVTagging()
{
	IActor*  pClient = g_pGame->GetIGameFramework()->GetClientActor();

	if (CPlayer* pPlayer=((pClient && pClient->IsPlayer()) ? static_cast< CPlayer* >( pClient ) : NULL))
	{
		SSpectatorInfo*  spinf = &(((SPlayerStats*)pPlayer->GetActorStats())->spectatorInfo);

		if (pPlayer->GetSpectatorMode() == CActor::eASM_CCTV)
		{
			USpectatorModeData::SCctv*  cctvd = &spinf->dataU.cctv;

			cctvd->entUnderCursor = FindEntityUnderCCTVCursor();

			// draw CCTV cursor (TODO consider refactoring into func)
			{
				ColorF  col = (cctvd->entUnderCursor ? ColorF(1.f, .1f, .1f) : ColorF(1.f, 1.f, 1.f));

				if (!GetISystem()->IsDedicated())
				{
					assert(m_specCctvCursorTexId);
				}
				// TODO the cursor will eventually need to be unanchored from the centre of the screen
				m_pUIDraw->DrawImageCentered(m_specCctvCursorTexId, 400.f, 300.f, 16.f, 16.f, 0.f, col.r, col.g, col.b, col.a);
			}
		}

		// update CCTV tagging GFX (TODO consider refactoring into func)
		{
			if (spinf->cctvTagFx.ent)
			{
				if (IEntity* fxent=gEnv->pEntitySystem->GetEntity(spinf->cctvTagFx.ent))
				{
					if (spinf->cctvTagFx.time <= kCctvTagZoomFxTime)
					{
						const float  f = GetISystem()->GetITimer()->GetFrameTime();

						IRenderAuxGeom  *pRenderAuxGeom ( gEnv->pRenderer->GetIRenderAuxGeom() );

						pRenderAuxGeom->SetRenderFlags(e_Def2DPublicRenderflags|e_AlphaBlended);

						const float  maxw = 0.6f;
						const float  maxh = 0.6f;
						const float  minw = 0.003f;
						const float  minh = 0.009f;

						AABB  bounds;
						fxent->GetWorldBounds( bounds );
						const Vec3  cent3 = bounds.GetCenter();
						Vec3  cent2;
						gEnv->pRenderer->ProjectToScreen(cent3.x, cent3.y, cent3.z, &cent2.x, &cent2.y, &cent2.z);
						cent2 /= 100.f;

						const float  t = (spinf->cctvTagFx.time / kCctvTagZoomFxTime);
						const float  wdiv2 = ((minw + ((1.f - t) * (maxw - minw))) * 0.5f);
						const float  hdiv2 = ((minh + ((1.f - t) * (maxh - minh))) * 0.5f);

						const float  lft = (cent2.x - wdiv2);
						const float  rht = (cent2.x + wdiv2);
						const float  top = (cent2.y - hdiv2);
						const float  btm = (cent2.y + hdiv2);

						const Vec3  lt ( lft, top, 0.f );
						const Vec3  rt ( rht, top, 0.f );
						const Vec3  rb ( rht, btm, 0.f );
						const Vec3  lb ( lft, btm, 0.f );

						const ColorB  col ( 0, 0, 255, 200 );

						// [tlh] TODO look at using DrawLines() here instead?
						pRenderAuxGeom->DrawLine(lt, col, rt, col);
						pRenderAuxGeom->DrawLine(rt, col, rb, col);
						pRenderAuxGeom->DrawLine(rb, col, lb, col);
						pRenderAuxGeom->DrawLine(lb, col, lt, col);

						spinf->cctvTagFx.time += f;

						if (spinf->cctvTagFx.time > kCctvTagZoomFxTime)
						{
							//g_pGame->GetHUD()->GetSilhouettes()->SetSilhouette(fxent, 0.f, 0.f, 1.f, 1.f, kTagTime, false);
						}
					}

					if (spinf->cctvTagFx.time > kCctvTagZoomFxTime)
					{
						spinf->cctvTagFx.ent = 0;
						spinf->cctvTagFx.time = 0.f;
					}
				}
			}
		}
	}
}

// -----------------------------------------------------------------------
EntityId CGameRulesMPSpectator::FindEntityUnderCCTVCursor()
{
	EntityId  underEnt = 0;

	IActor*  pClient = g_pGame->GetIGameFramework()->GetClientActor();

	if (CPlayer* pPlayer=((pClient && pClient->IsPlayer()) ? static_cast< CPlayer* >( pClient ) : NULL))
	{
		if (pPlayer->GetSpectatorMode() == CActor::eASM_CCTV)
		{
			IPersistantDebug  *pDebug = g_pGame->GetIGameFramework()->GetIPersistantDebug();
			pDebug->Begin("CCTVEnemyPos", false);

			if (CGameRules* pGameRules=static_cast< CGameRules* >( gEnv->pGame->GetIGameFramework()->GetIGameRulesSystem()->GetCurrentGameRules() ))
			{
				IActorSystem  *pActorSystem = gEnv->pGame->GetIGameFramework()->GetIActorSystem();

				int  friendlyTeam = pGameRules->GetTeam(pPlayer->GetEntity()->GetId());

				int  teamCount = pGameRules->GetTeamCount();
				for (int i = 1; i<=teamCount; i++)
				{
					if (i == friendlyTeam)
					{
						continue;
					}

					CGameRules::TPlayers  opposingPlayers;
					pGameRules->GetTeamPlayers(i, opposingPlayers);

					CGameRules::TPlayers::const_iterator  it = opposingPlayers.begin();
					CGameRules::TPlayers::const_iterator  end = opposingPlayers.end();
					for(; it!=end; ++it)
					{
						CPlayer  *loopPlr = static_cast<CPlayer*>(pActorSystem->GetActor(*it));

						if (!loopPlr)
						{
							//CryLog(" > skipping entity because its actor is NULL");
							continue;
						}

						IEntity  *loopEnt = loopPlr->GetEntity();
						assert(loopEnt);

						// [tlh] this check should be redundant seems we're explicitly only iterating through opposing teams, but I'll leave it here just for sanity's sake
						if(pGameRules->GetTeam(loopEnt->GetId()) == friendlyTeam)
						{
							//CryLog(" > skipping entity '%s', same team", loopEnt->GetName());
							continue;
						}

						if(!loopPlr || (loopPlr->GetHealth() <= 0))
						{
							//CryLog(" > skipping entity '%s', dead apparently", loopEnt->GetName());
							continue;				
						}

						if(loopPlr->GetSpectatorMode() != CActor::eASM_None)
						{
							//CryLog(" > skipping entity '%s', spectating (mode %d) apparently", loopEnt->GetName(), loopPlr->GetSpectatorMode());
							continue;
						}

						AABB  loopWBounds;
						loopEnt->GetWorldBounds( loopWBounds );

						SScreenBounds  boundsScreen;
						boundsScreen.min.x = 1000000.f;
						boundsScreen.min.y = 1000000.f;
						boundsScreen.max.x = -1000000.f;
						boundsScreen.max.y = -1000000.f;

						int  numOnScreen = 0;
						numOnScreen += ProjectBoundsPointToScreen(loopWBounds.min.x, loopWBounds.min.y, loopWBounds.min.z, &boundsScreen);
						numOnScreen += ProjectBoundsPointToScreen(loopWBounds.max.x, loopWBounds.min.y, loopWBounds.min.z, &boundsScreen);
						numOnScreen += ProjectBoundsPointToScreen(loopWBounds.min.x, loopWBounds.max.y, loopWBounds.min.z, &boundsScreen);
						numOnScreen += ProjectBoundsPointToScreen(loopWBounds.min.x, loopWBounds.min.y, loopWBounds.max.z, &boundsScreen);
						numOnScreen += ProjectBoundsPointToScreen(loopWBounds.max.x, loopWBounds.max.y, loopWBounds.min.z, &boundsScreen);
						numOnScreen += ProjectBoundsPointToScreen(loopWBounds.max.x, loopWBounds.min.y, loopWBounds.max.z, &boundsScreen);
						numOnScreen += ProjectBoundsPointToScreen(loopWBounds.min.x, loopWBounds.max.y, loopWBounds.max.z, &boundsScreen);
						numOnScreen += ProjectBoundsPointToScreen(loopWBounds.max.x, loopWBounds.max.y, loopWBounds.max.z, &boundsScreen);

						if (numOnScreen <= 0)
						{
							//CryLog(" > skipping entity '%s', no projected bounds points onscreen", loopEnt->GetName());
							continue;
						}

						if (boundsScreen.max.x > 100)
							boundsScreen.max.x = 100;
						if (boundsScreen.min.x < 0)
							boundsScreen.min.x = 0;

						if (boundsScreen.max.y > 100)
							boundsScreen.max.y = 100;
						if (boundsScreen.min.y < 0)
							boundsScreen.min.y = 0;

#if 0
						ColorF  col( 1.f, 0.f, 0.8f );
#define SCRX(x)  ((x/100.f)*(400-1))
#define SCRY(y)  ((y/100.f)*(300-1))
						pDebug->Add2DLine(SCRX(boundsScreen.min.x), SCRY(boundsScreen.min.y), SCRX(boundsScreen.max.x), SCRY(boundsScreen.min.y), col, 20.f);
						pDebug->Add2DLine(SCRX(boundsScreen.max.x), SCRY(boundsScreen.min.y), SCRX(boundsScreen.max.x), SCRY(boundsScreen.max.y), col, 20.f);
						pDebug->Add2DLine(SCRX(boundsScreen.max.x), SCRY(boundsScreen.max.y), SCRX(boundsScreen.min.x), SCRY(boundsScreen.max.y), col, 20.f);
						pDebug->Add2DLine(SCRX(boundsScreen.min.x), SCRY(boundsScreen.max.y), SCRX(boundsScreen.min.x), SCRY(boundsScreen.min.y), col, 20.f);
#undef SCRX
#undef SCRY
#endif

						Vec2  crosshair( 50.f, 50.f );  // [tlh] TODO the CCTV crosshair will eventually need to work unanchored from the centre of the screen...

						if ((crosshair.x < boundsScreen.min.x) || (crosshair.x > boundsScreen.max.x) || (crosshair.y < boundsScreen.min.y) || (crosshair.y > boundsScreen.max.y))
						{
							//CryLog(" > skipping entity '%s', crosshair not within its screen bounds (crosshair: %f, %f)", loopEnt->GetName(), crosshair.x, crosshair.y);
							continue;
						}

						EntityId  cctvEid = pPlayer->GetSpectatorCCTVCam();
						assert(cctvEid);
						IEntity  *cctvEnt = gEnv->pEntitySystem->GetEntity(cctvEid);
						assert(cctvEnt);

						Vec3  loopCPos = loopWBounds.GetCenter();
						Vec3  cctvPos = cctvEnt->GetWorldPos();
						Vec3  rayOffs = loopCPos - cctvPos;

						IPhysicalWorld  *pWorld = gEnv->pPhysicalWorld;
						ray_hit  hit;

						// note: the 8 in the RayWorldIntersection call below is apparently piercability, something to do with passing through certain object types - 8 apparently means it won't
						if (pWorld->RayWorldIntersection(cctvPos, rayOffs, ent_all, (geom_colltype_ray<<rwi_colltype_bit)|rwi_colltype_any|rwi_force_pierceable_noncoll|rwi_ignore_solid_back_faces|8, &hit, 1))
						{
							CryLog(" > ray hit collider with foreign data = %d", (hit.pCollider?hit.pCollider->GetiForeignData():-1));
							//pDebug->AddSphere(hit.pt, 0.15f, ColorF(0.f,1.f,1.f), 15.f);

							bool  rayHitLoopEnt = false;

							IEntity  *hitEnt = NULL;
							if (hit.pCollider && (hit.pCollider->GetiForeignData() == PHYS_FOREIGN_ID_ENTITY))
							{
								hitEnt = (IEntity*) hit.pCollider->GetForeignData(PHYS_FOREIGN_ID_ENTITY);
								if (hitEnt)
								{
									rayHitLoopEnt = (hitEnt == loopEnt);
								}
							}

							if (!rayHitLoopEnt)
							{
								//CryLog(" > skipping entity '%s', ray hit something else ('%s')", loopEnt->GetName(), (hitEnt?hitEnt->GetName():"[not ent]"));
								continue;
							}
						}

						underEnt = loopEnt->GetId();
						break;
					}

					if (underEnt)
					{
						break;
					}
				}
			}
		}
	}

	return underEnt;
}

// -----------------------------------------------------------------------
void CGameRulesMPSpectator::HandleCCTVTaggingAction()
{
	CryLog("[tlh] CGameRulesMPSpectator::PHandleCCTVTaggingActionostUpdateSpectatorCCTVTagging: processing");

	IActor*  pClient = g_pGame->GetIGameFramework()->GetClientActor();
	CPlayer*  pPlayer = ((pClient && pClient->IsPlayer()) ? static_cast< CPlayer* >( pClient ) : NULL);

	if (pPlayer && (pPlayer->GetSpectatorMode() == CActor::eASM_CCTV))
	{
		SSpectatorInfo*  spinf = &(((SPlayerStats*)pPlayer->GetActorStats())->spectatorInfo);
		USpectatorModeData::SCctv*  cctvd = &spinf->dataU.cctv;

		enum ETagFeedback
		{
			kTagFeedback_repeat,
			kTagFeedback_miss,
			kTagFeedback_tagged,
			kTagFeedback_cannotTag
		};

		ETagFeedback feedback = kTagFeedback_miss;

		if (cctvd->entUnderCursor)
		{
			if (cctvd->entUnderCursor != spinf->cctvTagFx.ent)  // the != check is to stop spam tagging of same player during effects
			{
				IActor * pActor = g_pGame->GetIGameFramework()->GetIActorSystem()->GetActor(cctvd->entUnderCursor);
				if (pActor && pActor->IsPlayer() && ((CPlayer*)pActor)->IsPerkActive(ePerk_RadarJammer))
				{
					feedback = kTagFeedback_cannotTag;
				}
				else
				{
					feedback = kTagFeedback_tagged;

					//pDebug->AddSphere(loopCPos, 0.25f, ColorF(1.f,0.f,0.8f), 2.f);  // [tlh] temporary debug gfx to indicate a successful tag

					CryLog(" > entity '%s' is a HIT!!", /*->GetName()*/"?");

					if (CGameRules* pGameRules=static_cast< CGameRules* >( gEnv->pGame->GetIGameFramework()->GetIGameRulesSystem()->GetCurrentGameRules() ))
					{
						pGameRules->RequestAddTempRadarEntity(pPlayer->GetEntity()->GetId(), cctvd->entUnderCursor, kTagTime);
					}

					spinf->cctvTagFx.ent = cctvd->entUnderCursor;
					spinf->cctvTagFx.time = 0.f;
				}
			}
			else
			{
				feedback = kTagFeedback_repeat;
			}
		}

		switch (feedback)
		{
			case kTagFeedback_miss:
			case kTagFeedback_cannotTag:
			// [Tomas] TODO please avoid hardcoded sound references, use Game Audio Signal System instead
			if (_smart_ptr<ISound> pSound = gEnv->pSoundSystem->CreateSound("Sounds/crysiswars2:interface:multiplayer/mp_cctv_tag_fail_fp", FLAG_SOUND_EVENT))  // [tlh] TODO this sound needs to be specified in data somewhere
			{
				pSound->SetSemantic(eSoundSemantic_HUD);
				pSound->Play();
			}
			break;

			case kTagFeedback_tagged:
			// [Tomas] TODO please avoid hardcoded sound references, use Game Audio Signal System instead
			if (_smart_ptr<ISound> pSound = gEnv->pSoundSystem->CreateSound("Sounds/crysiswars2:interface:multiplayer/mp_cctv_tag_success_fp", FLAG_SOUND_EVENT))  // [tlh] TODO this sound needs to be specified in data somewhere
			{
				pSound->SetSemantic(eSoundSemantic_HUD);
				pSound->Play();
			}
			break;
		}
	}
}


#undef SScreenBounds

