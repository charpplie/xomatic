/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2014.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Schematyc create schema dialog.
-------------------------------------------------------------------------
History:
- 01:01:2013: Created by Paul Slinger
*************************************************************************/

#include "StdAfx.h"

#include "Schematyc_CreateSchemaDlg.h"

#include <Schematyc/Schematyc_IFoundation.h>

#include "Resource.h"
#include "Schematyc_PluginUtils.h"

namespace Schematyc
{
	//////////////////////////////////////////////////////////////////////////
	CCreateSchemaDlg::CCreateSchemaDlg(CWnd* pParent, CPoint pos, IDoc& doc, const SGUID& scopeGUID)
		: CDialog(IDD_SCHEMATYC_CREATE_SCHEMA, pParent)
		, m_pos(pos)
		, m_doc(doc)
		, m_scopeGUID(scopeGUID)
 	{}

	//////////////////////////////////////////////////////////////////////////
	BOOL CCreateSchemaDlg::OnInitDialog()
	{
		SetWindowPos(NULL, m_pos.x, m_pos.y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
		CDialog::OnInitDialog();

		GetSchematycFramework().GetEnvRegistry().VisitFoundations(MAKE_MEMBER_DELEGATE(CCreateSchemaDlg::VisitFoundation, *this));
		for(size_t iFoundation = 0, foundationCount = m_foundations.size(); iFoundation < foundationCount; ++ iFoundation)
		{
			m_foundationsCtrl.AddString(m_foundations[iFoundation].second.c_str());
		}
		m_foundationsCtrl.SetCurSel(0);

		return true;
	}

	//////////////////////////////////////////////////////////////////////////
	void CCreateSchemaDlg::DoDataExchange(CDataExchange* pDX)
	{
		DDX_Control(pDX, IDC_SCHEMATYC_SCHEMA_FOUNDATIONS, m_foundationsCtrl);
		DDX_Control(pDX, IDC_SCHEMATYC_SCHEMA_NAME, m_nameCtrl);
		CDialog::DoDataExchange(pDX);
	}

	//////////////////////////////////////////////////////////////////////////
	void CCreateSchemaDlg::OnOK()
	{
		CString	name;
		m_nameCtrl.GetWindowText(name);
		if(ValidateName(name.GetString()) == true)
		{
			m_doc.AddSchema(m_scopeGUID, name.GetString(), GetSelection());
			CDialog::OnOK();
		}
	}

	//////////////////////////////////////////////////////////////////////////
	inline VisitStatus::EValue CCreateSchemaDlg::VisitFoundation(const IFoundationConstPtr& pFoundation)
	{
		m_foundations.push_back(TGUIDStringPair(pFoundation->GetGUID(), pFoundation->GetName()));
		return VisitStatus::CONTINUE;
	}

	//////////////////////////////////////////////////////////////////////////
	bool CCreateSchemaDlg::ValidateName(const stack_string& name)
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
	SGUID CCreateSchemaDlg::GetSelection() const
	{
		const int	curSel = m_foundationsCtrl.GetCurSel();
		if((curSel != LB_ERR) && (curSel >= 0))
		{
			return m_foundations[curSel].first;
		}
		return SGUID();
	}
}