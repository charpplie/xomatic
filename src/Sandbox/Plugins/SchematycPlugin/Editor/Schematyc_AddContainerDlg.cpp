/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2015.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Schematyc add container dialog.
-------------------------------------------------------------------------
History:
- 06:01:2015: Created by Paul Slinger
*************************************************************************/

#include "StdAfx.h"

#include "Schematyc_AddContainerDlg.h"

#include "Resource.h"
#include "Schematyc_PluginUtils.h"

namespace Schematyc
{
	//////////////////////////////////////////////////////////////////////////
	CAddContainerDlg::CAddContainerDlg(CWnd* pParent, CPoint pos, IDoc& doc, const SGUID& scopeGUID)
		: CDialog(IDD_SCHEMATYC_ADD_CONTAINER, pParent)
		, m_pos(pos)
		, m_doc(doc)
		, m_scopeGUID(scopeGUID)
	{}

	//////////////////////////////////////////////////////////////////////////
	BOOL CAddContainerDlg::OnInitDialog()
	{
		SetWindowPos(NULL, m_pos.x, m_pos.y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
		CDialog::OnInitDialog();

		GetSchematycFramework().GetEnvRegistry().VisitTypeDescs(MAKE_MEMBER_DELEGATE(CAddContainerDlg::VisitTypeDesc, *this));
		m_doc.VisitEnumerations(MAKE_MEMBER_DELEGATE(CAddContainerDlg::VisitEnumeration, *this));
		m_doc.VisitStructures(MAKE_MEMBER_DELEGATE(CAddContainerDlg::VisitStructure, *this));
		for(TGUIDStringPairVector::const_iterator iType = m_types.begin(), iEndType = m_types.end(); iType != iEndType; ++ iType)
		{
			m_typesCtrl.AddString(iType->second.c_str());
		}
		m_typesCtrl.SetCurSel(0);

		return true;
	}

	//////////////////////////////////////////////////////////////////////////
	void CAddContainerDlg::DoDataExchange(CDataExchange* pDX)
	{
		DDX_Control(pDX, IDC_SCHEMATYC_CONTAINER_TYPES, m_typesCtrl);
		DDX_Control(pDX, IDC_SCHEMATYC_CONTAINER_NAME, m_nameCtrl);
		CDialog::DoDataExchange(pDX);
	}

	//////////////////////////////////////////////////////////////////////////
	void CAddContainerDlg::OnOK()
	{
		CString	name;
		m_nameCtrl.GetWindowText(name);
		if(ValidateName(name.GetString()) == true)
		{
			m_doc.AddContainer(m_scopeGUID, name.GetString(), GetSelection());
			CDialog::OnOK();
		}
	}

	//////////////////////////////////////////////////////////////////////////
	VisitStatus::EValue CAddContainerDlg::VisitTypeDesc(const ITypeDescConstPtr& pTypeDesc)
	{
		m_types.push_back(TGUIDStringPair(pTypeDesc->GetTypeGUID(), pTypeDesc->GetName()));
		return VisitStatus::CONTINUE;
	}

	//////////////////////////////////////////////////////////////////////////
	VisitStatus::EValue CAddContainerDlg::VisitEnumeration(const IDocEnumerationConstPtr& pEnumeration)
	{
		m_types.push_back(TGUIDStringPair(pEnumeration->GetGUID(), pEnumeration->GetName()));
		return VisitStatus::CONTINUE;
	}

	//////////////////////////////////////////////////////////////////////////
	VisitStatus::EValue CAddContainerDlg::VisitStructure(const IDocStructureConstPtr& pStructure)
	{
		m_types.push_back(TGUIDStringPair(pStructure->GetGUID(), pStructure->GetName()));
		return VisitStatus::CONTINUE;
	}

	//////////////////////////////////////////////////////////////////////////
	bool CAddContainerDlg::ValidateName(const stack_string& name)
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
	SGUID CAddContainerDlg::GetSelection() const
	{
		const int	curSel = m_typesCtrl.GetCurSel();
		if((curSel != LB_ERR) && (curSel >= 0))
		{
			return m_types[curSel].first;
		}
		return SGUID();
	}
}