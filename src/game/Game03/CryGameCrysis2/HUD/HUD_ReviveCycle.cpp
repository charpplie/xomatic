#include "StdAfx.h"
#include "HUD_ReviveCycle.h"
//#include "Flash/Flash.h"
#include "HUD.h"
//#include "HUD/HUD_Impl.h"
#include "IActorSystem.h"
//#include "Game.h"
#include "GameRules.h"
#include "Player.h"
//#include "FlashAnimation.h"

//------------------------------------------------------------------------
CHUD_ReviveCycle::CHUD_ReviveCycle()
{
	m_bVisible = false;

	m_remainingReviveCycleTime = -1.f;
}

//------------------------------------------------------------------------
CHUD_ReviveCycle::~CHUD_ReviveCycle()
{
}

//------------------------------------------------------------------------
void CHUD_ReviveCycle::Reload()
{
}

//------------------------------------------------------------------------
void CHUD_ReviveCycle::OnHUDEvent(const SHUDEvent& event)
{
	if(event.eventType == eHUDEvent_ShowReviveCycle)
	{
		IHUDAsset* reviveCycle = GetAsset("revivecycle");
		const bool show = (event.eventIntData != 0);
		m_bVisible = show;
		if (m_bVisible)
		{
			m_remainingReviveCycleTime = -1.f;
		}
	}
}

//------------------------------------------------------------------------
void CHUD_ReviveCycle::Init()
{
	IHUDAsset *reviveCycle = GetAsset("revivecycle");
	if (reviveCycle == CHUD::s_pHUDAssetNULL)
		return;
}

//------------------------------------------------------------------------
void CHUD_ReviveCycle::Update(float frameTime)
{
	if (!m_bVisible)
		return;

	IHUDAsset *reviveCycle = GetAsset("revivecycle");
	if (reviveCycle == CHUD::s_pHUDAssetNULL)
		return;

	CGameRules *pGameRules = g_pGame->GetGameRules();
	if (!pGameRules)
		return;

	CPlayer *pPlayer = static_cast<CPlayer *>(gEnv->pGame->GetIGameFramework()->GetClientActor());
	IActorSystem *pActorSystem = gEnv->pGame->GetIGameFramework()->GetIActorSystem();
	if(pActorSystem && pPlayer)
	{
		//get players that will spawn with you
		static std::vector<string> players;
		players.resize(0);

		int clientTeam = pGameRules->GetTeam(pPlayer->GetEntityId());
		EntityId spawnID = pGameRules->GetPlayerSpawnGroup(pPlayer);

		if (spawnID || pGameRules->AllowNullSpawnGroups())
		{
			static std::vector<EntityId> teamMates;
			teamMates.resize(0);

			pGameRules->GetTeamPlayers(clientTeam, teamMates);

			std::vector<EntityId>::const_iterator it = teamMates.begin();
			std::vector<EntityId>::const_iterator end = teamMates.end();

			players.reserve(teamMates.size());

			for(;it!=end;++it)
			{
				CActor *pTempActor = static_cast<CActor *>(pActorSystem->GetActor(*it));
				if(pTempActor && pTempActor != pPlayer)
				{
					if((pTempActor->GetHealth()<=0) && (pGameRules->GetPlayerSpawnGroup(pTempActor)==spawnID))
						players.push_back(pTempActor->GetEntity()->GetName());
				}
			}
		}

		int size = players.size();

		std::vector<const char*> pushArray;
		pushArray.reserve(size);
		const char**playerNameArray=NULL;

		if (size>0)
		{
			for (int i(0); i<size; ++i)
			{
				pushArray.push_back(players[i].c_str());
			}

			playerNameArray = &pushArray[0];
		}

		reviveCycle->GetFlashPlayer()->SetVariableArray(FVAT_ConstStrPtr, "m_players", 0, playerNameArray, size);
	}


	float remaining = pGameRules->GetRemainingReviveCycleTime();
	// Because of net lag, it can happens that the time value is reset from 0 g_revivetime
	// To prevent this, just ignore these values (it should be a matter of a second)
	if(-1.0f == m_remainingReviveCycleTime)
		m_remainingReviveCycleTime = remaining;
	if(remaining > m_remainingReviveCycleTime)
		remaining = 0.0f;
	else
		m_remainingReviveCycleTime = remaining;
	
	SFlashVarValue args[2] = {g_pGameCVars->g_revivetime, remaining};
	reviveCycle->Invoke("setSeconds", args, 2);

	reviveCycle->Update(frameTime);
}

//------------------------------------------------------------------------
void CHUD_ReviveCycle::Draw()
{
	if (m_bVisible)
	{
		GetAsset("revivecycle")->Draw();
	}
}