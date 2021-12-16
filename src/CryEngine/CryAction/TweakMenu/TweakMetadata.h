/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2006.
-------------------------------------------------------------------------

Description: 
	Header for CTweakMetadata

-------------------------------------------------------------------------
History:
- 28:02:2006  : Created by Matthew Jack
- 23:06:2006	:	Incrementer/decrementer functionality moved here

Notes:
	State table awaits implementation, for more powerful LUA incrementers.

*************************************************************************/


#ifndef __CTWEAKMETADATA_H__
#define __CTWEAKMETADATA_H__

#pragma once

//-------------------------------------------------------------------------

#include "TweakCommon.h"
#include "IScriptSystem.h"

//-------------------------------------------------------------------------
// Forward declarations

struct IScriptTable;

//-------------------------------------------------------------------------

class CTweakMetadata : public CTweakCommon {
public:
	
	CTweakMetadata(IScriptTable *pTable);
	
	~CTweakMetadata() {};

	virtual string GetValue(void) = 0;

	virtual bool DecreaseValue(void) = 0;

	virtual bool IncreaseValue(void) = 0;

	ETweakType GetType(void) { return eTT_Metadata; }

	static CTweakCommon * GetNewMetadata(IScriptTable *pTable);

protected:
	
	// Call any incrementer functions present, for custom actions
	bool CallAnyIncrementer(bool bIncrement) const;

	// Call any evaluator function present, for custom display
	bool CallAnyEvaluator(string &result) const;

	// Wraps fetching a Lua function value
	HSCRIPTFUNCTION FetchFunctionValue(IScriptTable *pTable, const char *sKey) const;

	// Clamp a value to the set limits
	double ClampToLimits( double x ) const;

	// Was the metadata constructed successfully?
	bool m_bValid;

	// The variable this Tweak would tweak
	string m_sVariable; // The name of the CVAR/LUA variable

	// The delta hint amount
	double m_fDelta;

	// The minimum and maximum numeric values
	float m_fMin, m_fMax;

	// Special delta value denoting that the value is really a boolean
	static const double TREATASBOOLEAN;

	// The incrementer function, if it exists
	HSCRIPTFUNCTION m_incrementer;

	// The decrementer function, if it exists
	HSCRIPTFUNCTION m_decrementer;

	// The evaluator function, if it exists
	HSCRIPTFUNCTION m_evaluator;
};

#endif // __CTWEAKMETADATA_H__