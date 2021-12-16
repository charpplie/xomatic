/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios
-------------------------------------------------------------------------
History:
- 07:12:2009		Created by Ben Parbury
*************************************************************************/

#include "StdAfx.h"
#include "PlayerProgression.h"
#include "SuitModeProgression.h"
#include "SkillKill.h"

#include "Player.h"
#include "NanoSuit_v2.h"

#include "GameRules.h"
#include "GameRulesModules/IGameRulesStateModule.h"

#include <IPlayerProfiles.h>
#include "IFlashPlayer.h"

#include "Menus/OptionsManager.h"

#include "HUD/HUD.h"
#include "HUD/UI/UIButtonPromptRegion.h"

#include "Utility/StringUtils.h"
#include "Utility/CryWatch.h"
#include "Utility/DesignerWarning.h"
#include "StringUtils.h"

#include "Audio/AudioSignalPlayer.h"

CPlayerProgression* CPlayerProgression::s_playerProgression_instance = NULL;

static AUTOENUM_BUILDNAMEARRAY(s_eventName, PlayerProgressionType);
static int pp_debug = 0;
static int pp_defaultUnlockAll = 0;

CPlayerProgression::CPlayerProgression()
{
	m_ranks.clear();
	m_unlocks.clear();

	memset(m_events, 0, sizeof(m_events));

	for(int i = 0; i < eNanoSuitMode_Last; i++)
	{
		m_suitProgression[i] = new CSuitModeProgression();
		m_time[i] = 0.0f;
	}

	m_time[k_deadTime] = 0.0f;

	m_xp = 0;
	m_gameStartXp = 0;
	m_rank = 0;
	m_gameStartRank = 0;
	m_matchBonus = 0;
	m_maxRank = 0;
	
	CRY_ASSERT(s_playerProgression_instance == NULL);
	s_playerProgression_instance = this;
}

CPlayerProgression::~CPlayerProgression()
{
	CRY_ASSERT(s_playerProgression_instance == this);
	s_playerProgression_instance = NULL;
}

//static
CPlayerProgression* CPlayerProgression::GetInstance()
{
	CRY_ASSERT(s_playerProgression_instance);
	return s_playerProgression_instance;	
}

void CPlayerProgression::Init()
{
	if(g_pGameCVars->g_EPD)
	{
		InitRanks("Scripts/Progression/EPDRanks.xml");
	}
	else
	{
		InitRanks("Scripts/Progression/Ranks.xml");
	}
	
	InitEvents("Scripts/Progression/Events.xml");

	if(g_pGameCVars->g_EPD)
	{
		CSuitModeProgression::InitSharedSuitLevels("Scripts/Progression/Levels.xml");
	}
	else
	{
		CSuitModeProgression::InitSharedSuitLevels("Scripts/Progression/EPDLevels.xml");
	}

	InitFromProfile();	//after InitRanks and SuitLevels

	InitConsoleCommands();
}

void CPlayerProgression::InitFromProfile()
{
	IPlayerProfileManager *pProfileMan = g_pGame->GetOptions()->GetProfileManager();
	if ( pProfileMan )
	{
		IPlayerProfile *pProfile = pProfileMan->GetCurrentProfile(pProfileMan->GetCurrentUser());
		if(pProfile)
		{
			pProfile->GetAttribute("MP/PlayerProgression/XP", m_xp);
			m_gameStartXp = m_xp;
			m_rank = CalculateRankFromXp(m_xp);
			m_gameStartRank = m_rank;

			for(int i = 0; i < eNanoSuitMode_Last; i++)
			{
				int m_suitXp = 0;
				string name;
				name.Format("MP/PlayerProgression/%sXP", CNanoSuit::GetNanoSuitModeName(i));
				pProfile->GetAttribute(name.c_str(), m_suitXp);
				m_suitProgression[i]->Init(m_suitXp);
			}
		}
	}
}

void CPlayerProgression::SaveProfile()
{
	COptionsManager* pOptionsMgr = g_pGame->GetOptions();
	IPlayerProfileManager *pProfileMan = pOptionsMgr->GetProfileManager();
	if(pProfileMan)
	{
		IPlayerProfile *pProfile = pProfileMan->GetCurrentProfile(pProfileMan->GetCurrentUser());
		if(pProfile)
		{
			pProfile->SetAttribute("MP/PlayerProgression/XP", m_xp);

			for(int i = 0; i < eNanoSuitMode_Last; i++)
			{
				int m_suitXp = m_suitProgression[i]->GetData(ePPS_XP);
				string name;
				name.Format("MP/PlayerProgression/%sXP", CNanoSuit::GetNanoSuitModeName(i));
				pProfile->SetAttribute(name.c_str(), m_suitXp);
			}

			pOptionsMgr->SaveProfile();
		}
	}
}

int CPlayerProgression::CalculateRankFromXp(int xp)
{
	for(int i = 0; i < m_maxRank - 1; i++)
	{
		if(xp >= m_ranks[i].m_xpRequired && xp < m_ranks[i + 1].m_xpRequired)
		{
			return i;
		}
	}
/* //Diesel cut
	if(xp >= m_ranks[m_maxRank - 1].m_xpRequired)
	{
		return m_maxRank - 1;
	}

	CRY_ASSERT_MESSAGE(false, "Failed to CalculateRankFromXp"); */ //Diesel cut
	return 0;
}

void CPlayerProgression::InitRanks(const char* filename)
{
	XmlNodeRef xml = GetISystem()->LoadXmlFile( filename );

	if(xml)
	{
		const int childCount = xml->getChildCount();

		DesignerWarning(childCount < k_maxPossibleRanks, "There should be less than '%d' ranks not '%d'", k_maxPossibleRanks, childCount);
		m_maxRank = childCount;

		for (int iRank = 0; iRank < childCount; ++iRank)
		{
			XmlNodeRef childXML = xml->getChild(iRank);

			SRank rank(childXML);
			m_ranks.push_back(rank);

			const int unlockCount = childXML->getChildCount();
			for (int iUnlock = 0; iUnlock < unlockCount; ++iUnlock)
			{
				XmlNodeRef unlockNode = childXML->getChild(iUnlock);

				DesignerWarning( m_unlocks.size() != k_maxUnlocks, "Too many unlocks registered in %s!", filename );
				SUnlock unlock(unlockNode, iRank);
				m_unlocks.push_back(unlock);
			}
		}

		SanityCheckRanks();
	}
}

void CPlayerProgression::SanityCheckRanks()
{
	for(int i = 1; i < m_maxRank; i++)
	{
		DesignerWarning(m_ranks[i - 1].m_xpRequired < m_ranks[i].m_xpRequired, "Rank %d needs more xp than rank %d", i - 1, i);
	}
}

CPlayerProgression::SRank::SRank(XmlNodeRef node)
{
	CRY_ASSERT_MESSAGE(strcmp(node->getTag(), "Rank") == 0, "Invalid tag found in rank xml");
	CRY_ASSERT_MESSAGE(node->haveAttr("name"), "Missing name attribute in rank xml");
	CRY_ASSERT_MESSAGE(node->haveAttr("xpRequired"), "Missing xpRequired attribute in rank xml");

	cry_strncpy(m_name, node->getAttr("name"), k_maxNameLength);
	node->getAttr("xpRequired", m_xpRequired);
}

CPlayerProgression::SUnlock::SUnlock(XmlNodeRef node, int rank)
{
	CRY_ASSERT_MESSAGE(strcmpi(node->getTag(), "unlock") == 0, "Invalid tag found in rank xml");
	CRY_ASSERT_MESSAGE(node->haveAttr("name"), "Missing name attribute on unlock in rank xml");
	CRY_ASSERT_MESSAGE(node->haveAttr("type"), "Missing type attribute on unlock in rank xml");

	m_type = GetUnlockTypeFromName(node->getAttr("type"));
	cry_strncpy(m_name, node->getAttr("name"), k_maxNameLength);
	m_rank = rank;
}

//static
EUnlockType CPlayerProgression::GetUnlockTypeFromName(const char* name)
{
	if(strcmpi(name, "loadout") == 0)
	{
		return eUT_Loadout;
	}
	else if(strcmpi(name, "weapon") == 0)
	{
		return eUT_Weapon;
	}

	CRY_ASSERT_MESSAGE(false, ("Invalid unlock type found '%s'", name));
	return eUT_Invalid;
}

void CPlayerProgression::InitEvents(const char* filename)
{
	XmlNodeRef xml = GetISystem()->LoadXmlFile( filename );

	if(xml)
	{
		const int childCount = min(xml->getChildCount(), EPP_Max);
		for (int iChild = 0; iChild < childCount; ++iChild)
		{
			XmlNodeRef childXML = xml->getChild(iChild);

			CRY_ASSERT_MESSAGE(childXML->haveAttr("name"), "Missing name attribute in event xml");
			CRY_ASSERT_MESSAGE(childXML->haveAttr("reward"), "Missing reward attribute in event xml");

			CRY_ASSERT_MESSAGE(strcmp(s_eventName[iChild], childXML->getAttr("name")) == 0, string().Format("Expected %s as %th child, not %s", s_eventName[iChild], iChild, childXML->getAttr("name")));

			childXML->getAttr("reward", m_events[iChild]);
		}
	}
}
void CPlayerProgression::Event(EPPType type, bool skillKill)
{
	CRY_ASSERT_MESSAGE(type >= 0 && type < EPP_Max, "Invalid event type");

	EventInternal(m_events[type]);

	SHUDEvent xpHudEvent(eHUDEvent_OnNewXP);
	xpHudEvent.AddData(type);
	xpHudEvent.AddData(m_events[type]);
	CHUD::CallEvent(xpHudEvent);

	SendEventToListeners(type, skillKill);
}

void CPlayerProgression::EventInternal(int points)
{
	CGameRules *pGameRules=g_pGame->GetGameRules();
	if(!pGameRules)
		return;

	IGameRulesStateModule *pStateModule = pGameRules->GetStateModule();
	if (pStateModule && pStateModule->GetGameState() != IGameRulesStateModule::EGRS_InGame)
		return;

	//Only xp gained from external events counts towards team perks
	IActor* pActor = gEnv->pGame->GetIGameFramework()->GetClientActor();
	if(pActor && pActor->IsPlayer())
	{
		CPlayer* pPlayer = static_cast<CPlayer*>(pActor);
		pPlayer->SendPerkEvent(EPE_ClientGainedXP, &points);
	}

	IncrementXP(points);
}

bool CPlayerProgression::SkillKillEvent(CGameRules* pGameRules, IActor* pTargetActor, IActor* pShooterActor, const char* weaponClassName, int damage, int hit_joint, int hit_type)
{
	CRY_ASSERT(pGameRules);
	if(pTargetActor && pShooterActor &&	pTargetActor != pShooterActor)
	{
		CPlayer* pShooterPlayer = static_cast<CPlayer*>(pShooterActor);

		bool firstBlood = SkillKill::IsFirstBlood();	//need to update first blood locally for all actors

		if(pShooterActor->GetEntityId() == gEnv->pGame->GetIGameFramework()->GetClientActorId() && //only for local player
			!pShooterPlayer->IsFriendlyEntity(pTargetActor->GetEntityId()))		//not friendly target
		{
			CPlayer* pTargetPlayer = static_cast<CPlayer*>(pTargetActor);

			bool skillKill = false;

			if(firstBlood)
			{
				Event(EPP_FirstBlood, true);
				skillKill = true;
			}

#define SkillKillCheck(check, event) \
	if(SkillKill::check) \
			{ \
				Event(event, true); \
				skillKill = true; \
			}

			SkillKillCheck(IsKillJoy(pTargetPlayer), EPP_KillJoy);
			SkillKillCheck(IsBlindKill(pShooterPlayer), EPP_BlindKill);
			SkillKillCheck(IsRumbled(pTargetPlayer), EPP_Rumbled);
			SkillKillCheck(IsAirDeath(pTargetPlayer), EPP_AirDeath);
			SkillKillCheck(IsUnderwaterKill(pShooterPlayer), EPP_UnderwaterKill);
			SkillKillCheck(IsNearDeathExperience(pShooterPlayer), EPP_NearDeathExperience);
			if(SkillKill::IsMeleeTakedown(pGameRules, hit_type))
			{
				Event(EPP_MeleeTakedown, true);
				skillKill = true;
			}
			else
			{
				SkillKillCheck(IsPistolKill(weaponClassName), EPP_PistolKill);
				SkillKillCheck(IsHeadshot(pTargetPlayer, hit_joint), EPP_Headshot);
				SkillKillCheck(IsImpale(pGameRules, hit_type), EPP_Impale);
				SkillKillCheck(IsRoadKill(pGameRules, hit_type), EPP_RoadKill);
			}

			return skillKill;

#undef SkillKillCheck
		}
	}

	return false;
}

void CPlayerProgression::Update(CPlayer *pPlayer, float deltaTime, int iHealth)
{
#ifndef _RELEASE
	if(pp_debug > 0)
	{
		if(pp_debug == 1)
		{
			CryWatch("XP %d", m_xp);
			CryWatch("game start XP %d and rank %d", m_gameStartXp, m_gameStartRank);
			CryWatch("Rank %d - %s", m_rank, GetRankName(m_rank + 1));
			if(m_rank != m_maxRank - 1)
			{
				CryWatch("Next Rank %s in %d xp", GetRankName(m_rank + 2), m_ranks[m_rank+1].m_xpRequired - m_xp);
			}
		}
		else if(pp_debug == 2)
		{
			float totalTime = 0.0f;
			for(int i = 0; i < eNanoSuitMode_Last; i++)
			{
				totalTime += m_time[i];
			}

			if(totalTime >= 0.0f)
			{
				for(int i = 0; i < eNanoSuitMode_Last; i++)
				{
					CryWatch("Mode %s - %.2f", CNanoSuit::GetNanoSuitModeName(i), (m_time[i]/totalTime));
					m_suitProgression[i]->DebugWatch();
				}
			}
			CryWatch("Dead time - %.2f", m_time[k_deadTime]);
		}
		else if(pp_debug == 3)
		{
			const int unlockSize = m_unlocks.size();
			CryWatch("Unlocks %d", unlockSize);
			for(int i = 0; i < unlockSize; i++)
			{
				const SUnlock unlock = m_unlocks[i];
				int unlockValue = 0;
				bool exists = false;
				CryWatch("Unlock %s on rank %d type %d - %s", unlock.m_name, unlock.m_rank, unlock.m_type, HaveUnlocked(unlock.m_type, unlock.m_name, exists, unlockValue) ? "Unlocked" : "Locked");
			}
		}
	}
#endif

	CGameRules *pGameRules=g_pGame->GetGameRules();
	if(!pGameRules)
		return;

	IGameRulesStateModule *pStateModule = pGameRules->GetStateModule();
	if (pStateModule && pStateModule->GetGameState() != IGameRulesStateModule::EGRS_InGame)
		return;

	CRY_ASSERT_MESSAGE(pPlayer->GetEntityId() == gEnv->pGame->GetIGameFramework()->GetClientActorId() || gEnv->bHostMigrating, "CPlayerProgression::Update is happening on the wrong player entity!");

	if(iHealth > 0)
	{
		ENanoSuitMode mode =  pPlayer->GetActorSuitGameParameters().GetMode();
		CRY_ASSERT_MESSAGE(mode >= 0 && mode < eNanoSuitMode_Last, "Invalid suitmode from player");

		m_time[mode] += deltaTime;
	}
	else
	{
		m_time[k_deadTime] += deltaTime;
	}

}

void CPlayerProgression::IncrementXP(int amount)
{
	m_xp += amount;

	if(m_rank != m_maxRank - 1 && m_xp >= m_ranks[m_rank + 1].m_xpRequired)
	{
		m_rank = CalculateRankFromXp(m_xp);

		IActor* pActor = gEnv->pGame->GetIGameFramework()->GetClientActor();
		if(pActor && pActor->IsPlayer())
		{
			CPlayer* pPlayer = static_cast<CPlayer*>(pActor);
			CHANGED_NETWORK_STATE(pPlayer, CPlayer::ASPECT_RANK_CLIENT);
			
			const char* message = g_pGame->GetHUD()->LocalizeString("@pp_promoted", GetRankName(m_rank + 1));
			CUIButtonPromptRegion::SetOnScreenMessageText("gamerulesBigAnnouncements", message, NULL, 3.0f);
		}

		CAudioSignalPlayer::JustPlay("RankUp");

		CryLog("CPlayerProgression - Rank up to %s!", GetRankName(m_rank + 1));
	}
}

void CPlayerProgression::ClientScoreEvent(EGameRulesScoreType type, int points)
{
	//don't take away xp
	if(points > 0)
	{
		//hud event should already have been handled
		EventInternal(points);
	}
}

void CPlayerProgression::EnteredGame()
{
	m_gameStartXp = m_xp;
	m_gameStartRank = m_rank;
	m_matchBonus = 0;

	for(int i = 0; i < eNanoSuitMode_Last; i++)
	{
		m_suitProgression[i]->OnGameStart();
	}

	SkillKill::Reset();

	CGameRules *pGameRules=g_pGame->GetGameRules();
	pGameRules->RegisterClientScoreListener(this);
}

void CPlayerProgression::GameOver(EGameOverType localWinner)
{
	float totalTime = 0.0f;
	for(int i = 0; i < k_timeSlots; i++)
	{
		totalTime += m_time[i];
	}

	MatchBonus(localWinner, totalTime);

	CRY_ASSERT_MESSAGE(m_gameStartXp <= m_xp, "invalid game start xp");
	int xpGained = m_xp - m_gameStartXp;
	CryLog("%d xp gained this match", xpGained);

	float aliveTime = totalTime - m_time[k_deadTime];
	if(aliveTime > 0.0f)
	{
		for(int i = 0; i < eNanoSuitMode_Last; i++)
		{
			m_suitProgression[i]->OnGameEnd((int) (xpGained * (m_time[i]/aliveTime)));
		}
	}

	//reset time for next game
	for(int i = 0; i < k_timeSlots; i++)
	{
		m_time[i] = 0.0f;
	}

	SaveProfile();

	CGameRules *pGameRules=g_pGame->GetGameRules();
	pGameRules->UnRegisterClientScoreListener(this);

}

void CPlayerProgression::MatchBonus(const EGameOverType localWinner, const float totalTime)
{
	//length in game * win/draw/lose modifier * rank

	CGameRules *pGameRules=g_pGame->GetGameRules();
	if(pGameRules)
	{
		float totalGameTime = pGameRules->GetCurrentGameTime();
		float fractionOfGamePlayed = totalTime/totalGameTime;
		fractionOfGamePlayed = clamp(fractionOfGamePlayed, 0.0f, 1.0f);

		float winModifier = WinModifier(localWinner);
		float rankModifier = ((m_rank + 1)*0.25f) + 1.0f;			//divided by (rank/4) + 1
		float lengthBonus = totalGameTime*0.16666666666666f;	//divided by 60
		m_matchBonus = (int) (fractionOfGamePlayed * winModifier * rankModifier * lengthBonus);
		IncrementXP(m_matchBonus);
	}
}

float CPlayerProgression::WinModifier(const EGameOverType localWinner) const
{
	const static float k_WinModifier = 12.5f;
	const static float k_DrawModifier = 10.0f;
	const static float k_LoseModifier = 7.5f;

	switch(localWinner)
	{
	case EGOT_Win:
		return k_WinModifier;
	case EGOT_Draw:
		return k_DrawModifier;
	case EGOT_Lose:
		return k_LoseModifier;
	default:
		CRY_ASSERT_MESSAGE(false, "Unable to determine Win Modifier");
		return k_DrawModifier;
	}
}

void CPlayerProgression::InitConsoleCommands()
{
	IConsole * console = gEnv->pConsole;

	if (console)
	{
		console->AddCommand("pp_GainXP", CmdGainXP, VF_CHEAT, CVARHELP("increments your xp"));
		console->AddCommand("pp_GameEnd", CmdGameEnd, VF_CHEAT, CVARHELP("increments your xp and applies end of round bonus"));
		console->AddCommand("pp_UnlockAll", CmdUnlockAll, VF_CHEAT, CVARHELP("applies any unlocks gained immediately"));
		console->AddCommand("pp_UnlocksNow", CmdUnlocksNow, VF_CHEAT, CVARHELP("applies any unlocks gained immediately"));
		console->AddCommand("pp_ResetXP", CmdResetXP, VF_CHEAT, CVARHELP("resets xp to 0"));

		REGISTER_CVAR(pp_debug, 0, 0, "Enable/Disables player progression debug messages");
		REGISTER_CVAR(pp_defaultUnlockAll, 0, VF_CHEAT, "Can be used in cfg file to unlock everything");
	}
}

const int CPlayerProgression::GetData(EPPData dataType)
{
	switch(dataType)
	{
		//Ranks values go from 0 to m_maxRank - 1
		//However outside world should see 1 to m_maxRank
		case EPP_Rank:
		return m_rank + 1;

		case EPP_MaxRank:
		return m_maxRank;

		case EPP_XP:
		return m_xp;

		case EPP_XPToNextRank:
		return (m_rank != m_maxRank - 1) ? m_ranks[m_rank + 1].m_xpRequired - m_xp : 0;

		case EPP_XPLastMatch:
		return m_xp - m_gameStartXp;

		case EPP_MatchStartRank:
		return m_gameStartRank + 1;

		case EPP_MatchStartXPInCurrentRank:
		return m_gameStartXp - m_ranks[m_gameStartRank].m_xpRequired;

		case EPP_MatchStartXPToNextRank:
		return (m_gameStartRank != m_maxRank - 1) ? m_ranks[m_gameStartRank + 1].m_xpRequired - m_gameStartXp : 0;

		case EPP_MatchBonus:
		return m_matchBonus;

		case EPP_XPInCurrentRank:
		return m_xp - m_ranks[m_rank].m_xpRequired;

		case EPP_NextRank:
		return (m_rank != m_maxRank - 1) ? (m_rank + 2) : m_maxRank;

	default:
		CRY_ASSERT_MESSAGE(false, "Unable to find data type in CPlayerProgression::GetData");
		return -1;
	}
}

const int CPlayerProgression::GetSuitModeData(ENanoSuitMode suitMode, EPPSuitData dataType)
{
	if (suitMode >= 0 && suitMode < eNanoSuitMode_Last)
	{
		return m_suitProgression[(int)suitMode]->GetData(dataType);
	}
	else
	{
		CRY_ASSERT_MESSAGE(false, "Invalid suit mode CPlayerProgression::GetSuitModeData");
	}

	return -1;
}

const char* CPlayerProgression::GetRankName(uint8 rank)
{
	CRY_ASSERT(rank > 0 && rank <= m_maxRank);
	return g_pGame->GetHUD()->LocalizeString(m_ranks[rank - 1].m_name);
}

bool CPlayerProgression::HaveUnlocked(EUnlockType type, const char* name, bool &exists, int &unlockValue)
{
	exists = false;

#ifndef _RELEASE
	if(pp_defaultUnlockAll > 0)
	{
		return false;
	}
#endif

	const int unlockSize = m_unlocks.size();
	for(int i = 0; i < unlockSize; i++)
	{
		const SUnlock unlock = m_unlocks[i];
		if(strcmpi(unlock.m_name, name) == 0)
		{
			if(unlock.m_type == type)
			{
				exists = true;
				unlockValue = unlock.m_rank + 1;

				if (unlock.m_rank <= m_gameStartRank)
				{
					return true;
				}
				break;
			}
		}
	}

	return false;
}

// Get the unlock type by name to be handled by the flash for the correct localized string and icon to display.
const char * CPlayerProgression::GetUnlockTypeString(EUnlockType type)
{
	const char *result = "unknown";
	switch (type)
	{
	case eUT_Loadout:
		result = "loadout";
		break;
	case eUT_Weapon:
		result = "weapon";
		break;
	};

	return result;
}

void CPlayerProgression::SendUnlocksToFlash(IFlashPlayer *pFlashPlayer)
{
	if (!pFlashPlayer)
		return;

	CryFixedArray<SFlashUnlockInfo, 16> unlocksArray; // Fixed at 16 stats to be displayed, can be changed if more are required..

	int size = m_unlocks.size();
	for (int i(0); i<size; ++i)
	{
		const SUnlock &unlock = m_unlocks[i];

		if ((unlock.m_rank > m_gameStartRank) && (unlock.m_rank <= m_rank))
		{
			SFlashUnlockInfo unlockInfo;

			unlockInfo.m_name = unlock.m_name;
			unlockInfo.m_type = GetUnlockTypeString(unlock.m_type);
			unlockInfo.m_rank = CryStringUtils::toString((uint32)unlock.m_rank+1).c_str();

			unlocksArray.push_back(unlockInfo);
		}
	}

	size = unlocksArray.size();
	std::vector<const char*> pushArray;
	pushArray.reserve(size);
	for (int i(0); i<size; ++i)
	{
		const SFlashUnlockInfo &unlockInfo = unlocksArray[i];
		pushArray.push_back( unlockInfo.m_name );
		pushArray.push_back( unlockInfo.m_type.c_str() );
		pushArray.push_back( unlockInfo.m_rank.c_str() );
	}

	pFlashPlayer->SetVariableArray(FVAT_ConstStrPtr, "m_allValues", 0, &pushArray[0], pushArray.size());
	pFlashPlayer->Invoke0("SetEndMatchProgressionUnlocks");
}

void CPlayerProgression::ResetXP()
{
	m_xp = 0;
	m_gameStartXp = 0;
	m_rank = 0;
	m_gameStartRank = 0;
	m_matchBonus = 0;

	for(int i = 0; i < eNanoSuitMode_Last; i++)
	{
		m_suitProgression[i]->ResetXP();
	}

	IActor* pActor = gEnv->pGame->GetIGameFramework()->GetClientActor();
	if(pActor && pActor->IsPlayer())
	{
		CPlayer* pPlayer = static_cast<CPlayer*>(pActor);
		CHANGED_NETWORK_STATE(pPlayer, CPlayer::ASPECT_RANK_CLIENT);
	}
}

void CPlayerProgression::UpdateStartRank()
{
	m_gameStartRank = m_rank;
}

void CPlayerProgression::AddEventListener(IPlayerProgressionEventListener* pEventListener)
{
	stl::push_back_unique(m_eventListeners, pEventListener);
}

void CPlayerProgression::RemoveEventListener(IPlayerProgressionEventListener* pEventListener)
{
	stl::find_and_erase(m_eventListeners, pEventListener);
}

void CPlayerProgression::SendEventToListeners(EPPType type, bool skillKill)
{
	if (!m_eventListeners.empty())
	{
		TEventListener::iterator iter = m_eventListeners.begin();
		TEventListener::iterator end = m_eventListeners.end();
		for(; iter != end; ++iter)
		{
			(*iter)->OnEvent(type, skillKill);
		}
	}
}

//static
void CPlayerProgression::CmdGainXP(IConsoleCmdArgs* pCmdArgs)
{
	if(pCmdArgs->GetArgCount() == 2)
	{
		const char* number = pCmdArgs->GetArg(1);
		int xp = atoi(number);
		CryLogAlways("Incrementing %d", xp);
		CPlayerProgression::GetInstance()->IncrementXP(xp);
	}
	else
	{
		CryLogAlways("[Usage] pp_GainXP <amountOfXp>");
	}
}

//static
void CPlayerProgression::CmdGameEnd(IConsoleCmdArgs* pCmdArgs)
{
	if(pCmdArgs->GetArgCount() == 2)
	{
		const char* number = pCmdArgs->GetArg(1);
		int xp = atoi(number);
		CryLogAlways("Incrementing %d", xp);
		CPlayerProgression::GetInstance()->IncrementXP(xp);
		CPlayerProgression::GetInstance()->GameOver(EGOT_Win);
	}
	else
	{
		CryLogAlways("[Usage] pp_GameEnd <amountOfXp>");
	}
}

//static
void CPlayerProgression::CmdUnlockAll(IConsoleCmdArgs* pCmdArgs)
{
	CryLogAlways("Mid-game unlocked unlocks for rank %s", CPlayerProgression::GetInstance()->GetRankName(CPlayerProgression::GetInstance()->GetData(EPP_Rank)));
	CPlayerProgression::GetInstance()->IncrementXP(1000000);	//One Million XP!
	CPlayerProgression::GetInstance()->UpdateStartRank();
}

//static
void CPlayerProgression::CmdUnlocksNow(IConsoleCmdArgs* pCmdArgs)
{
	CryLogAlways("Mid-game unlocked unlocks for rank %s", CPlayerProgression::GetInstance()->GetRankName(CPlayerProgression::GetInstance()->GetData(EPP_Rank)));

	CPlayerProgression::GetInstance()->UpdateStartRank();
}

//static
void CPlayerProgression::CmdResetXP(IConsoleCmdArgs* pCmdArgs)
{
	CryLogAlways("Reset player xp");

	CPlayerProgression::GetInstance()->ResetXP();
}