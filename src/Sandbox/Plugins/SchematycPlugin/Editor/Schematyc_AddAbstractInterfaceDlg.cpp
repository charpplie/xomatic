/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2014.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Schematyc add abstract interface dialog.
-------------------------------------------------------------------------
History:
- 11:07:2014: Created by Paul Slinger
*************************************************************************/

#include "StdAfx.h"

#include "Schematyc_AddAbstractInterfaceDlg.h"

#include <Schematyc/Schematyc_DocUtils.h>

#include "Resource.h"

namespace Schematyc
{
	//////////////////////////////////////////////////////////////////////////
	BEGIN_MESSAGE_MAP(CAddAbstractInterfaceDlg, CDialog)
		ON_LBN_SELCHANGE(IDD_SCHEMATYC_ABSTRACT_INTERFACES, OnAbstractInterfacesCtrlSelChange)
	END_MESSAGE_MAP()

	//////////////////////////////////////////////////////////////////////////
	CAddAbstractInterfaceDlg::CAddAbstractInterfaceDlg(CWnd* pParent, CPoint pos, IDoc& doc, const SGUID& objectGUID)
		: CDialog(IDD_SCHEMATYC_ADD_ABSTRACT_INTERFACE, pParent)
		, m_pos(pos)
		, m_doc(doc)
		, m_objectGUID(objectGUID)
	{}

	//////////////////////////////////////////////////////////////////////////
	CAddAbstractInterfaceDlg::~CAddAbstractInterfaceDlg() {}

	//////////////////////////////////////////////////////////////////////////
	BOOL CAddAbstractInterfaceDlg::OnInitDialog()
	{
		SetWindowPos(NULL, m_pos.x, m_pos.y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
		CDialog::OnInitDialog();

		GetSchematycFramework().GetEnvRegistry().VisitAbstractInterfaces(MAKE_MEMBER_DELEGATE(CAddAbstractInterfaceDlg::VisitAbstractInterface, *this));
		for(TGUIDStringPairVector::const_iterator iAbstractInterface = m_abstractInterfaces.begin(), iEndAbstractInterface = m_abstractInterfaces.end(); iAbstractInterface != iEndAbstractInterface; ++ iAbstractInterface)
		{
			m_abstractInterfacesCtrl.AddString(iAbstractInterface->second.c_str());
		}
		m_abstractInterfacesCtrl.SetCurSel(0);
		OnAbstractInterfacesCtrlSelChange();

		return true;
	}

	//////////////////////////////////////////////////////////////////////////
	void CAddAbstractInterfaceDlg::DoDataExchange(CDataExchange* pDX)
	{
		DDX_Control(pDX, IDD_SCHEMATYC_ABSTRACT_INTERFACES, m_abstractInterfacesCtrl);
		DDX_Control(pDX, IDD_SCHEMATYC_ABSTRACT_INTERFACE_DESCRIPTION, m_descriptionCtrl);
		CDialog::DoDataExchange(pDX);
	}

	//////////////////////////////////////////////////////////////////////////
	void CAddAbstractInterfaceDlg::OnAbstractInterfacesCtrlSelChange()
	{
		IAbstractInterfaceConstPtr	pAbstractInterface = GetSelectedAbstractInterface();
		CRY_ASSERT(pAbstractInterface != NULL);
		if(pAbstractInterface != NULL)
		{
			m_descriptionCtrl.SetWindowText(pAbstractInterface->GetTextDesc());
		}
		else
		{
			m_descriptionCtrl.SetWindowText("");
		}
	}

	//////////////////////////////////////////////////////////////////////////
	void CAddAbstractInterfaceDlg::OnOK()
	{
		IAbstractInterfaceConstPtr	pAbstractInterface = GetSelectedAbstractInterface();
		CRY_ASSERT(pAbstractInterface != NULL);
		if(pAbstractInterface != NULL)
		{
			m_doc.AddAbstractInterfaceInstance(m_objectGUID, pAbstractInterface->GetGUID());
		}
		CDialog::OnOK();
	}

	//////////////////////////////////////////////////////////////////////////
	VisitStatus::EValue CAddAbstractInterfaceDlg::VisitAbstractInterface(const IAbstractInterfaceConstPtr& pAbstractInterface)
	{
		m_abstractInterfaces.push_back(TGUIDStringPair(pAbstractInterface->GetGUID(), pAbstractInterface->GetName()));
		return VisitStatus::CONTINUE;
	}

	//////////////////////////////////////////////////////////////////////////
	IAbstractInterfaceConstPtr CAddAbstractInterfaceDlg::GetSelectedAbstractInterface() const
	{
		const int	curSel = m_abstractInterfacesCtrl.GetCurSel();
		if((curSel != LB_ERR) && (curSel >= 0))
		{
			return GetSchematycFramework().GetEnvRegistry().GetAbstractInterface(m_abstractInterfaces[curSel].first);
		}
		return NULL;
	}
}