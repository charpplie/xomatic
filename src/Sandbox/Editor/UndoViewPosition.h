////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2013.
// -------------------------------------------------------------------------
//  File name:   CUndoViewPosition.h
//  Version:     v1.00
//  Created:     01/02/2013 by Matthias Gojny.
//  Description: Undo for Python function (PySetCurrentViewPosition)
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __CUNDOVIEWPOSITION__H__
#define __CUNDOVIEWPOSITION__H__

class CUndoViewPosition : public IUndoObject
{
public:
	CUndoViewPosition(const char* pUndoDescription = "Set Current View Position");

protected:
	int GetSize();
	const char* GetDescription();
	void Undo(bool bUndo);
	void Redo();

private:
	Vec3 m_undo;
	Vec3 m_redo;
	const char* m_undoDescription;
};

#endif // __CUNDOVIEWPOSITION__H__