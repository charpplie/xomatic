/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2014.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Schematyc add variable dialog.
-------------------------------------------------------------------------
History:
- 01:01:2013: Created by Paul Slinger
*************************************************************************/

#include "StdAfx.h"

#include "Schematyc_AddVariableDlg.h"

#include "Resource.h"
#include "Schematyc_PluginUtils.h"

namespace Schematyc
{
	//////////////////////////////////////////////////////////////////////////
	CAddVariableDlg::CAddVariableDlg(CWnd* pParent, CPoint pos, IDoc& doc, const SGUID& scopeGUID)
		: CDialog(IDD_SCHEMATYC_ADD_VARIABLE, pParent)
		, m_pos(pos)
		, m_doc(doc)
		, m_scopeGUID(scopeGUID)
	{}

	//////////////////////////////////////////////////////////////////////////
	BOOL CAddVariableDlg::OnInitDialog()
	{
		SetWindowPos(NULL, m_pos.x, m_pos.y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
		CDialog::OnInitDialog();

		GetSchematycFramework().GetEnvRegistry().VisitTypeDescs(MAKE_MEMBER_DELEGATE(CAddVariableDlg::VisitTypeDesc, *this));
		m_doc.VisitEnumerations(MAKE_MEMBER_DELEGATE(CAddVariableDlg::VisitEnumeration, *this));
		m_doc.VisitStructures(MAKE_MEMBER_DELEGATE(CAddVariableDlg::VisitStructure, *this));
		for(TGUIDStringPairVector::const_iterator iType = m_types.begin(), iEndType = m_types.end(); iType != iEndType; ++ iType)
		{
			m_typesCtrl.AddString(iType->second.c_str());
		}
		m_typesCtrl.SetCurSel(0);

		return true;
	}

	//////////////////////////////////////////////////////////////////////////
	void CAddVariableDlg::DoDataExchange(CDataExchange* pDX)
	{
		DDX_Control(pDX, IDC_SCHEMATYC_VARIABLE_TYPES, m_typesCtrl);
		DDX_Control(pDX, IDC_SCHEMATYC_VARIABLE_NAME, m_nameCtrl);
		CDialog::DoDataExchange(pDX);
	}

	//////////////////////////////////////////////////////////////////////////
	void CAddVariableDlg::OnOK()
	{
		CString	name;
		m_nameCtrl.GetWindowText(name);
		if(ValidateName(name.GetString()) == true)
		{
			m_doc.AddVariable(m_scopeGUID, name.GetString(), GetSelection());
			CDialog::OnOK();
		}
	}

	//////////////////////////////////////////////////////////////////////////
	VisitStatus::EValue CAddVariableDlg::VisitTypeDesc(const ITypeDescConstPtr& pTypeDesc)
	{
		m_types.push_back(TGUIDStringPair(pTypeDesc->GetTypeGUID(), pTypeDesc->GetName()));
		return VisitStatus::CONTINUE;
	}

	//////////////////////////////////////////////////////////////////////////
	VisitStatus::EValue CAddVariableDlg::VisitEnumeration(const IDocEnumerationConstPtr& pEnumeration)
	{
		m_types.push_back(TGUIDStringPair(pEnumeration->GetGUID(), pEnumeration->GetName()));
		return VisitStatus::CONTINUE;
	}

	//////////////////////////////////////////////////////////////////////////
	VisitStatus::EValue CAddVariableDlg::VisitStructure(const IDocStructureConstPtr& pStructure)
	{
		m_types.push_back(TGUIDStringPair(pStructure->GetGUID(), pStructure->GetName()));
		return VisitStatus::CONTINUE;
	}

	//////////////////////////////////////////////////////////////////////////
	bool CAddVariableDlg::ValidateName(const stack_string& name)
	{
		stack_string	errorMessage;
		if(PluginUtils::IsValidName(name, errorMessage) == true)
		{
			return true;
		}
		else
		{
			MessageBox(errorMessage.c_str(), "Invalid Name", MB_OK | MB_ICONEXCLAMATION);
			return false;
		}
	}

	//////////////////////////////////////////////////////////////////////////
	SGUID CAddVariableDlg::GetSelection() const
	{
		const int	curSel = m_typesCtrl.GetCurSel();
		if((curSel != LB_ERR) && (curSel >= 0))
		{
			return m_types[curSel].first;
		}
		return SGUID();
	}
}