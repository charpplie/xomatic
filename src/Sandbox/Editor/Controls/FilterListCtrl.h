//////////////////////////////////  CRYTEK  ////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2011.
// -------------------------------------------------------------------------
//  File Name        : FilterListCtrl.h
//  Author           : Jaewon Jung
//  Time of creation : 4/12/2011   15:11
//  Compilers        : VS2008
//  Description      : List and header controls with filtering support per column
//  Notice           : http://www.codeproject.com/KB/list/filterheaderctrl.aspx
// -------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////
#if !defined(AFX_FILTERLISTCTRL_H__05FBF341_AD76_40E4_97C0_959D63420FA5__INCLUDED_)
#define AFX_FILTERLISTCTRL_H__05FBF341_AD76_40E4_97C0_959D63420FA5__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// FilterListCtrl.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CFilterListCtrl window

// notify struct
struct NMFILTERHDR : public NMHDR
{
	char* szText; // read only!!!
};

// external notify messages
#define	FLCN_FILTERCHANGING		101
#define	FLCN_FILTERCHANGED		102
#define	FLCN_SHOWINGEDIT		103
#define	FLCN_BEGINFILTEREDIT	104
#define	FLCN_ENDFILTEREDIT		105
#define	FLCN_FILTERTEXTCHANGED	106

// const
#define HEADER_FILTER_NONE				0
#define HEADER_FILTER_ENABLED			1
#define HEADER_FILTER_DISABLED			2

/////////////////////////////////////////////////////////////////////////////
// CFilterHeaderCtrl window

// Filter Edit Control ID
#define IDC_ED_EDIT		101

// internal messages
#define	FLM_EDITTEXTCHANGED		(WM_USER+100)
#define	FLM_FILTERTEXTCHANGED	(WM_USER+101)

class CFilterHeaderCtrl : public CHeaderCtrl
{
	class CFilterEdit : public CEdit
	{
	// Construction
	public:
		CFilterEdit();

	// Attributes
	public:

		BOOL m_bNotifySent;

	// Operations
	public:

	// Overrides
		// ClassWizard generated virtual function overrides
		//{{AFX_VIRTUAL(CFilterEdit)
		public:
		virtual BOOL PreTranslateMessage(MSG* pMsg);
		virtual void PostNcDestroy( ); 
		//}}AFX_VIRTUAL

	// Implementation
	public:
		virtual ~CFilterEdit();

		// Generated message map functions
	protected:
		//{{AFX_MSG(CFilterEdit)
		afx_msg void OnKillFocus(CWnd* pNewWnd);
		afx_msg void OnChange();
		//}}AFX_MSG

		DECLARE_MESSAGE_MAP()

		CString m_lastFilterText;
	};

	class CFilterInfo
	{
	public:
		CFilterInfo()
		{
			m_nStatus=HEADER_FILTER_ENABLED;
		};

		CString m_strFilter;
		int		m_nStatus;
	};

// Construction
public:
	CFilterHeaderCtrl();

// Attributes
public:

private:
	CFilterEdit*	m_pEdit;
	int m_nEditColumn;
	CFont	m_FilterFont;
	CString m_strFilterDisabled;
	CPtrArray m_arFilters;
	int m_nFont1Height;
	int m_nFont2Height;
	BOOL	m_bEndEditSent;
	int m_nSelectedColumn;

	int m_spacing;
	COLORREF m_cr3DHighLight;
	COLORREF m_cr3DShadow;
	COLORREF m_cr3DFace;
	COLORREF m_crText;

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CFilterHeaderCtrl)
	protected:
	virtual LRESULT DefWindowProc(UINT message, WPARAM wParam, LPARAM lParam);
	//}}AFX_VIRTUAL

// Implementation
public:
	void SetFilterDisabledString(CString strText);
	void SetFilterStatus(int nIndex, UINT uStatus);

private:
	CFilterInfo* GetFilter(int nIndex)
	{
		return (CFilterInfo*)m_arFilters[nIndex];
	}

public:
	CString GetFilterText(int nIndex)
	{
		return GetFilter(nIndex)->m_strFilter;
	}

	int GetFilterStatus(int nIndex)
	{
		return GetFilter(nIndex)->m_nStatus;
	}

private:
	void HideEdit(BOOL bValidate);
	void ShowEdit(int nColumn);
	void DrawCtrl(CDC* pDC);

public:
	int IndexToOrder(int nIndex);
	BOOL FilterEditing();
	void SetFilterFont(CFont* pFont);
	void CalcFontHeight();
	virtual ~CFilterHeaderCtrl();
	virtual void DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct);

	// Generated message map functions
protected:
	//{{AFX_MSG(CFilterHeaderCtrl)
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg LRESULT OnEditTextChanged(WPARAM, LPARAM);
	afx_msg LRESULT OnFilterTextChanged(WPARAM, LPARAM);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnPaint();
	afx_msg void OnSysColorChange();
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

class CFilterListCtrl : public CListCtrl
{
// Construction
public:
	CFilterListCtrl();

// Attributes
public:

private:
	CFilterHeaderCtrl	m_ctlHeader;

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CFilterListCtrl)
	protected:
	virtual void PreSubclassWindow();
	virtual BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult);
	//}}AFX_VIRTUAL

// Implementation
public:

	int InsertColumn(int nCol, const LVCOLUMN* pColumn,int nFilter=HEADER_FILTER_ENABLED);
	int InsertColumn(int nCol, LPCTSTR lpszColumnHeading,
		int nFormat = LVCFMT_LEFT, int nWidth = -1, int nSubItem = -1,int nFilter=HEADER_FILTER_ENABLED);

	virtual ~CFilterListCtrl();

	// Generated message map functions
protected:
	//{{AFX_MSG(CFilterListCtrl)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.


#endif // !defined(AFX_FILTERLISTCTRL_H__05FBF341_AD76_40E4_97C0_959D63420FA5__INCLUDED_)
