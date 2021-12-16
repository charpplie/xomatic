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

#ifndef __SCHEMATYC_QUICKSEARCHDLG_H__
#define __SCHEMATYC_QUICKSEARCHDLG_H__

// TODO : Move CQuickSearchEditCtrl inside CQuickSearchDlg?

namespace Schematyc
{
	struct IQuickSearchOptions
	{
		virtual ~IQuickSearchOptions() {}

		virtual size_t GetCount() const = 0;
		virtual const char* GetName(size_t iOption) const = 0;
	};

	class CQuickSearchEditCtrl : public CEdit
	{
		DECLARE_MESSAGE_MAP()

	private:

		afx_msg UINT OnGetDlgCode();
		afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	};

	class CQuickSearchDlg : public CDialog
	{
		DECLARE_MESSAGE_MAP()

		friend class CQuickSearchEditCtrl;

	public:

		CQuickSearchDlg(CWnd* pParent, CPoint pos, const IQuickSearchOptions& options);

		virtual ~CQuickSearchDlg();

		const size_t GetSelectedOption() const;

	private:

		typedef std::vector<stack_string> TStackStringVector;

		virtual BOOL OnInitDialog();
		virtual void DoDataExchange(CDataExchange* pDX);
		virtual void OnOK();

		afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
		afx_msg void OnFilterChanged();
		afx_msg void OnTreeSelChanged(NMHDR* pNMHDR, LRESULT* pResult);
		afx_msg void OnTreeDblClk(NMHDR* pNMHDR, LRESULT* pResult);

		void RefreshTreeCtrl();
		void RefreshOkButton();
		HTREEITEM FindTreeCtrlItem(HTREEITEM hRootItem, const char* name, bool recursive) const;
		HTREEITEM GetPrevTreeCtrlItem(HTREEITEM hItem);
		HTREEITEM GetNextTreeCtrlItem(HTREEITEM hItem, bool ignoreChildren);
		HTREEITEM GetLastTreeCtrlItemChild(HTREEITEM hItem);

		CPoint											m_pos;
		const IQuickSearchOptions&	m_options;
		CQuickSearchEditCtrl				m_filterCtrl;
		CTreeCtrl										m_treeCtrl;
		CButton											m_okButton;
		size_t											m_iSelectedOption;
	};
}

#endif __SCHEMATYC_QUICKSEARCHDLG_H__