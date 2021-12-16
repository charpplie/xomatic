/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2006.
-------------------------------------------------------------------------

Description: 
	Abstract class unifying metadata classes

-------------------------------------------------------------------------
History:
- 28:02:2006  : Created by Matthew Jack

*************************************************************************/

#include "StdAfx.h"
#include "TweakMetadata.h"
#include "TweakMetadataCommand.h"
#include "TweakMetadataCVAR.h"
#include "TweakMetadataLUA.h"
#include "IScriptSystem.h"

//-------------------------------------------------------------------------
const double CTweakMetadata::TREATASBOOLEAN = 0.0f;
//-------------------------------------------------------------------------

CTweakMetadata::CTweakMetadata(IScriptTable *pTable) {
	// Default values
	m_fDelta = 1.0f;
	m_fMin = -FLT_MAX; 
	m_fMax = FLT_MAX;

	// Get the common elements of Tweaks
	m_sName = FetchStringValue(pTable, "NAME");

	// Fetch any delta hint
	string sIncHint = FetchStringValue(pTable, "DELTA");
	if (!sIncHint.empty()) {
		double fReadDelta = atof(sIncHint.c_str());
		// Check the read value
		if (fReadDelta == 0.0) {
			// Didn't parse as a float, might be special case
			if ( stricmp( sIncHint.c_str(), "bool") == 0)
				m_fDelta = TREATASBOOLEAN;
			else
				m_sError = "Invalid delta value";
		} else {
			m_fDelta = fReadDelta;
		}
	}	

	// Fetch description
	m_sDescription = FetchStringValue(pTable, "DESCRIPTION");
	if (m_sDescription.empty())
		m_sDescription = "N/A";

	// Fetch any min value
	pTable->GetValue("MINIMUM",m_fMin);

	// Fetch any max value
	pTable->GetValue("MAXIMUM",m_fMax);

	// Look for an evaluation function	
	m_evaluator = FetchFunctionValue(pTable, "EVALUATOR");

	// Look for an incrementer function
	m_incrementer = FetchFunctionValue(pTable, "INCREMENTER");

	// Look for an decrementer function
	m_decrementer = FetchFunctionValue(pTable, "DECREMENTER");

	// It would be a bit daft to use a custom function for, say, incrementing
	// but change the value in the standard way when decrementing, so for 
	// sanity's sake, if only one function is provided then use it for both.
	if (m_incrementer && !m_decrementer) 
		m_decrementer = m_incrementer;
	if (m_decrementer && !m_incrementer) 
		m_incrementer = m_decrementer;

	if (!pTable->GetValue("HIDDEN", m_bHidden))
		m_bHidden = false;
}

//-------------------------------------------------------------------------

// Identify the type metadata a table represents, return an instance
CTweakCommon *CTweakMetadata::GetNewMetadata( IScriptTable *pTable )  {

	int foundTypes = 0;

	const bool bLua = pTable->HaveValue("LUA");
	if(bLua)
		++foundTypes;

	const bool bCVar = pTable->HaveValue("CVAR");
	if(bCVar)
		++foundTypes;

	const bool bCommand = pTable->HaveValue("COMMAND");
	if(bCommand)
		++foundTypes;

	if (foundTypes != 1) {
		// One, and only one, should be present
		return new CTweakCommon("*Broken metadata*");
	}

	if (bLua) return new CTweakMetadataLUA(pTable);
	else if(bCVar) return new CTweakMetadataCVAR(pTable);
	else return new CTweakMetadataCommand(pTable);
}

//-------------------------------------------------------------------------

// If there is an incrementer function, use it rather than changing values using delta
bool CTweakMetadata::CallAnyIncrementer(bool bIncrement) const  {
	IScriptSystem *scripts = gEnv->pScriptSystem;
	
	// Get delta and negate if need be
	float delta = (m_fDelta == TREATASBOOLEAN ? 1.0f : (float)m_fDelta);
	if (!bIncrement) delta *= -1.0f;

	if (!bIncrement && m_decrementer) {
		// A decrement function has been provided and we are decremented
		scripts->BeginCall(m_decrementer);
		scripts->PushFuncParam(delta); // Pass in the delta for reference
		//m_pSystem->GetIScriptSystem()->PushFuncParam(maxAlertness); // Give it state
		scripts->EndCall();
		return true;

	} else if (bIncrement && m_incrementer) {
		// An increment function has been provided 
		scripts->BeginCall(m_incrementer);
		scripts->PushFuncParam(delta); // Pass in the delta for reference
		//m_pSystem->GetIScriptSystem()->PushFuncParam(maxAlertness); // Give it state
		scripts->EndCall();
		return true;
	}

	return false;
}

//-------------------------------------------------------------------------

// Use evaluator function in preference to looking up variable
bool CTweakMetadata::CallAnyEvaluator(string &result) const  {
	IScriptSystem *scripts = gEnv->pScriptSystem;
	char * returned;
	if (m_evaluator)
	{
		scripts->BeginCall(m_evaluator);
		if (scripts->EndCall(returned))
		{
			result = returned;
			return true;
		}
	}
	return false;
}

//-------------------------------------------------------------------------

HSCRIPTFUNCTION CTweakMetadata::FetchFunctionValue(IScriptTable *pTable, const char *sKey) const {

	// Can't this be done using the templating?

	ScriptAnyValue fun;
	if (pTable->GetValueAny(sKey, fun)) {

		if (fun.type == ANY_TFUNCTION) {
			return fun.function;
		}
	} 

	return NULL;
}

//-------------------------------------------------------------------------

double CTweakMetadata::ClampToLimits( double x ) const {
	// This should stay as a conditional clamp because m_fMin and m_fMax are
	// often set to -FLT_MAX and FLT_MAX, and this seems to be problematic with
	// a mathematical clamp
	return CLAMP(x, (double)m_fMin, (double)m_fMax);
}
