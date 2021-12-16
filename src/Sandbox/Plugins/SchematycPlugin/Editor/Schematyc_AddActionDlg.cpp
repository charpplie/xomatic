/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2014.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Schematyc add action dialog.
-------------------------------------------------------------------------
History:
- 23:04:2014: Created by Paul Slinger
*************************************************************************/

#include "StdAfx.h"

#include "Schematyc_AddActionDlg.h"

#include <Include/IPropertyTree.h>

#include <Schematyc/Schematyc_DocUtils.h>

#include "Resource.h"
#include "Schematyc_PluginUtils.h"

namespace Schematyc
{
	//////////////////////////////////////////////////////////////////////////
	BEGIN_MESSAGE_MAP(CAddActionDlg, CDialog)
		ON_LBN_SELCHANGE(IDD_SCHEMATYC_ACTIONS, OnActionCtrlSelChange)
		ON_EN_CHANGE(IDD_SCHEMATYC_COMPONENT_NAME, OnNameCtrlSelChange)
	END_MESSAGE_MAP()

	//////////////////////////////////////////////////////////////////////////
	CAddActionDlg::CAddActionDlg(CWnd* pParent, CPoint pos, IDoc& doc, const SGUID& scopeGUID)
		: CDialog(IDD_SCHEMATYC_ADD_ACTION, pParent)
		, m_pos(pos)
		, m_doc(doc)
		, m_scopeGUID(scopeGUID)
		, m_nameModified(false)
	{}

	//////////////////////////////////////////////////////////////////////////
	BOOL CAddActionDlg::OnInitDialog()
	{
		SetWindowPos(NULL, m_pos.x, m_pos.y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
		CDialog::OnInitDialog();

		GetSchematycFramework().GetEnvRegistry().VisitActionFactories(MAKE_MEMBER_DELEGATE(CAddActionDlg::VisitActionFactory, *this));
		for(TActionVector::const_iterator iAction = m_actions.begin(), iEndAction = m_actions.end(); iAction != iEndAction; ++ iAction)
		{
			m_actionsCtrl.AddString(iAction->fullName.c_str());
		}
		m_actionsCtrl.SetCurSel(0);
		OnActionCtrlSelChange();

		return true;
	}

	//////////////////////////////////////////////////////////////////////////
	void CAddActionDlg::DoDataExchange(CDataExchange* pDX)
	{
		DDX_Control(pDX, IDD_SCHEMATYC_ACTIONS, m_actionsCtrl);
		DDX_Control(pDX, IDD_SCHEMATYC_ACTION_DESCRIPTION, m_descriptionCtrl);
		DDX_Control(pDX, IDD_SCHEMATYC_ACTION_NAME, m_nameCtrl);
		CDialog::DoDataExchange(pDX);
	}

	//////////////////////////////////////////////////////////////////////////
	void CAddActionDlg::OnActionCtrlSelChange()
	{
		const SAction		action = GetSelection();
		CString					name;
		m_nameCtrl.GetWindowText(name);
		if((m_nameModified == false) || (name.IsEmpty() == true))
		{
			m_nameCtrl.SetWindowText(action.name.c_str());
			m_nameModified = false;
		}
		m_descriptionCtrl.SetWindowText(action.textDesc.c_str());
	}

	//////////////////////////////////////////////////////////////////////////
	void CAddActionDlg::OnNameCtrlSelChange()
	{
		m_nameModified = true;
	}

	//////////////////////////////////////////////////////////////////////////
	void CAddActionDlg::OnOK()
	{
		CString	name;
		m_nameCtrl.GetWindowText(name);
		if(ValidateName(name.GetString()) == true)
		{
			const SAction	action = GetSelection();
			m_doc.AddActionInstance(m_scopeGUID, name.GetString(), action.guid, action.componentInstanceGUID);
			CDialog::OnOK();
		}
	}

	//////////////////////////////////////////////////////////////////////////
	VisitStatus::EValue CAddActionDlg::VisitActionFactory(const IActionFactoryConstPtr& pActionFactory)
	{
		const SGUID&	componentGUID = pActionFactory->GetComponentGUID();
		if(componentGUID.Empty() == false)
		{
			if(const IDocSchemaConstPtr& pDocSchema = DocUtils::FindOwnerSchema(m_doc, m_scopeGUID))
			{
				TDocComponentInstanceConstVector	componentInstances;
				DocUtils::CollectComponentInstances(m_doc, pDocSchema->GetGUID(), componentInstances, true);
				for(TDocComponentInstanceConstVector::const_iterator iComponentInstance = componentInstances.begin(), iEndComponentInstance = componentInstances.end(); iComponentInstance != iEndComponentInstance; ++ iComponentInstance)
				{
					const IDocComponentInstance&	componentInstance = *(*iComponentInstance);
					if(componentInstance.GetComponentGUID() == componentGUID)
					{
						const char*		name = pActionFactory->GetName();
						stack_string	fullName;
						DocUtils::GetFullElementName(m_doc, componentInstance, fullName);
						fullName.append("::");
						fullName.append(name);
						m_actions.push_back(SAction(pActionFactory->GetActionGUID(), componentInstance.GetGUID(), name, fullName.c_str(), pActionFactory->GetTextDesc()));
					}
				}
			}
		}
		else
		{
			const char*	scope = pActionFactory->GetScope();
			if(DocUtils::IsElementAvailableInScope(m_doc, m_scopeGUID, scope) == true)
			{
				const char*		name = pActionFactory->GetName();
				stack_string	fullName;
				EnvRegistryUtils::GetFullName(name, scope, componentGUID, fullName);
				m_actions.push_back(SAction(pActionFactory->GetActionGUID(), componentGUID, name, fullName.c_str(), pActionFactory->GetTextDesc()));
			}
		}
		return VisitStatus::CONTINUE;
	}

	//////////////////////////////////////////////////////////////////////////
	bool CAddActionDlg::ValidateName(const stack_string& name)
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
	CAddActionDlg::SAction CAddActionDlg::GetSelection() const
	{
		const int	curSel = m_actionsCtrl.GetCurSel();
		return (curSel != LB_ERR) && (curSel >= 0) ? m_actions[curSel] : SAction();
	}

	//////////////////////////////////////////////////////////////////////////
	CAddActionDlg::SAction::SAction() {}

	//////////////////////////////////////////////////////////////////////////
	CAddActionDlg::SAction::SAction(const SGUID& _guid, const SGUID& _componentInstanceGUID, const char* _name, const char* _fullName, const char* _textDesc)
		: guid(_guid)
		, componentInstanceGUID(_componentInstanceGUID)
		, name(_name)
		, fullName(_fullName)
		, textDesc(_textDesc)
	{}
}
