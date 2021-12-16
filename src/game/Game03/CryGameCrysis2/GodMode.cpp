#include "StdAfx.h"
#include "GodMode.h"
#include "Player.h"
#include "GameCVars.h"
#include "GameRules.h"

const float CGodMode::m_timeToWaitBeforeRespawn = 4.0f;
const char * CGodMode::m_godModeCVarName = "g_godMode";

CGodMode::CGodMode()
	: m_respawningFromDemiGodDeath(false)
	, m_elapsedTime(0.0f)
	, m_godMode(0)
{

}

CGodMode& CGodMode::GetInstance()
{
	static CGodMode godMode;
	return godMode;
}

void CGodMode::RegisterConsoleVars(IConsole* pConsole)
{
	pConsole->Register(m_godModeCVarName, &m_godMode, 0, VF_CHEAT, CVARHELP("God Mode"));
}

void CGodMode::UnregisterConsoleVars(IConsole* pConsole) const
{
	pConsole->UnregisterVariable(m_godModeCVarName, true);
}

EGodModeState CGodMode::GetCurrentState() const
{
	return (EGodModeState)m_godMode;
}

const char* CGodMode::GetCurrentStateString() const
{
	const char* res = "None";

	switch (GetCurrentState())
	{
	case eGMS_GodMode:
		res = "GodMode";
		break;
	case eGMS_TeamGodMode:
		res =  "TeamGodMode";
		break;
	case eGMS_DemiGodMode:
		res =  "DemiGodMode";
		break;
	}

	return res;
}

void CGodMode::MoveToNextState()
{
	m_godMode = (m_godMode + 1) % eGMS_LAST;
}

void CGodMode::DemiGodDeath()
{
	if (!gEnv->bMultiplayer && eGMS_DemiGodMode == GetCurrentState())
	{
		CActor* player = static_cast<CActor*>(g_pGame->GetIGameFramework()->GetClientActor());

		player->GetEntity()->GetAI()->Event(AIEVENT_DISABLE, NULL);
		player->SetHealth(0);
		player->CreateScriptEvent("kill", 0);

		m_respawningFromDemiGodDeath = true;
		m_elapsedTime = 0.0;
	}
}

bool CGodMode::IsGod() const
{
	EGodModeState currentState = GetCurrentState();
	return currentState == eGMS_GodMode || currentState == eGMS_TeamGodMode;
}

bool CGodMode::IsGodModeActive() const
{
	EGodModeState currentState = GetCurrentState();
	return currentState != eGMS_None && currentState != eGMS_LAST;
}

void CGodMode::RespawnIfDead(CActor* actor) const
{
	if (actor && actor->GetHealth() <= 0.0f)
	{
		IEntity* entity = actor->GetEntity();

		actor->StandUp();

		Vec3 pos = entity->GetPos();
		pos.z += 1.0f;
		entity->SetPos(pos);

		actor->Revive(CActor::kRFR_Spawn);
		actor->SetHealth(actor->GetMaxHealth());

		actor->HolsterItem(true);
		actor->HolsterItem(false);

		entity->GetAI()->Event(AIEVENT_ENABLE, NULL);
	}
}

void CGodMode::RespawnPlayerIfDead() const
{
	CActor* player = static_cast<CActor*>(g_pGame->GetIGameFramework()->GetClientActor());
	RespawnIfDead(player);
}

void CGodMode::Update(float frameTime)
{
	if (!gEnv->bMultiplayer && m_respawningFromDemiGodDeath && eGMS_DemiGodMode == GetCurrentState())
	{
		m_elapsedTime += frameTime;

		if (m_elapsedTime > m_timeToWaitBeforeRespawn)
		{
			RespawnPlayerIfDead();
			m_respawningFromDemiGodDeath = false;
		}
	}
}
