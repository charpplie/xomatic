/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2014.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Schematyc quick search dialog.
-------------------------------------------------------------------------
History:
- 01:01:2013: Created by Paul Slinger
*************************************************************************/

#include "StdAfx.h"

#include "Schematyc_QuickSearchDlg.h"

#include <Schematyc/Schematyc_StringUtils.h>

#include "Resource.h"
#include "Schematyc_PluginUtils.h"

namespace Schematyc
{
	//////////////////////////////////////////////////////////////////////////
	BEGIN_MESSAGE_MAP(CQuickSearchEditCtrl, CEdit)
		ON_WM_GETDLGCODE()
		ON_WM_KEYDOWN()
	END_MESSAGE_MAP()

	//////////////////////////////////////////////////////////////////////////
	UINT CQuickSearchEditCtrl::OnGetDlgCode()
	{
		return __super::OnGetDlgCode() | DLGC_WANTALLKEYS; 
	}

	//////////////////////////////////////////////////////////////////////////
	void CQuickSearchEditCtrl::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
	{
		if(CQuickSearchDlg* pParent = static_cast<CQuickSearchDlg*>(GetParent()))
		{
			pParent->OnKeyDown(nChar, nRepCnt, nFlags);
		}
	}

	//////////////////////////////////////////////////////////////////////////
	BEGIN_MESSAGE_MAP(CQuickSearchDlg, CDialog)
		ON_WM_KEYDOWN()
		ON_EN_UPDATE(IDC_SCHEMATYC_QUICK_SEARCH_FILTER, OnFilterChanged)
		ON_NOTIFY(TVN_SELCHANGED, IDC_SCHEMATYC_QUICK_SEARCH_TREE, OnTreeSelChanged)
		ON_NOTIFY(NM_DBLCLK, IDC_SCHEMATYC_QUICK_SEARCH_TREE, OnTreeDblClk)
	END_MESSAGE_MAP()

	//////////////////////////////////////////////////////////////////////////
	CQuickSearchDlg::CQuickSearchDlg(CWnd* pParent, CPoint pos, const IQuickSearchOptions& options)
		: CDialog(IDD_SCHEMATYC_QUICK_SEARCH, pParent)
		, m_pos(pos)
		, m_options(options)
		, m_iSelectedOption(INVALID_INDEX)
	{}

	//////////////////////////////////////////////////////////////////////////
	CQuickSearchDlg::~CQuickSearchDlg() {}

	//////////////////////////////////////////////////////////////////////////
	const size_t CQuickSearchDlg::GetSelectedOption() const
	{
		return m_iSelectedOption;
	}

	//////////////////////////////////////////////////////////////////////////
	BOOL CQuickSearchDlg::OnInitDialog()
	{
		SetWindowPos(NULL, m_pos.x, m_pos.y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
		CDialog::OnInitDialog();
		RefreshTreeCtrl();
		return true;
	}

	//////////////////////////////////////////////////////////////////////////
	void CQuickSearchDlg::DoDataExchange(CDataExchange* pDX)
	{
		DDX_Control(pDX, IDC_SCHEMATYC_QUICK_SEARCH_FILTER, m_filterCtrl);
		DDX_Control(pDX, IDC_SCHEMATYC_QUICK_SEARCH_TREE, m_treeCtrl);
		DDX_Control(pDX, IDOK, m_okButton);
		CDialog::DoDataExchange(pDX);
	}

	//////////////////////////////////////////////////////////////////////////
	void CQuickSearchDlg::OnOK()
	{
		CRY_ASSERT(m_okButton.IsWindowEnabled());
		if(HTREEITEM hSelectedItem = m_treeCtrl.GetSelectedItem())
		{
			const size_t	iSelectedOption = m_treeCtrl.GetItemData(hSelectedItem);
			if(iSelectedOption < m_options.GetCount())
			{
				m_iSelectedOption = iSelectedOption;
			}
		}
		__super::OnOK();
	}

	//////////////////////////////////////////////////////////////////////////
	void CQuickSearchDlg::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
	{
		switch(nChar)
		{
			case VK_UP:
			{
				if(HTREEITEM hPrevItem = GetPrevTreeCtrlItem(m_treeCtrl.GetSelectedItem()))
				{
					m_treeCtrl.SelectItem(hPrevItem);
				}
				break;
			}
			case VK_DOWN:
			{
				if(HTREEITEM hNextItem = GetNextTreeCtrlItem(m_treeCtrl.GetSelectedItem(), false))
				{
					m_treeCtrl.SelectItem(hNextItem);
				}
				break;
			}
			case VK_RETURN:
			{
				if(m_okButton.IsWindowEnabled())
				{
					OnOK();
					EndDialog(TRUE);
				}
				break;
			}
			case VK_ESCAPE:
			{
				EndDialog(FALSE);
				break;
			}
			default:
			{
				__super::OnKeyDown(nChar, nRepCnt, nFlags);
				break;
			}
		}
	}

	//////////////////////////////////////////////////////////////////////////
	void CQuickSearchDlg::OnFilterChanged()
	{
		RefreshTreeCtrl();
		RefreshOkButton();
	}

	//////////////////////////////////////////////////////////////////////////
	void CQuickSearchDlg::OnTreeSelChanged(NMHDR* pNMHDR, LRESULT* pResult)
	{
		RefreshOkButton();
	}

	//////////////////////////////////////////////////////////////////////////
	void CQuickSearchDlg::OnTreeDblClk(NMHDR* pNMHDR, LRESULT* pResult)
	{
		CPoint	pos;
		GetCursorPos(&pos);
		m_treeCtrl.ScreenToClient(&pos);
		UINT			flags = 0;
		HTREEITEM	hTreeItem = m_treeCtrl.HitTest(pos, &flags);
		if((flags & TVHT_ONITEM) && !m_treeCtrl.ItemHasChildren(hTreeItem))
		{
			OnOK();
			EndDialog(IDOK);
		}
	}

	//////////////////////////////////////////////////////////////////////////
	void CQuickSearchDlg::RefreshTreeCtrl()
	{
		const size_t	optionCount = m_options.GetCount();
		size_t				iPrevSelectedOption = optionCount;
		if(HTREEITEM hSelectedItem = m_treeCtrl.GetSelectedItem())
		{
			iPrevSelectedOption = m_treeCtrl.GetItemData(hSelectedItem);
		}

		m_treeCtrl.DeleteAllItems();
		HTREEITEM	hSelectedItem = NULL;
		CString		filter;
		m_filterCtrl.GetWindowText(filter);
		for(size_t iOption = 0; iOption < optionCount; ++ iOption)
		{
			const char*	option = m_options.GetName(iOption);
			if((filter.IsEmpty() == true) || (StringUtils::FilterString(option, filter.GetString()) == true))
			{
				stack_string				tokenizedOption = option;
				const size_t				length = tokenizedOption.length();
				TStackStringVector	tokens;
				int									pos = 0;
				do
				{
					tokens.push_back(tokenizedOption.Tokenize("::", pos));
				} while(pos < length);

				HTREEITEM	hParentItem = NULL;
				for(size_t iToken = 0, tokenCount = tokens.size(); iToken < tokenCount; ++ iToken)
				{
					const char*	token = tokens[iToken].c_str();
					HTREEITEM		hItem = FindTreeCtrlItem(hParentItem, token, false);
					if(!hItem)
					{
						hItem = m_treeCtrl.InsertItem(token, 0, 0, hParentItem, TVI_LAST);
						m_treeCtrl.EnsureVisible(hItem);
						if(iToken == (tokenCount - 1))
						{
							m_treeCtrl.SetItemData(hItem, iOption);
							if((iOption == iPrevSelectedOption) || !m_treeCtrl.GetSelectedItem())
							{
								m_treeCtrl.SelectItem(hItem);
							}
						}
						else
						{
							m_treeCtrl.SetItemData(hItem, optionCount);
						}
					}
					CRY_ASSERT(hItem);
					hParentItem = hItem;
				}
			}
		}
		m_treeCtrl.EnsureVisible(m_treeCtrl.GetRootItem());
	}

	//////////////////////////////////////////////////////////////////////////
	void CQuickSearchDlg::RefreshOkButton()
	{
		bool	enableOkButton = false;
		if(HTREEITEM hSelectedItem = m_treeCtrl.GetSelectedItem())
		{
			enableOkButton = m_treeCtrl.GetItemData(hSelectedItem) < m_options.GetCount();
		}
		m_okButton.EnableWindow(enableOkButton);
	}

	//////////////////////////////////////////////////////////////////////////
	HTREEITEM CQuickSearchDlg::FindTreeCtrlItem(HTREEITEM hRootItem, const char* name, bool recursive) const
	{
		CRY_ASSERT(name);
		if(name)
		{
			for(HTREEITEM hItem = m_treeCtrl.GetChildItem(hRootItem); hItem; hItem = m_treeCtrl.GetNextSiblingItem(hItem))
			{
				if(stricmp(m_treeCtrl.GetItemText(hItem), name) == 0)
				{
					return hItem;
				}

				if(recursive)
				{
					if(HTREEITEM hChildItem = FindTreeCtrlItem(hItem, name, true))
					{
						return hChildItem;
					}
				}
			}
		}
		return NULL;
	}

	//////////////////////////////////////////////////////////////////////////
	HTREEITEM CQuickSearchDlg::GetPrevTreeCtrlItem(HTREEITEM hItem)
	{
		if(hItem)
		{
			if(HTREEITEM hPrevSiblingItem = m_treeCtrl.GetPrevSiblingItem(hItem))
			{
				return GetLastTreeCtrlItemChild(hPrevSiblingItem);
			}
			else if(HTREEITEM hParentItem = m_treeCtrl.GetParentItem(hItem))
			{
				return hParentItem;
			}
		}
		return NULL;
	}

	//////////////////////////////////////////////////////////////////////////
	HTREEITEM CQuickSearchDlg::GetNextTreeCtrlItem(HTREEITEM hItem, bool ignoreChildren)
	{
		if(hItem)
		{
			if(!ignoreChildren)
			{
				if(HTREEITEM hChildItem = m_treeCtrl.GetChildItem(hItem))
				{
					return hChildItem;
				}
			}
			if(HTREEITEM hNextSiblingItem = m_treeCtrl.GetNextSiblingItem(hItem))
			{
				return hNextSiblingItem;
			}
			else
			{
				return GetNextTreeCtrlItem(m_treeCtrl.GetParentItem(hItem), true);
			}
		}
		return NULL;
	}

	//////////////////////////////////////////////////////////////////////////
	HTREEITEM CQuickSearchDlg::GetLastTreeCtrlItemChild(HTREEITEM hItem)
	{
		HTREEITEM	hLastChildItem = hItem;
		if(hItem)
		{
			for(HTREEITEM hChildItem = m_treeCtrl.GetChildItem(hItem); hChildItem; hChildItem = m_treeCtrl.GetNextSiblingItem(hChildItem))
			{
				hLastChildItem = hChildItem;
			}
		}
		return hLastChildItem != hItem ? GetLastTreeCtrlItemChild(hLastChildItem) : hLastChildItem;
	}
}