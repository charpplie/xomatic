//////////////////////////////////  CRYTEK  ////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2011.
// -------------------------------------------------------------------------
//  File Name        : ScriptHelpDialog.cpp
//  Author           : Jaewon Jung
//  Time of creation : 4/12/2011   15:34
//  Compilers        : VS2008
//  Description      : For listing available script commands with their descriptions
// -------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "ScriptHelpDialog.h"
#include "Util/BoostPythonHelpers.h"
#include "Clipboard.h"

namespace
{
	enum
	{
		COMMAND_COL_IDX = 0,
		MODULE_COL_IDX,
		DESCRIPTION_COL_IDX,
		EXAMPLE_COL_IDX
	};
};

IMPLEMENT_DYNAMIC(CScriptHelpDialog, CXTResizeDialog)

void CScriptHelpDialog::DoDataExchange(CDataExchange* pDX)
{
	CXTResizeDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_SCRIPT_COMMAND_LIST, m_cmdListCtrl);
}

BEGIN_MESSAGE_MAP(CScriptHelpDialog, CXTResizeDialog)
	ON_WM_CLOSE()
	ON_NOTIFY(LVN_COLUMNCLICK, IDC_SCRIPT_COMMAND_LIST, OnHeaderClick)
	ON_NOTIFY(NM_DBLCLK, IDC_SCRIPT_COMMAND_LIST, OnListDblClk)
END_MESSAGE_MAP()

BOOL CScriptHelpDialog::OnInitDialog()
{
	CXTResizeDialog::OnInitDialog();

	CFont fntSmall;
	fntSmall.CreatePointFont(80, "Courier New");
	((CFilterHeaderCtrl*)m_cmdListCtrl.GetHeaderCtrl())->SetFilterFont(&fntSmall);
	m_cmdListCtrl.SetExtendedStyle(LVS_EX_HEADERDRAGDROP|LVS_EX_FULLROWSELECT);

	m_cmdListCtrl.InsertColumn(COMMAND_COL_IDX, "Command", LVCFMT_LEFT,100,-1, HEADER_FILTER_ENABLED);
	m_cmdListCtrl.InsertColumn(MODULE_COL_IDX, "Module", LVCFMT_LEFT,50,-1, HEADER_FILTER_ENABLED);
	m_cmdListCtrl.InsertColumn(DESCRIPTION_COL_IDX, "Description", LVCFMT_LEFT,300,-1, HEADER_FILTER_NONE);
	m_cmdListCtrl.InsertColumn(EXAMPLE_COL_IDX, "Example", LVCFMT_LEFT,200,-1, HEADER_FILTER_NONE);

	FillCommandList("", "");

	SetResize(IDC_SCRIPT_COMMAND_LIST, SZ_RESIZE(1));
	
return TRUE;
}

BOOL CScriptHelpDialog::OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult) 
{
	BOOL bDefault=TRUE;

	if((DWORD)wParam==(DWORD)m_cmdListCtrl.m_hWnd)
	{
		// notifications from list control

		NMHDR* pNM=(NMHDR*)lParam;

		if (pNM->code==FLCN_FILTERTEXTCHANGED)
		{
			CString commandFilter = ((CFilterHeaderCtrl*)m_cmdListCtrl.GetHeaderCtrl())->GetFilterText(COMMAND_COL_IDX);
			CString moduleFilter = ((CFilterHeaderCtrl*)m_cmdListCtrl.GetHeaderCtrl())->GetFilterText(MODULE_COL_IDX);
			m_cmdListCtrl.DeleteAllItems();
			FillCommandList(commandFilter, moduleFilter);

			bDefault=FALSE;
		}
	}		

	return (bDefault ? CXTResizeDialog::OnNotify(wParam, lParam, pResult) : TRUE);
}

void CScriptHelpDialog::FillCommandList(const CString& commandFilter, const CString& moduleFilter)
{
	m_cmdListCtrl.SetRedraw(FALSE);
	int i = 0;
	CAutoRegisterPythonCommandHelper *pCurrent = CAutoRegisterPythonCommandHelper::s_pFirst;
	while(pCurrent)
	{
		CString command = pCurrent->m_name.c_str(); 
		CString module = CAutoRegisterPythonModuleHelper::s_modules[pCurrent->m_moduleIndex].name.c_str();
		CString description = pCurrent->m_description.c_str();
		CString example = pCurrent->m_example.c_str();
		pCurrent = pCurrent->m_pNext;
	
		// Apply filters here.
		bool bFilteredOut = false;
		if(commandFilter.IsEmpty() == false)
		{
			if(command.Find(commandFilter) == -1)
				continue;
		}
		if(moduleFilter.IsEmpty() == false)
		{
			if(module.Find(moduleFilter) == -1)
				continue;
		}

		int nItem = m_cmdListCtrl.InsertItem(i, command);
		m_cmdListCtrl.SetItemData(nItem, i);
		m_cmdListCtrl.SetItemText(i, MODULE_COL_IDX, module);
		m_cmdListCtrl.SetItemText(i, DESCRIPTION_COL_IDX, description);
		m_cmdListCtrl.SetItemText(i, EXAMPLE_COL_IDX, example);
		++i;
	}
	m_cmdListCtrl.SetRedraw(TRUE);
	m_cmdListCtrl.Invalidate();
	m_cmdListCtrl.UpdateWindow();
}

void CScriptHelpDialog::OnClose()
{
	ShowWindow(SW_HIDE);
	SetWindowText("Script Help");
}

void CScriptHelpDialog::OnHeaderClick(NMHDR* pNMHDR, LRESULT* pResult)
{
	NMLISTVIEW* pLV = reinterpret_cast<NMLISTVIEW*>(pNMHDR);

	SetFocus();	// Ensure other controls gets kill-focus

	int colIndex = pLV->iSubItem;

	if (m_iSortColumn==colIndex)
	{
		m_bAscending = !m_bAscending;
	}
	else
	{
		m_iSortColumn = colIndex;
		m_bAscending = true;
	}

	SortColumn(m_iSortColumn, m_bAscending);
}

void CScriptHelpDialog::OnListDblClk(NMHDR* pNMHDR, LRESULT* pResult)
{
	UNREFERENCED_PARAMETER(pResult);

	LPNMITEMACTIVATE pia = (LPNMITEMACTIVATE)pNMHDR;
	LVHITTESTINFO lvhti;

	lvhti.pt = pia->ptAction;
	m_cmdListCtrl.SubItemHitTest(&lvhti);

	if(lvhti.flags & LVHT_ONITEMLABEL)
	{
		CClipboard clipboard;
		CString example = m_cmdListCtrl.GetItemText(lvhti.iItem, EXAMPLE_COL_IDX);
		CString title;
		if(example.IsEmpty() == false)
		{
			clipboard.PutString(example);
			title.Format("Script Help (Copied \"%s\" to clipboard)", example);
		}
		else
		{
			CString command = m_cmdListCtrl.GetItemText(lvhti.iItem, COMMAND_COL_IDX);
			CString module = m_cmdListCtrl.GetItemText(lvhti.iItem, MODULE_COL_IDX);
			CString fullCmd = module;
			fullCmd += ".";
			fullCmd += command;
			fullCmd += "()";
			clipboard.PutString(fullCmd);
			title.Format("Script Help (Copied \"%s\" to clipboard)", fullCmd);
		}
		SetWindowText(title);
	}
}

namespace {
	struct PARAMSORT
	{
		PARAMSORT(HWND hWnd, int columnIndex, bool ascending)
			:m_hWnd(hWnd)
			,m_ColumnIndex(columnIndex)
			,m_Ascending(ascending)
		{}

		HWND m_hWnd;
		int  m_ColumnIndex;
		bool m_Ascending;
	};

	// Comparison extracts values from the List-Control
	int CALLBACK SortFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
	{
		PARAMSORT& ps = *(PARAMSORT*)lParamSort;

		TCHAR left[256] = _T(""), right[256] = _T("");
		ListView_GetItemText(ps.m_hWnd, lParam1, ps.m_ColumnIndex, left, sizeof(left));
		ListView_GetItemText(ps.m_hWnd, lParam2, ps.m_ColumnIndex, right, sizeof(right));	
		left[sizeof(left)-1] = right[sizeof(right)-1] = '\0';

		if (ps.m_Ascending)
			return _tcscmp( left, right );
		else
			return _tcscmp( right, left );			
	}
}

bool CScriptHelpDialog::SortColumn(int columnIndex, bool ascending)
{
	PARAMSORT paramsort(m_cmdListCtrl.GetSafeHwnd(), columnIndex, ascending);
	m_cmdListCtrl.SortItemsEx(SortFunc, (DWORD_PTR)&paramsort);
	return true;
}