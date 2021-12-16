/************************************************************************/
/* UI BattleLog for Component HUD, Tim Furnish, 2009										*/
/************************************************************************/

#include "StdAfx.h"
#include "UIBattleLog.h"
#include "GameCVars.h"
#include "HUD/HUD.h"
#include "HUD/ScreenLayoutManager.h"
#include "Graphics/2DRenderUtils.h"
#include "IViewSystem.h"
#include "Game.h"
#include "GameRules.h"
#include "GameRulesTypes.h"
#include "Utility/StringUtils.h"
#include "PerkDbgDisplay.h"
#include "PlayerProgression.h"

#if ( defined(USER_timf) || defined(USER_frankh) ) && !defined(_RELEASE)
#define BATTLE_LOG_DEBUGGING		1
#else
#define BATTLE_LOG_DEBUGGING		0
#endif

#if !defined(_RELEASE)
static AUTOENUM_BUILDNAMEARRAY(s_listNames, MessageListIDs);
#endif

//--------------------------------------------------------------------------------
CUIBattleLog::CUIBattleLog( )
{
#if BATTLE_LOG_DEBUGGING
	CryLogAlways ("BATTLELOG: Being created now!");
#endif

	// TOD : register for events
	CHUD::AddHUDEventListener( this, "OnEnterGame_RemotePlayer" );
	CHUD::AddHUDEventListener( this, "OnLeaveGame_RemotePlayer" );
	CHUD::AddHUDEventListener( this, "OnNewBattleLogMessage" );
	CHUD::AddHUDEventListener( this, "OnPlayerPromotion" );
}

CUIBattleLog::~CUIBattleLog()
{
#if BATTLE_LOG_DEBUGGING
	CryLogAlways ("BATTLELOG: Being destroyed now!");
#endif

	if (gEnv->pConsole)
	{
		gEnv->pConsole->RemoveCommand("hud_testBattleLog");
	}

	CHUD::RemoveHUDEventListener(this);
}

void CUIBattleLog::Initialize( const IItemParamsNode* xmlElement, IUIElement* parent )
{
	TBattleLogParent::Initialize(xmlElement, parent);

	for (int i = 0; i < xmlElement->GetChildCount(); ++ i)
	{
		const IItemParamsNode * child = xmlElement->GetChild(i);
		const char * childTypeName = child->GetName();
		if (0 == stricmp (childTypeName, "KillMessages"))
		{
			CreateMessageDefListFromXML(child, kMessageList_kill);
		}
		else if (0 == strcmp (childTypeName, "SuicideMessages"))
		{
			CreateMessageDefListFromXML(child, kMessageList_suicide);
		}
	}

	REGISTER_COMMAND("hud_testBattleLog", CmdTestBattleLog, 0, "Add a test message to the battle log");
}

//--------------------------------------------------------------------------------
void CUIBattleLog::CreateMessageDefListFromXML(const IItemParamsNode * xmlElement, EMessageListID listID)
{
	CRY_ASSERT_MESSAGE(m_messageDefinitions[listID].empty(), string().Format("Message list #%d defined twice!", listID));
	m_messageDefinitions[listID].reserve(xmlElement->GetChildCount());

	const int childCount = xmlElement->GetChildCount();
	for (int j = 0; j < childCount; ++ j)
	{
		const IItemParamsNode * child = xmlElement->GetChild(j);
		const char * typeName = child->GetName();
		if (0 == stricmp (typeName, "Message"))
		{
			CreateMessageDefFromXML(child, listID);
		}
		else
		{
			CRY_ASSERT_MESSAGE(false, string().Format("Unexpected tag '%s' inside message list #%d!", typeName, listID));
		}
	}
}

//--------------------------------------------------------------------------------
void CUIBattleLog::CreateMessageDefFromXML(const IItemParamsNode * xmlElement, EMessageListID listID)
{
	SMessageDef messageDef;

	const char * strHitType = xmlElement->GetAttribute("hitType");
	const char * strDisplay = xmlElement->GetAttribute("display");
	if (strDisplay)
	{
		cry_strncpy(messageDef.type, strHitType ? strHitType : "normal", sizeof(messageDef.type));
		cry_strncpy(messageDef.msg,  strDisplay, sizeof(messageDef.msg));
		m_messageDefinitions[listID].push_back(messageDef);

#if BATTLE_LOG_DEBUGGING
		CryLogAlways ("BATTLELOG: [List %d] Adding '%s'=>'%s', list size is now %d", listID, messageDef.type, messageDef.msg, m_messageDefinitions[listID].size());
#endif
	}
}

const CUIBattleLog::SMessageDef * CUIBattleLog::PickMessageDef(EMessageListID listID, const char * hitType)
{
	const int k_maxPossibles = 16;
	const SMessageDef * possibles[k_maxPossibles];
	int numPossibles = 0;

	for (std::vector<SMessageDef>::iterator it = m_messageDefinitions[listID].begin(); numPossibles < k_maxPossibles && it != m_messageDefinitions[listID].end(); ++ it)
	{
		SMessageDef * entry = &(*it);
		if (0 == stricmp (hitType, entry->type))
		{
			possibles[numPossibles] = entry;
			++ numPossibles;
		}
	}

	if (numPossibles > 0)
	{
		return possibles[cry_rand32() % numPossibles];
	}

#if !defined(_RELEASE)
	CryWarning(VALIDATOR_MODULE_GAME, VALIDATOR_WARNING, "<Battlelog> No message defined in list '%s' for hit type '%s'", s_listNames[listID], hitType);
#endif

	return NULL;
}

const CUIBattleLog::SMessageDef * CUIBattleLog::PickMessageDefIncludeDefault(EMessageListID listID, const char * hitType)
{
	const SMessageDef * reply = (hitType && hitType[0]) ? PickMessageDef(listID, hitType) : NULL;
	return reply ? reply : PickMessageDef(listID, "normal");
}

void CUIBattleLog::AppendGeneratedMessageToLog(const char * message)
{
	if (g_pGame->GetIGameFramework()->GetClientActorId())
	{
		float baseTime = 4.f /*g_pGameCVars->ui2_battle_log_time*/;
		SBattleLogInfo bi;
		bi.msg  = message;
		bi.timeLeft = baseTime + (0.2f * m_player_battle_log.size());
		bi.timeUntilRevealNextCharacter = 0.f;
		bi.numCharactersRevealed = 0;

		m_player_battle_log.reserve(m_player_battle_log.size() + 1 );
		m_player_battle_log.push_back( bi );

#if BATTLE_LOG_DEBUGGING
		CryLogAlways ("BATTLELOG: \"%s\" added", message);
#endif
	}
	else
	{
#if BATTLE_LOG_DEBUGGING
		CryLogAlways ("BATTLELOG: \"%s\" not added as there's no local player (so clearing list of %d items instead)", message, m_player_battle_log.size());
#endif
		m_player_battle_log.clear();
	}
}

int CUIBattleLog::GetLocalPlayerTeam()
{
	return g_pGame->GetGameRules() ? g_pGame->GetGameRules()->GetTeam(g_pGame->GetIGameFramework()->GetClientActorId()) : 0;
}

char CUIBattleLog::GetInkColourForEntity(EntityId entityID, int localPlayerTeam)
{
	char colourCode = '1';

	if (localPlayerTeam != 0)
	{
		int entityTeam = g_pGame->GetGameRules()->GetTeam(entityID);
		if (entityTeam != 0)
		{
			colourCode = (entityTeam != localPlayerTeam) ? '4' : '2';
		}
	}

	return colourCode;
}

IEntity * CUIBattleLog::GetEntityForMessage(EntityId eid)
{
	IEntity * pWho = gEnv->pEntitySystem->GetEntity(eid);
	return (pWho && 0 == strcmp(pWho->GetClass()->GetName(), "Player")) ? pWho : NULL;
}

void CUIBattleLog::NewJoinMessage(EntityId eid)
{
	if (IEntity * pWho = GetEntityForMessage(eid))
	{
		// TODO: Localisation support!
		AppendGeneratedMessageToLog(string().Format("Player $%c%s$o has joined the game!", GetInkColourForEntity(eid, GetLocalPlayerTeam()), pWho->GetName()));
	}
}

void CUIBattleLog::NewLeaveMessage(EntityId eid)
{
	if (IEntity * pWho = GetEntityForMessage(eid))
	{
		// TODO: Localisation support!
		AppendGeneratedMessageToLog(string().Format("Player $%c%s$o has left the game!", GetInkColourForEntity(eid, GetLocalPlayerTeam()), pWho->GetName()));
	}
}

void CUIBattleLog::NewPromotedMessage(EntityId eid, uint8 rank, const char* name)
{
	if (IEntity * pWho = GetEntityForMessage(eid))
	{
		// TODO: Localisation support!
		AppendGeneratedMessageToLog(string().Format("Player $%c%s$o has been Promoted to Rank %d - %s!", GetInkColourForEntity(eid, GetLocalPlayerTeam()), pWho->GetName(), rank + 1, name));
	}
}

void CUIBattleLog::NewBattleLogMessage(const SBattleLogMessageInfo & info)
{
	IEntity *pTarget = GetEntityForMessage(info.targetId);

	if (pTarget)
	{
		CGameRules *pGameRules=g_pGame->GetGameRules();
		CRY_ASSERT(pGameRules);

		EMessageListID listID = (info.shooterId == info.targetId) ? kMessageList_suicide : kMessageList_kill;
		IEntity *pShooter = GetEntityForMessage(info.shooterId);
		const char * hitTypeString = pGameRules->GetHitType(info.hitType);

		if (pShooter == NULL)
		{
#if !defined(_RELEASE)
			CryWarning (VALIDATOR_MODULE_GAME, VALIDATOR_WARNING, "<Battlelog> Reporting the death of %s (list='%s' type='%s') but shooter is NULL or isn't a player! Treating this death as a suicide!", s_listNames[listID], hitTypeString, pTarget->GetName());
#endif
			pShooter = pTarget;
			listID = kMessageList_suicide;
		}

		const SMessageDef * messageDef = PickMessageDefIncludeDefault(listID, hitTypeString);

#if BATTLE_LOG_DEBUGGING
		CryLogAlways ("BATTLELOG: %s killed %s (list='%s' type='%s'), picked message %p", pShooter->GetName(), pTarget->GetName(), s_listNames[listID], hitTypeString, messageDef);
#endif

		if (messageDef)
		{
			int localPlayerTeam = GetLocalPlayerTeam();
			char shooterColourCode = GetInkColourForEntity(info.shooterId, localPlayerTeam);
			char targetColourCode = GetInkColourForEntity(info.targetId, localPlayerTeam);
			string message;

			if (listID == kMessageList_kill)
			{
				message.Format(messageDef->msg, string().Format("$%c%s$o", shooterColourCode, pShooter->GetName()).c_str(), string().Format("$%c%s$o", targetColourCode, pTarget->GetName()).c_str());
			}
			else
			{
				message.Format(messageDef->msg, string().Format("$%c%s$o", shooterColourCode, pShooter->GetName()).c_str());
			}

			AppendGeneratedMessageToLog(message);
		}
	}
}

void CUIBattleLog::Update(float frameTime)
{
	std::vector<SBattleLogInfo>::iterator it = m_player_battle_log.begin();

	while( it != m_player_battle_log.end() )
	{
		SBattleLogInfo * entry = &(*it);

		entry->timeLeft -= frameTime;
		if(entry->timeLeft <= 0.f)
		{
			it = m_player_battle_log.erase( it );
		}
		else
		{
			const float k_charactersRevealedPerSecond = 30.f;
			entry->timeUntilRevealNextCharacter -= frameTime * k_charactersRevealedPerSecond;
			while (entry->timeUntilRevealNextCharacter <= 0.f && entry->msg.c_str()[entry->numCharactersRevealed])
			{
				if (entry->msg.c_str()[entry->numCharactersRevealed] == '$' && entry->msg.c_str()[entry->numCharactersRevealed + 1] != '\0')
				{
					entry->numCharactersRevealed += 2;
				}
				else
				{
					++ (entry->numCharactersRevealed);
					entry->timeUntilRevealNextCharacter += 1.f;
				}
			}
			++it;
		}
	}

	TBattleLogParent::Update(frameTime);
}

void CUIBattleLog::Draw(void) const
{
	std::vector<SBattleLogInfo>::const_iterator it = m_player_battle_log.begin();
	float xPos = GetPosX();
	float yPos = GetPosY();
	const float k_size = GetHeight();
	const float k_gap = GetWidth();
	const float k_fadeOutTime = 0.5f;
	const float k_scrollUpTime = 0.7f;

	CryFixedStringT<64> battleString;
	C2DRenderUtils* pRenderUtils = g_pGame->GetHUD()->Get2DRenderUtils();
	ScreenLayoutManager* pLayout = g_pGame->GetHUD()->GetLayoutManager();
	const ScreenLayoutStates prevLState = pLayout->GetState();
	pLayout->SetState( eSLO_AdaptToSafeArea|eSLO_ScaleMethod_WithY );

	while (it != m_player_battle_log.end())
	{
		CRY_ASSERT((*it).timeLeft >= 0.f);

		if ((*it).timeLeft <= k_scrollUpTime)
		{
			float vanishFraction = (*it).timeLeft / k_scrollUpTime;
			yPos += k_gap * HUDCurveFraction(vanishFraction);
		}
		else
		{
			float alpha = MIN(1.f, ((*it).timeLeft - k_scrollUpTime) / k_fadeOutTime);
			battleString = (*it).msg.substr(0, (*it).numCharactersRevealed).c_str();
			pRenderUtils->DrawText(xPos, yPos, k_size, k_size, battleString.c_str(), ColorF(1.f, 1.f, 1.f, HUDCurveFraction(alpha)) /*, UIDRAWHORIZONTAL_LEFT */);
			yPos += k_gap;
		}

		++ it;
	}

	pLayout->SetState( prevLState );
	return;
}

void CUIBattleLog::CmdTestBattleLog(IConsoleCmdArgs* pCmdArgs)
{
	SBattleLogMessageInfo info;
	info.shooterId = g_pGame->GetIGameFramework()->GetClientActorId();
	info.targetId = info.shooterId;

	const EntityId localPlayerId = g_pGame->GetIGameFramework()->GetClientActorId();

	if (pCmdArgs->GetArgCount() > 1)
	{
		const int numArgs = pCmdArgs->GetArgCount();
		for (int i = 1; i < numArgs; ++ i)
		{
			SHUDEvent battleLogEvent(eHUDEvent_OnNewBattleLogMessage);
			battleLogEvent.ReserveData(3);
			battleLogEvent.AddData( atoi(pCmdArgs->GetArg(i)) );
			battleLogEvent.AddData( static_cast<int>(localPlayerId) );
			battleLogEvent.AddData( static_cast<int>(localPlayerId) );
			CHUD::CallEvent(battleLogEvent);
		}
	}
	else
	{
		const int hitType = cry_rand32() % 32;

		SHUDEvent battleLogEvent(eHUDEvent_OnNewBattleLogMessage);
		battleLogEvent.ReserveData(3);
		battleLogEvent.AddData( hitType );
		battleLogEvent.AddData( static_cast<int>(localPlayerId) );
		battleLogEvent.AddData( static_cast<int>(localPlayerId) );
		CHUD::CallEvent(battleLogEvent);
	}
}

void CUIBattleLog::OnHUDEvent(const SHUDEvent& event)
{
	switch( event.eventType )
	{
	case eHUDEvent_OnEnterGame_RemotePlayer :
		{
			const EntityId playerId = static_cast<EntityId>(event.GetData(0).GetInt());
			NewJoinMessage( playerId );
		}
		break;
	case eHUDEvent_OnLeaveGame_RemotePlayer :
		{
			const EntityId playerId = static_cast<EntityId>(event.GetData(0).GetInt());
			NewLeaveMessage( playerId );
		}
		break;
	case eHUDEvent_OnNewBattleLogMessage :
		{
			SBattleLogMessageInfo info;
			info.hitType = event.GetData(0).GetInt();
			info.shooterId = static_cast<EntityId>(event.GetData(1).GetInt());
			info.targetId = static_cast<EntityId>(event.GetData(2).GetInt());
			NewBattleLogMessage(info);
		}
		break;
	default : //case eHUDEvent_OnPlayerPromotion :
		{
			CRY_ASSERT( eHUDEvent_OnPlayerPromotion == event.eventType );
			const EntityId playerId = static_cast<EntityId>(event.GetData(0).GetInt());
			const int newRank = event.GetData(1).GetInt();
			const char* rankName = CPlayerProgression::GetInstance()->GetRankName(newRank);
			NewPromotedMessage( playerId, newRank, rankName );
		}
		break;
	}
}