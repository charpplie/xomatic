/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2014.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Schematyc add component dialog.
-------------------------------------------------------------------------
History:
- 23:04:2014: Created by Paul Slinger
*************************************************************************/

#include "StdAfx.h"

#include "Schematyc_AddComponentDlg.h"

#include <Include/IPropertyTree.h>

#include <Schematyc/Schematyc_DocUtils.h>

#include "Resource.h"
#include "Schematyc_PluginUtils.h"

namespace Schematyc
{
	//////////////////////////////////////////////////////////////////////////
	BEGIN_MESSAGE_MAP(CAddComponentDlg, CDialog)
		ON_LBN_SELCHANGE(IDD_SCHEMATYC_COMPONENTS, OnComponentsCtrlSelChange)
		ON_EN_CHANGE(IDD_SCHEMATYC_COMPONENT_NAME, OnNameCtrlSelChange)
	END_MESSAGE_MAP()

	//////////////////////////////////////////////////////////////////////////
	CAddComponentDlg::CAddComponentDlg(CWnd* pParent, CPoint pos, IDoc& doc, const SGUID& scopeGUID)
		: CDialog(IDD_SCHEMATYC_ADD_COMPONENT, pParent)
		, m_pos(pos)
		, m_doc(doc)
		, m_scopeGUID(scopeGUID)
		, m_nameModified(false)
	{}

	//////////////////////////////////////////////////////////////////////////
	BOOL CAddComponentDlg::OnInitDialog()
	{
		SetWindowPos(NULL, m_pos.x, m_pos.y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
		CDialog::OnInitDialog();

		GetSchematycFramework().GetEnvRegistry().VisitComponentFactories(MAKE_MEMBER_DELEGATE(CAddComponentDlg::VisitComponentFactory, *this));
		for(TGUIDStringPairVector::const_iterator iComponent = m_components.begin(), iEndComponent = m_components.end(); iComponent != iEndComponent; ++ iComponent)
		{
			m_componentsCtrl.AddString(iComponent->second.c_str());
		}
		m_componentsCtrl.SetCurSel(0);
		OnComponentsCtrlSelChange();

		return true;
	}

	//////////////////////////////////////////////////////////////////////////
	void CAddComponentDlg::DoDataExchange(CDataExchange* pDX)
	{
		DDX_Control(pDX, IDD_SCHEMATYC_COMPONENTS, m_componentsCtrl);
		DDX_Control(pDX, IDD_SCHEMATYC_COMPONENT_DESCRIPTION, m_descriptionCtrl);
		DDX_Control(pDX, IDD_SCHEMATYC_COMPONENT_NAME, m_nameCtrl);
		CDialog::DoDataExchange(pDX);
	}

	//////////////////////////////////////////////////////////////////////////
	void CAddComponentDlg::OnComponentsCtrlSelChange()
	{
		IComponentFactoryConstPtr	pComponentFactory = GetSchematycFramework().GetEnvRegistry().GetComponentFactory(GetSelection());
		CRY_ASSERT(pComponentFactory != NULL);
		if(pComponentFactory != NULL)
		{
			m_descriptionCtrl.SetWindowText(pComponentFactory->GetTextDesc());
			CString	name;
			m_nameCtrl.GetWindowText(name);
			if((m_nameModified == false) || (name.IsEmpty() == true))
			{
				m_nameCtrl.SetWindowText(pComponentFactory->GetName());
				m_nameModified = false;
			}
		}
		else
		{
			m_descriptionCtrl.SetWindowText("");
		}
	}

	//////////////////////////////////////////////////////////////////////////
	void CAddComponentDlg::OnNameCtrlSelChange()
	{
		m_nameModified = true;
	}

	//////////////////////////////////////////////////////////////////////////
	void CAddComponentDlg::OnOK()
	{
		CString	name;
		m_nameCtrl.GetWindowText(name);
		if(ValidateName(name.GetString()) == true)
		{
			m_doc.AddComponentInstance(m_scopeGUID, name.GetString(), GetSelection(), DocComponentInstanceFlags::NONE);
			CDialog::OnOK();
		}
	}

	//////////////////////////////////////////////////////////////////////////
	VisitStatus::EValue CAddComponentDlg::VisitComponentFactory(const IComponentFactoryConstPtr& pComponentFactory)
	{
		const char*	scope = pComponentFactory->GetScope();
		if(DocUtils::IsElementAvailableInScope(m_doc, m_scopeGUID, scope))
		{
			stack_string	componentName = scope;
			if(componentName[0] != '\0')
			{
				componentName.append("::");
			}
			componentName.append(pComponentFactory->GetName());
			m_components.push_back(TGUIDStringPair(pComponentFactory->GetComponentGUID(), componentName));
		}
		return VisitStatus::CONTINUE;
	}

	//////////////////////////////////////////////////////////////////////////
	bool CAddComponentDlg::ValidateName(const stack_string& name)
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
	SGUID CAddComponentDlg::GetSelection() const
	{
		const int	curSel = m_componentsCtrl.GetCurSel();
		return (curSel != LB_ERR) && (curSel >= 0) ? m_components[curSel].first : SGUID();
	}
}