/*************************************************************************
	Crytek Source File.
	Copyright (C), Crytek Studios, 2009.
	-------------------------------------------------------------------------
	$Id$
	$DateTime$
	Description: 
		Objectives module that uses a lua table
	-------------------------------------------------------------------------
	History:
	- 25:11:2009  : Created by Colin Gulliver

*************************************************************************/

#include "StdAfx.h"
#include "GameRulesLuaObjectives.h"
#include "GameRules.h"

//-------------------------------------------------------------------------
CGameRulesLuaObjectives::CGameRulesLuaObjectives()
{
	m_funcUpdate = NULL;
	m_funcOnStartGame = NULL;
	m_funcOnGameReset = NULL;
	m_funcHasCompleted = NULL;
	m_funcOnHostMigration = NULL;
}

//-------------------------------------------------------------------------
CGameRulesLuaObjectives::~CGameRulesLuaObjectives()
{
}

//-------------------------------------------------------------------------
void CGameRulesLuaObjectives::Init( XmlNodeRef xml )
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
			m_objectivesScript->GetValue("OnGameReset", m_funcOnGameReset);
			m_objectivesScript->GetValue("HasCompleted", m_funcHasCompleted);
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
void CGameRulesLuaObjectives::Update( float frameTime )
{
	if (m_objectivesScript.GetPtr() && m_funcUpdate)
	{
		Script::Call(m_pScriptSystem, m_funcUpdate, m_objectivesScript, frameTime);
	}
}

//-------------------------------------------------------------------------
void CGameRulesLuaObjectives::OnStartGame()
{
	if (m_objectivesScript.GetPtr() && m_funcOnStartGame)
	{
		Script::Call(m_pScriptSystem, m_funcOnStartGame, m_objectivesScript);
	}
}

//-------------------------------------------------------------------------
bool CGameRulesLuaObjectives::NetSerialize( TSerialize ser, EEntityAspects aspect, uint8 profile, int flags )
{
	return true;
}

//-------------------------------------------------------------------------
void CGameRulesLuaObjectives::OnGameReset()
{
	if (m_objectivesScript.GetPtr() && m_funcOnGameReset)
	{
		Script::Call(m_pScriptSystem, m_funcOnGameReset, m_objectivesScript);
	}
}

//-------------------------------------------------------------------------
bool CGameRulesLuaObjectives::HasCompleted( int teamId )
{
	bool hasCompleted = false;
	if (m_objectivesScript.GetPtr() && m_funcHasCompleted)
	{
		Script::CallReturn(m_pScriptSystem, m_funcHasCompleted, m_objectivesScript, teamId, hasCompleted);
	}
	return hasCompleted;
}

//-------------------------------------------------------------------------
void CGameRulesLuaObjectives::OnHostMigration(bool becomeServer)
{
	if (m_objectivesScript.GetPtr() && m_funcOnHostMigration)
	{
		Script::Call(m_pScriptSystem, m_funcOnHostMigration, m_objectivesScript, becomeServer);
	}
}
