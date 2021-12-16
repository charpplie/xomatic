////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2013.
// -------------------------------------------------------------------------
//  File name:   CUndoCVar.h
//  Version:     v1.00
//  Created:     28/01/2013 by Matthias Gojny.
//  Description: Undo for Python function (PySetCVar)
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __CUNDOCVAR__H__
#define __CUNDOCVAR__H__

#include "Util/BoostPythonHelpers.h"

class CUndoCVar : public IUndoObject
{
public:
	CUndoCVar(const char* pCVarName, const char* pUndoDescription = "Set CVar");

protected:
	int GetSize();
	const char* GetDescription();
	void Undo(bool bUndo);
	void Redo();

private:
	SPyWrappedProperty m_undo;
	SPyWrappedProperty m_redo;
	const char* m_CVarName;
	const char* m_undoDescription;
};

#endif // __CUNDOCVAR__H__