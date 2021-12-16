/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2014.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Schematyc add state dialog.
-------------------------------------------------------------------------
History:
- 01:01:2013: Created by Paul Slinger
*************************************************************************/

#include "StdAfx.h"

#include "Schematyc_AddStateDlg.h"

#include "Resource.h"
#include "Schematyc_PluginUtils.h"

namespace Schematyc
{
	//////////////////////////////////////////////////////////////////////////
	CAddStateDlg::CAddStateDlg(CWnd* pParent, CPoint pos, IDoc& doc, const SGUID& scopeGUID)
		: CDialog(IDD_SCHEMATYC_ADD_STATE, pParent)
		, m_pos(pos)
		, m_doc(doc)
		, m_scopeGUID(scopeGUID)
	{}

	//////////////////////////////////////////////////////////////////////////
	BOOL CAddStateDlg::OnInitDialog()
	{
		SetWindowPos(NULL, m_pos.x, m_pos.y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
		CDialog::OnInitDialog();
		return true;
	}

	//////////////////////////////////////////////////////////////////////////
	void CAddStateDlg::DoDataExchange(CDataExchange* pDX)
	{
		DDX_Control(pDX, IDC_SCHEMATYC_STATE_NAME, m_nameCtrl);
		CDialog::DoDataExchange(pDX);
	}

	//////////////////////////////////////////////////////////////////////////
	void CAddStateDlg::OnOK()
	{
		CString	name;
		m_nameCtrl.GetWindowText(name);
		if(ValidateName(name.GetString()) == true)
		{
			m_doc.AddState(m_scopeGUID, name.GetString());
			CDialog::OnOK();
		}
	}

	//////////////////////////////////////////////////////////////////////////
	bool CAddStateDlg::ValidateName(const stack_string& name)
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
	size_t CAddStateDlg::GetStateCount() const
	{
		struct SStateVisitor
		{
			inline SStateVisitor()
				: stateCount(0)
			{}

			VisitStatus::EValue VisitState(const IDocStateConstPtr&)
			{
				++ stateCount;
				return VisitStatus::CONTINUE;
			}

			size_t	stateCount;
		};

		SStateVisitor	stateVisitor;
		m_doc.VisitStates(m_scopeGUID, MAKE_MEMBER_DELEGATE(SStateVisitor::VisitState, stateVisitor), false);
		return stateVisitor.stateCount;
	}
}