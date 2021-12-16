////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2013.
// -------------------------------------------------------------------------
//  File name:   CUndoConfigSpec.cpp
//  Version:     v1.00
//  Created:     01/02/2013 by Matthias Gojny.
//  Description: Undo for Python function (PySetConfigSpec)
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "UndoConfigSpec.h"

CUndoConficSpec::CUndoConficSpec(const char* pUndoDescription)
{
	m_undo = GetIEditor()->GetEditorConfigSpec();
	m_undoDescription = pUndoDescription;
}

int CUndoConficSpec::GetSize()
{
	return sizeof(*this);
}	

const char* CUndoConficSpec::GetDescription()
{ 
	return m_undoDescription;
}

void CUndoConficSpec::Undo(bool bUndo)
{
	if(bUndo)
	{
		m_redo = GetIEditor()->GetEditorConfigSpec();
	}
	GetIEditor()->SetEditorConfigSpec((ESystemConfigSpec)m_undo);
}

void CUndoConficSpec::Redo()
{
	GetIEditor()->SetEditorConfigSpec((ESystemConfigSpec)m_redo);
}