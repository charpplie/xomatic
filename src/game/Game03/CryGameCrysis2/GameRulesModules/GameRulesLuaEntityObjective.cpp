/*************************************************************************
	Crytek Source File.
	Copyright (C), Crytek Studios, 2009.
	-------------------------------------------------------------------------
	$Id$
	$DateTime$
	Description: 
		Implementation of an entity objective that uses a lua table

	-------------------------------------------------------------------------
	History:
	- 25:11:2009  : Created by Colin Gulliver

*************************************************************************/

#include "StdAfx.h"
#include "GameRulesLuaEntityObjective.h"
#include "GameRules.h"

//-------------------------------------------------------------------------
CGameRulesLuaEntityObjective::CGameRulesLuaEntityObjective()
{
	m_funcUpdate = NULL;
	m_funcOnStartGame = NULL;
	m_funcEnableCompletion = NULL;
	m_funcEnableScoring = NULL;
	m_funcIsComplete = NULL;
	m_funcAddEntityId = NULL;
	m_funcRemoveEntityId = NULL;
	m_funcClearEntities = NULL;
	m_funcIsEntityFinished = NULL;
	m_funcCanRemoveEntity = NULL;
	m_funcSetWaveNumber = NULL;
	m_funcOnHostMigration = NULL;
}

//-------------------------------------------------------------------------
CGameRulesLuaEntityObjective::~CGameRulesLuaEntityObjective()
{
}

//-------------------------------------------------------------------------
void CGameRulesLuaEntityObjective::Init( XmlNodeRef xml )
{
	const char *pTableName = 0;
	if (xml->getAttr("table", &pTableName))
	{
		CGameRules *pGameRules = g_pGame->GetGameRules();
		IScriptTable *pGameRulesScript = pGameRules->GetEntity()->GetScriptTable();
		CRY_ASSERT(pGameRulesScript);

		SmartScriptTable objectiveTable = NULL;
		pGameRulesScript->GetValue(pTableName, objectiveTable);

		if (objectiveTable.GetPtr())
		{
			InitLuaTable(objectiveTable);

			m_objectivesScript->GetValue("Update", m_funcUpdate);
			m_objectivesScript->GetValue("OnStartGame", m_funcOnStartGame);
			m_objectivesScript->GetValue("EnableCompletion", m_funcEnableCompletion);
			m_objectivesScript->GetValue("EnableScoring", m_funcEnableScoring);
			m_objectivesScript->GetValue("IsComplete", m_funcIsComplete);
			m_objectivesScript->GetValue("AddEntityId", m_funcAddEntityId);
			m_objectivesScript->GetValue("RemoveEntityId", m_funcRemoveEntityId);
			m_objectivesScript->GetValue("ClearEntities", m_funcClearEntities);
			m_objectivesScript->GetValue("IsEntityFinished", m_funcIsEntityFinished);
			m_objectivesScript->GetValue("CanRemoveEntity", m_funcCanRemoveEntity);
			m_objectivesScript->GetValue("SetWaveNumber", m_funcSetWaveNumber);
			m_objectivesScript->GetValue("OnHostMigration", m_funcOnHostMigration);
		}
		else
		{
			CryLog("CGameRulesLuaObjectives::Init, ERROR: Table not found in gamerules script");
		}
	}
	else
	{
		CRY_ASSERT_MESSAGE(false, "No table attribute specified");
		CryLog("CGameRulesLuaObjectives::Init, ERROR: No table attribute specified");
	}
}

//-------------------------------------------------------------------------
void CGameRulesLuaEntityObjective::Update( float frameTime )
{
	if (m_objectivesScript.GetPtr() && m_funcUpdate)
	{
		Script::Call(m_pScriptSystem, m_funcUpdate, m_objectivesScript, frameTime);
	}
}

//-------------------------------------------------------------------------
void CGameRulesLuaEntityObjective::OnStartGame()
{
	if (m_objectivesScript.GetPtr() && m_funcOnStartGame)
	{
		Script::Call(m_pScriptSystem, m_funcOnStartGame, m_objectivesScript);
	}
}

//-------------------------------------------------------------------------
bool CGameRulesLuaEntityObjective::NetSerialize( TSerialize ser, EEntityAspects aspect, uint8 profile, int flags )
{
	return true;
}

//-------------------------------------------------------------------------
void CGameRulesLuaEntityObjective::EnableCompletion( int teamId, bool enable )
{
	if (m_objectivesScript.GetPtr() && m_funcEnableCompletion)
	{
		Script::Call(m_pScriptSystem, m_funcEnableCompletion, m_objectivesScript, teamId, enable);
	}
}

//-------------------------------------------------------------------------
void CGameRulesLuaEntityObjective::EnableScoring( int teamId, bool enable )
{
	if (m_objectivesScript.GetPtr() && m_funcEnableScoring)
	{
		Script::Call(m_pScriptSystem, m_funcEnableScoring, m_objectivesScript, teamId, enable);
	}
}

//-------------------------------------------------------------------------
bool CGameRulesLuaEntityObjective::IsComplete(int teamId)
{
	bool isComplete = false;
	if (m_objectivesScript.GetPtr() && m_funcIsComplete)
	{
		Script::CallReturn(m_pScriptSystem, m_funcIsComplete, m_objectivesScript, teamId, isComplete);
	}
	return isComplete;
}

//-------------------------------------------------------------------------
void CGameRulesLuaEntityObjective::AddEntityId( int type, EntityId entityId, int index, bool isNewEntity )
{
	if (m_objectivesScript.GetPtr() && m_funcAddEntityId)
	{
		Script::Call(m_pScriptSystem, m_funcAddEntityId, m_objectivesScript, type, ScriptHandle(entityId), index, isNewEntity);
	}
}

//-------------------------------------------------------------------------
void CGameRulesLuaEntityObjective::RemoveEntityId( int type, EntityId entityId )
{
	if (m_objectivesScript.GetPtr() && m_funcRemoveEntityId)
	{
		Script::Call(m_pScriptSystem, m_funcRemoveEntityId, m_objectivesScript, type, ScriptHandle(entityId));
	}
}

//-------------------------------------------------------------------------
void CGameRulesLuaEntityObjective::ClearEntities( int type )
{
	if (m_objectivesScript.GetPtr() && m_funcClearEntities)
	{
		Script::Call(m_pScriptSystem, m_funcClearEntities, m_objectivesScript, type);
	}
}

//-------------------------------------------------------------------------
bool CGameRulesLuaEntityObjective::IsEntityFinished(int type, int index)
{
	bool isFinished = false;
	if (m_objectivesScript.GetPtr() && m_funcIsEntityFinished)
	{
		Script::CallReturn(m_pScriptSystem, m_funcIsEntityFinished, m_objectivesScript, type, index, isFinished);
	}
	return isFinished;
}

//-------------------------------------------------------------------------
bool CGameRulesLuaEntityObjective::CanRemoveEntity(int type, int index)
{
	bool canRemoveEntity = false;
	if (m_objectivesScript.GetPtr() && m_funcCanRemoveEntity)
	{
		Script::CallReturn(m_pScriptSystem, m_funcCanRemoveEntity, m_objectivesScript, type, index, canRemoveEntity);
	}
	return canRemoveEntity;
}


//-------------------------------------------------------------------------
void CGameRulesLuaEntityObjective::SetWaveNumber(int num, int waveCount)
{
	if (m_objectivesScript.GetPtr() && m_funcSetWaveNumber)
	{
		Script::Call(m_pScriptSystem, m_funcSetWaveNumber, m_objectivesScript, num, waveCount);
	}
}

//-------------------------------------------------------------------------
void CGameRulesLuaEntityObjective::OnHostMigration( bool becomeServer )
{
	if (m_objectivesScript.GetPtr() && m_funcOnHostMigration)
	{
		Script::Call(m_pScriptSystem, m_funcOnHostMigration, m_objectivesScript, becomeServer);
	}
}
