/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2006.
-------------------------------------------------------------------------

Description: 
	Wraps a unit of Tweak metadata

-------------------------------------------------------------------------
History:
- 28:02:2006  : Created by Matthew Jack

*************************************************************************/

#include "StdAfx.h"
#include "TweakMetadataCVAR.h"
#include "IScriptSystem.h"

//-------------------------------------------------------------------------

CTweakMetadataCVAR::CTweakMetadataCVAR(IScriptTable *pTable) : CTweakMetadata(pTable)
{
	// Already fetched by base classes: DELTA, NAME

	// Look for the essential elements of a Tweak
	m_bValid = true;

	// Fetch the variable name (we know this exists)
	m_sVariable = FetchStringValue(pTable, "CVAR");

	// Get description
	ICVar *cVar = GetCVar();
	if (cVar)
	{
		m_sDescription = cVar->GetHelp();
	}
		
	// There's probably other checks that should be done, including looking for unrecognised elements
	
	// What type is the CVAR?
	m_CVarType = 0;
	if (GetCVar()) m_CVarType  = GetCVar()->GetType();

	// Use Rcon command?
	pTable->GetValue("RCON", m_bUseRconCommand);
}

//-------------------------------------------------------------------------

string CTweakMetadataCVAR::GetValue(void) {
	string result = "Not found";

	if (CallAnyEvaluator(result)) 
		return result;

	ICVar *cVar = GetCVar();
	if (cVar) 
	{
		// Should we treat this as a bool?
		if ( TREATASBOOLEAN == m_fDelta && CVAR_INT == cVar->GetType() )
			result = ( cVar->GetIVal() ? "True" : "False" );
		else
			result = cVar->GetString();
	}
	return result;
}
		
//-------------------------------------------------------------------------

bool CTweakMetadataCVAR::ChangeValue(bool bIncrement) {

	if (!CallAnyIncrementer(bIncrement))
	{
		// Get delta
		double fDelta = m_fDelta;
		if (!bIncrement) fDelta *= -1.0f;

		// Get and check CVAR
		ICVar *cVar = GetCVar();
		if (!cVar) return false;

		string sValue;
		
		// Deal with appropriate type
		switch (cVar->GetType()) {
			case CVAR_INT:
					// Should we treat this as a bool?
					if (TREATASBOOLEAN == m_fDelta)
						sValue.Format("%d", cVar->GetIVal() ? 0 : 1);
					else
						sValue.Format("%d", (int)ClampToLimits(cVar->GetIVal() + fDelta));
				break;
			case CVAR_FLOAT:
				sValue.Format("%f", (float)ClampToLimits(cVar->GetFVal() + fDelta));
				break;
			default:;
				// Strings are non-obvious
					// Might also be a non-existent variable			
		}

		// Send RCon command?
		IRemoteControlClient *pRConClient = gEnv->pNetwork->GetRemoteControlSystemSingleton()->GetClientSingleton();
		CRY_ASSERT(pRConClient);
		if (m_bUseRconCommand && !gEnv->bServer && pRConClient)
		{
			string sCommand;
			sCommand.Format("%s %s", m_sVariable.c_str(), sValue.c_str());
			pRConClient->SendCommand(sCommand.c_str());
		}
		else
		{
			// Set value
			cVar->Set(sValue.c_str());
		}
	}

	return true;
}

//-------------------------------------------------------------------------

// Wraps fetching the console variable
ICVar * CTweakMetadataCVAR::GetCVar(void) const 
{
	return gEnv->pConsole->GetCVar(m_sVariable.c_str());
}

//-------------------------------------------------------------------------

