////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2013.
// -------------------------------------------------------------------------
//  File name:   CUndoViewRotation.h
//  Version:     v1.00
//  Created:     01/02/2013 by Matthias Gojny.
//  Description: Undo for Python function (PySetCurrentViewRotation)
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __CUNDOVIEWROTATION__H__
#define __CUNDOVIEWROTATION__H__

class CUndoViewRotation : public IUndoObject
{
public:
	CUndoViewRotation(const char* pUndoDescription = "Set Current View Rotation");

protected:
	int GetSize();
	const char* GetDescription();
	void Undo(bool bUndo);
	void Redo();

private:
	Ang3 m_undo;
	Ang3 m_redo;
	const char* m_undoDescription;
};

#endif // __CUNDOVIEWROTATION__H__