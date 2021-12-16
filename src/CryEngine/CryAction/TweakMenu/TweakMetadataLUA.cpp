/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2006.
-------------------------------------------------------------------------

Description: 
	Wraps a unit of Tweak metadata

-------------------------------------------------------------------------
History:
- 28:02:2006  : Created by Matthew Jack

Notes:

*************************************************************************/

#include "StdAfx.h"
#include "TweakMetadataLUA.h"
#include "LuaVarUtils.h"

//-------------------------------------------------------------------------

CTweakMetadataLUA::CTweakMetadataLUA(IScriptTable *pTable) : CTweakMetadata(pTable)
{
	// Already fetched by base classes: DELTA, NAME

	// Look for the essential elements of a Tweak
	m_bValid = true;

	// Fetch the variable name (we know this exists)
	m_sVariable = FetchStringValue(pTable, "LUA");

	// There's probably other checks that should be done, including looking for unrecognised elements
}

//-------------------------------------------------------------------------

string CTweakMetadataLUA::GetValue(void) {
	string result = "Not found";

	if (CallAnyEvaluator(result)) 
		return result;
	
	// What type is the variable?
	ScriptAnyValue value = GetLuaValue();
	ScriptAnyType type = value.type;
	switch (type) {
		case ANY_TSTRING:
			result = value.str; 
			break;
		case ANY_TNUMBER:
			// Should we treat this as a boolean?
			if ( TREATASBOOLEAN == m_fDelta)
				result = ( (int)value.number ? "True" : "False" );
			else 
				result.Format("%f", value.number);
			break;
		case ANY_TBOOLEAN:
			result = (value.b ? "True" : "False");
			break;
		case ANY_TFUNCTION:
			result = "A function";
			break;
		default:
			result = "Type not handled";
	}

	return result;
}

//-------------------------------------------------------------------------


ScriptAnyValue CTweakMetadataLUA::GetLuaValue(void) const {
	IScriptSystem *scripts = gEnv->pScriptSystem;
	//ScriptAnyValue failed("Value unrecognised");
	ScriptAnyValue result;

	// Fetch as a variable
	if (LuaVarUtils::GetRecursive(m_sVariable.c_str(), result)) {
		
		// Is this actually a function? If so call it
		if (result.type == ANY_TFUNCTION) {
				scripts->BeginCall(result.function);
				scripts->EndCallAny(result);
		} 
	}
	return result;
}



//-------------------------------------------------------------------------

bool CTweakMetadataLUA::ChangeValue(bool bIncrement) const {
	
	if (!CallAnyIncrementer(bIncrement))
	{
		IScriptSystem *scripts = gEnv->pScriptSystem;
		// Simple variable - get, (in|de)crement and set

		// Decide delta
		double fDelta = m_fDelta;
		if (!bIncrement) fDelta *= -1.0f;

		// Change variable based on type
		ScriptAnyValue value = GetLuaValue();
		ScriptAnyType type = value.type;
		switch (type) {
			case ANY_TNUMBER:
				// Should we treat this as a bool?
				if ( TREATASBOOLEAN == m_fDelta)
					value.number = ( (int)value.number ? 0.0f : 1.0f );
				else
					value.number = (float)ClampToLimits( value.number + fDelta );
				break;
			case ANY_TBOOLEAN:
				value.b ^= true;
				break;
			default:
				// Type not handled
				return false;
		}

		// Set the variable
		LuaVarUtils::SetRecursive(m_sVariable.c_str(), value);
	}

	return true;
}

//-------------------------------------------------------------------------

