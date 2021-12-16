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


#ifndef __CTWEAKMETADATACOMMAND_H__
#define __CTWEAKMETADATACOMMAND_H__

#pragma once

//-------------------------------------------------------------------------

#include "TweakMetadata.h"

//-------------------------------------------------------------------------
// Forward declarations

struct IScriptTable;

//-------------------------------------------------------------------------

class CTweakMetadataCommand : public CTweakMetadata {
public:
	
	CTweakMetadataCommand(IScriptTable *pTable);

	string GetValue(void) { return string(); }

	bool DecreaseValue(void) { return ExecuteCommand(); }

	bool IncreaseValue(void) { return ExecuteCommand(); }

protected:

	bool ExecuteCommand();

	// Should CVAR be propagated over using rcon
	bool m_bUseRconCommand;
};

#endif // __CTWEAKMETADATACOMMAND_H__