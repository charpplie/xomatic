////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2013.
// -------------------------------------------------------------------------
//  File name:   CUndoConfigSpec.h
//  Version:     v1.00
//  Created:     01/02/2013 by Matthias Gojny.
//  Description: Undo for Python function (PySetConfigSpec)
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __CUNDOCONFIGSPEC__H__
#define __CUNDOCONFIGSPEC__H__

class CUndoConficSpec : public IUndoObject
{
public:
	CUndoConficSpec(const char* pUndoDescription = "Set Config Spec");

protected:
	int GetSize();
	const char* GetDescription();
	void Undo(bool bUndo);
	void Redo();

private:
	int m_undo;
	int m_redo;
	const char* m_undoDescription;
};

#endif // __CUNDOCONFIGSPEC__H__