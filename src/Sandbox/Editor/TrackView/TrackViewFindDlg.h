#if !defined(AFX_TRACKVIEWFINDDLG_H__AFB020BD_DBC3_40C0_A04C_8337A6530F3B__INCLUDED_)
#define AFX_TRACKVIEWFINDDLG_H__AFB020BD_DBC3_40C0_A04C_8337A6530F3B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// TrackViewFindDlg.h : header file
//
#include "..\ToolbarDialog.h"

class CTrackViewDialog;

/////////////////////////////////////////////////////////////////////////////
// CTrackViewFindDlg dialog
class CTrackViewFindDlg : public CToolbarDialog
{
	// Construction
public:
	CTrackViewFindDlg( const char *title = NULL,CWnd* pParent = NULL);   // standard constructor

	// Dialog Data
	//{{AFX_DATA(CTrackViewFindDlg)
	enum { IDD = IDD_TRACKVIEWFINDDLG };
	//}}AFX_DATA

	//Functions
	void FillData();
	void FillList();
	void Init(CTrackViewDialog * tvDlg);
	void ProcessSel();

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CTrackViewFindDlg)
protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	//}}AFX_VIRTUAL

	// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CTrackViewFindDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnDestroy();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnLbnDblClk();
	afx_msg void OnFilterChange();
	afx_msg void OnOK();
	afx_msg void OnCancel();
	afx_msg void OnClose();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	CEdit m_filter;
	CListBox m_list;

	struct ObjName{
		CString m_objName;
		CString m_seqName;
	};

	std::vector<ObjName> m_objs;
	CTrackViewDialog * m_tvDlg;

	int m_numSeqs;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_TRACKVIEWFINDDLG_H__AFB020BD_DBC3_40C0_A04C_8337A6530F3B__INCLUDED_)
