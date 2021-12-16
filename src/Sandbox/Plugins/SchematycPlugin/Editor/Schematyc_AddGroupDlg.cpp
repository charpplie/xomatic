/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2014.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Schematyc add group dialog.
-------------------------------------------------------------------------
History:
- 01:01:2013: Created by Paul Slinger
*************************************************************************/

#include "StdAfx.h"

#include "Schematyc_AddGroupDlg.h"

#include "Resource.h"
#include "Schematyc_PluginUtils.h"

namespace Schematyc
{
	//////////////////////////////////////////////////////////////////////////
	CAddGroupDlg::CAddGroupDlg(CWnd* pParent, CPoint pos, IDoc& doc, const SGUID& scopeGUID)
		: CDialog(IDD_SCHEMATYC_ADD_GROUP, pParent)
		, m_pos(pos)
		, m_doc(doc)
		, m_scopeGUID(scopeGUID)
	{}

	//////////////////////////////////////////////////////////////////////////
	BOOL CAddGroupDlg::OnInitDialog()
	{
		SetWindowPos(NULL, m_pos.x, m_pos.y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
		CDialog::OnInitDialog();
		return true;
	}

	//////////////////////////////////////////////////////////////////////////
	void CAddGroupDlg::DoDataExchange(CDataExchange* pDX)
	{
		DDX_Control(pDX, IDC_SCHEMATYC_GROUP_NAME, m_nameCtrl);
		CDialog::DoDataExchange(pDX);
	}

	//////////////////////////////////////////////////////////////////////////
	void CAddGroupDlg::OnOK()
	{
		CString	name;
		m_nameCtrl.GetWindowText(name);
		if(ValidateName(name.GetString()) == true)
		{
			m_doc.AddGroup(m_scopeGUID, name.GetString());
			CDialog::OnOK();
		}
	}

	//////////////////////////////////////////////////////////////////////////
	bool CAddGroupDlg::ValidateName(const stack_string& name)
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
}