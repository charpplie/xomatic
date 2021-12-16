/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2009.
-------------------------------------------------------------------------

Description: 
Header for CTweakMetadataCommand

-------------------------------------------------------------------------
History:
- 07:10:2009  : Created by Alex McCarthy

*************************************************************************/

#include "StdAfx.h"
#include "TweakMetadataCommand.h"

CTweakMetadataCommand::CTweakMetadataCommand(IScriptTable *pTable) : CTweakMetadata(pTable)
{
	// Already fetched by base classes: DELTA, NAME

	// Look for the essential elements of a Tweak
	m_bValid = true;

	// Fetch the variable name (we know this exists)
	m_sVariable = FetchStringValue(pTable, "COMMAND");

	// Use Rcon command?
	pTable->GetValue("RCON", m_bUseRconCommand);
}

bool CTweakMetadataCommand::ExecuteCommand()
{
	const char* szCommand = m_sVariable.c_str();
	// Send RCon command?
	IRemoteControlClient *pRConClient = gEnv->pNetwork->GetRemoteControlSystemSingleton()->GetClientSingleton();
	CRY_ASSERT(pRConClient);
	if (m_bUseRconCommand && !gEnv->bServer && pRConClient)
	{
		pRConClient->SendCommand(szCommand);
	}
	else
	{
		gEnv->pConsole->ExecuteString(szCommand);
	}

	return true;
}

