/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2014.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Schematyc create document dialog.
-------------------------------------------------------------------------
History:
- 01:01:2013: Created by Paul Slinger
*************************************************************************/

#include "StdAfx.h"

#include "Schematyc_CreateDocDlg.h"

#include <Schematyc/Schematyc_IDocManager.h>

#include "Resource.h"
#include "Schematyc_PluginUtils.h"

namespace Schematyc
{
	//////////////////////////////////////////////////////////////////////////
	CCreateDocDlg::CCreateDocDlg(CWnd* pParent, CPoint pos, const char* path)
		: CDialog(IDD_SCHEMATYC_CREATE_DOC, pParent)
		, m_pos(pos)
		, m_path(path)
	{}

	//////////////////////////////////////////////////////////////////////////
	BOOL CCreateDocDlg::OnInitDialog()
	{
		SetWindowPos(NULL, m_pos.x, m_pos.y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
		CDialog::OnInitDialog();
		return true;
	}

	//////////////////////////////////////////////////////////////////////////
	void CCreateDocDlg::DoDataExchange(CDataExchange* pDX)
	{
		DDX_Control(pDX, IDC_SCHEMATYC_DOC_NAME, m_nameCtrl);
		CDialog::DoDataExchange(pDX);
	}

	//////////////////////////////////////////////////////////////////////////
	void CCreateDocDlg::OnOK()
	{
		CString	name;
		m_nameCtrl.GetWindowText(name);
		if(ValidateFilePath(name.GetString()) == true)
		{
			stack_string	filePath = m_path;
			filePath.append("/");
			filePath.append(name.GetString());
			if(IDoc* pDoc = GetSchematycFramework().GetDocManager().CreateDoc(filePath.c_str()))
			{
				pDoc->Save();
			}
			CDialog::OnOK();
		}
	}

	//////////////////////////////////////////////////////////////////////////
	bool CCreateDocDlg::ValidateFilePath(const stack_string& filePath)
	{
		stack_string	errorMessage;
		if(PluginUtils::IsValidFilePath(filePath, errorMessage) == true)
		{
			return true;
		}
		else
		{
			MessageBox(errorMessage.c_str(), "Invalid File Name", MB_OK | MB_ICONEXCLAMATION);
			return false;
		}
	}
}