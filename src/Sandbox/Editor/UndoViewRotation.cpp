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
#include "UndoViewRotation.h"
#include "ViewManager.h"

CUndoViewRotation::CUndoViewRotation(const char* pUndoDescription)
{
	m_undoDescription = pUndoDescription;
	m_undo = RAD2DEG(Ang3::GetAnglesXYZ(Matrix33(GetIEditor()->GetSystem()->GetViewCamera().GetMatrix())));
}

int CUndoViewRotation::GetSize()
{
	return sizeof(*this);
}	

const char* CUndoViewRotation::GetDescription()
{ 
	return m_undoDescription; 
}

void CUndoViewRotation::Undo(bool bUndo)
{
	CViewport *pRenderViewport = GetIEditor()->GetViewManager()->GetGameViewport();
	if(pRenderViewport)
	{
		if(bUndo)
		{
			m_redo = RAD2DEG(Ang3::GetAnglesXYZ(Matrix33(GetIEditor()->GetSystem()->GetViewCamera().GetMatrix())));
		}

		Matrix34 tm = pRenderViewport->GetViewTM();
		tm.SetRotationXYZ(Ang3(DEG2RAD(m_undo.x), DEG2RAD(m_undo.y), DEG2RAD(m_undo.z)), tm.GetTranslation());
		pRenderViewport->SetViewTM(tm);
	}
}

void CUndoViewRotation::Redo()
{
	CViewport *pRenderViewport = GetIEditor()->GetViewManager()->GetGameViewport();
	if(pRenderViewport)
	{
		Matrix34 tm = pRenderViewport->GetViewTM();
		tm.SetRotationXYZ(Ang3(DEG2RAD(m_redo.x), DEG2RAD(m_redo.y), DEG2RAD(m_redo.z)), tm.GetTranslation());
		pRenderViewport->SetViewTM(tm);
	}
}