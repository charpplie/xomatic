/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2014.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Schematyc property tree dialog.
-------------------------------------------------------------------------
History:
- 25:11:2014: Created by Paul Slinger
*************************************************************************/

#include "StdAfx.h"

#include "Schematyc_PropertyTreeDlg.h"

#include <Include/IPropertyTree.h>

#include "Resource.h"
#include "Schematyc_PluginUtils.h"

namespace Schematyc
{
	//////////////////////////////////////////////////////////////////////////
	CPropertyTreeDlg::CPropertyTreeDlg(CWnd* pParent, CPoint pos, const char* name, Serialization::SStruct& properties)
		: CDialog(IDD_SCHEMATYC_PROPERTY_TREE, pParent)
		, m_pos(pos)
		, m_name(name)
		, m_properties(properties)
		, m_pPropertyTree(NULL)
	{}

	//////////////////////////////////////////////////////////////////////////
	CPropertyTreeDlg::~CPropertyTreeDlg()
	{
		SAFE_RELEASE(m_pPropertyTree);
	}

	//////////////////////////////////////////////////////////////////////////
	BOOL CPropertyTreeDlg::OnInitDialog()
	{
		SetWindowPos(NULL, m_pos.x, m_pos.y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
		CDialog::OnInitDialog();
		CDialog::SetWindowText(m_name.c_str());

		CWnd*	pDlgItem = GetDlgItem(IDC_SCHEMATYC_PROPERTY_TREE);
		CRY_ASSERT(pDlgItem != NULL);
		if(pDlgItem != NULL)
		{
			RECT	rect;
			pDlgItem->GetClientRect(&rect);
			m_pPropertyTree = CreatePropertyTree(GetDlgItem(IDC_SCHEMATYC_PROPERTY_TREE), rect);
			m_pPropertyTree->Attach(m_properties);
			m_pPropertyTree->SetExpandLevels(1);
			return true;
		}
		return false;
	}

	//////////////////////////////////////////////////////////////////////////
	void CPropertyTreeDlg::DoDataExchange(CDataExchange* pDX)
	{
		CDialog::DoDataExchange(pDX);
	}

	//////////////////////////////////////////////////////////////////////////
	void CPropertyTreeDlg::OnOK()
	{
		m_pPropertyTree->Detach();
		__super::OnOK();
	}
}