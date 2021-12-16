////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2013.
// -------------------------------------------------------------------------
//  File name:   CUndoViewPosition.cpp
//  Version:     v1.00
//  Created:     01/02/2013 by Matthias Gojny.
//  Description: Undo for Python function (PySetCurrentViewPosition)
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "UndoViewPosition.h"
#include "ViewManager.h"

CUndoViewPosition::CUndoViewPosition(const char* pUndoDescription)
{
	m_undoDescription = pUndoDescription;

	CViewport *pRenderViewport = GetIEditor()->GetViewManager()->GetGameViewport();
	if(pRenderViewport)
	{
		Matrix34 tm = pRenderViewport->GetViewTM();
		m_undo = tm.GetTranslation();
	}
}

int CUndoViewPosition::GetSize()
{
	return sizeof(*this);
}	

const char* CUndoViewPosition::GetDescription()
{ 
	return m_undoDescription; 
}

void CUndoViewPosition::Undo(bool bUndo)
{
	CViewport *pRenderViewport = GetIEditor()->GetViewManager()->GetGameViewport();
	if(pRenderViewport)
	{
		Matrix34 tm = pRenderViewport->GetViewTM();
		if(bUndo)
		{
			m_redo = tm.GetTranslation();
		}
	
		tm.SetTranslation(m_undo);
		pRenderViewport->SetViewTM(tm);
	}
}

void CUndoViewPosition::Redo()
{
	CViewport *pRenderViewport = GetIEditor()->GetViewManager()->GetGameViewport();
	if(pRenderViewport)
	{
		Matrix34 tm = pRenderViewport->GetViewTM();
		tm.SetTranslation(m_redo);
		pRenderViewport->SetViewTM(tm);
	}
}