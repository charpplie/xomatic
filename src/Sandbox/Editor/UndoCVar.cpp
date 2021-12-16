////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2013.
// -------------------------------------------------------------------------
//  File name:   CUndoCVar.cpp
//  Version:     v1.00
//  Created:     28/01/2013 by Matthias Gojny.
//  Description: Undo for Python function (PySetCVar)
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "UndoCVar.h"

CUndoCVar::CUndoCVar(const char* pCVarName, const char* pUndoDescription)
{
	m_CVarName = pCVarName;
	m_undoDescription = pUndoDescription;

	ICVar *pCVar = GetIEditor()->GetSystem()->GetIConsole()->GetCVar(m_CVarName);
	if(pCVar)
	{
		if(pCVar->GetType() == CVAR_INT)
		{
			m_undo.type = SPyWrappedProperty::eType_Int;
			m_undo.property.intValue = pCVar->GetIVal();
		}
		else if(pCVar->GetType() == CVAR_FLOAT)
		{
			m_undo.type = SPyWrappedProperty::eType_Float;
			m_undo.property.floatValue = pCVar->GetFVal();
		}
		else if(pCVar->GetType() == CVAR_STRING)
		{
			m_undo.type = SPyWrappedProperty::eType_String;
			m_undo.stringValue = pCVar->GetString();
		}
	}
}	

int CUndoCVar::GetSize()
{
	return sizeof(*this);
}	

const char* CUndoCVar::GetDescription()
{ 
	return m_undoDescription; 
}

void CUndoCVar::Undo(bool bUndo)
{
	ICVar* pTempCVar = GetIEditor()->GetSystem()->GetIConsole()->GetCVar(m_CVarName);
	if(pTempCVar)
	{
		if(bUndo)
		{
			if(pTempCVar->GetType() == CVAR_INT && m_undo.type == SPyWrappedProperty::eType_Int)
			{
				m_redo.type = SPyWrappedProperty::eType_Int;
				m_redo.property.intValue = pTempCVar->GetIVal();
			}
			else if(pTempCVar->GetType() == CVAR_FLOAT && m_undo.type == SPyWrappedProperty::eType_Float)
			{
				m_redo.type = SPyWrappedProperty::eType_Float;
				m_redo.property.intValue = pTempCVar->GetFVal();
			}
			else if(pTempCVar->GetType() == CVAR_STRING && m_undo.type == SPyWrappedProperty::eType_String)
			{
				m_redo.type = SPyWrappedProperty::eType_String;
				m_redo.stringValue = pTempCVar->GetString();
			}
		}
	
		if(pTempCVar->GetType() == CVAR_INT && m_undo.type == SPyWrappedProperty::eType_Int)
		{
			pTempCVar->Set(m_undo.property.intValue);
		}
		else if(pTempCVar->GetType() == CVAR_FLOAT && m_undo.type == SPyWrappedProperty::eType_Float)
		{
			pTempCVar->Set(m_undo.property.floatValue);
		}
		else if(pTempCVar->GetType() == CVAR_STRING && m_undo.type == SPyWrappedProperty::eType_String)
		{
			pTempCVar->Set(m_undo.stringValue);
		}
	}
}	

void CUndoCVar::Redo()
{
	ICVar* pTempCVar = GetIEditor()->GetSystem()->GetIConsole()->GetCVar(m_CVarName);
	if(pTempCVar)
	{
		if(pTempCVar->GetType() == CVAR_INT && m_redo.type == SPyWrappedProperty::eType_Int)
		{
			pTempCVar->Set(m_redo.property.intValue);
		}
		else if(pTempCVar->GetType() == CVAR_FLOAT && m_undo.type == SPyWrappedProperty::eType_Float)
		{
			pTempCVar->Set(m_redo.property.floatValue);
		}
		else if(pTempCVar->GetType() && m_undo.type == SPyWrappedProperty::eType_String)
		{
			pTempCVar->Set(m_redo.stringValue);
		}
	}
}