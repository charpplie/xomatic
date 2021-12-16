/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2014.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Schematyc add state machine dialog.
-------------------------------------------------------------------------
History:
- 01:01:2013: Created by Paul Slinger
*************************************************************************/

#include "StdAfx.h"

#include "Schematyc_AddStateMachineDlg.h"

#include "Resource.h"
#include "Schematyc_PluginUtils.h"

SCHEMATYC_DECLARE_DOC_STATE_MACHINE_LIFETIME_ENUM

namespace Schematyc
{
	//////////////////////////////////////////////////////////////////////////
	CAddStateMachineDlg::CAddStateMachineDlg(CWnd* pParent, CPoint pos, IDoc& doc, const SGUID& scopeGUID)
		: CDialog(IDD_SCHEMATYC_ADD_STATE_MACHINE, pParent)
		, m_pos(pos)
		, m_doc(doc)
		, m_scopeGUID(scopeGUID)
	{}

	//////////////////////////////////////////////////////////////////////////
	BOOL CAddStateMachineDlg::OnInitDialog()
	{
		SetWindowPos(NULL, m_pos.x, m_pos.y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
		CDialog::OnInitDialog();

		const Serialization::CEnumDescription&	enumDescription = Serialization::getEnumDescription<DocStateMachineLifetime::EValue>();
		for(int i = 0, count = enumDescription.count(); i < count; ++ i)
		{
			m_lifetimeControl.AddString(enumDescription.labelByIndex(i));
		}
		m_lifetimeControl.SetCurSel(0);

		return true;
	}

	//////////////////////////////////////////////////////////////////////////
	void CAddStateMachineDlg::DoDataExchange(CDataExchange* pDX)
	{
		DDX_Control(pDX, IDC_SCHEMATYC_STATE_MACHINE_LIFETIME, m_lifetimeControl);
		DDX_Control(pDX, IDC_SCHEMATYC_STATE_MACHINE_NAME, m_nameCtrl);
		CDialog::DoDataExchange(pDX);
	}

	//////////////////////////////////////////////////////////////////////////
	void CAddStateMachineDlg::OnOK()
	{
		CString	name;
		m_nameCtrl.GetWindowText(name);
		if(ValidateName(name.GetString()) == true)
		{
			const DocStateMachineLifetime::EValue	lifetime = static_cast<DocStateMachineLifetime::EValue>(m_lifetimeControl.GetCurSel());
			m_doc.AddStateMachine(m_scopeGUID, name.GetString(), lifetime);
			CDialog::OnOK();
		}
	}

	//////////////////////////////////////////////////////////////////////////
	bool CAddStateMachineDlg::ValidateName(const stack_string& name)
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