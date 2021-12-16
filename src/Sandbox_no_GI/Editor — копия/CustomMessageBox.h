#if !defined(AFX_CUSTOMBOXDIALOG_H__E1491920_A1B4_47B3_9556_CB049E1EAC7F__INCLUDED_)
#define AFX_CUSTOMBOXDIALOG_H__E1491920_A1B4_47B3_9556_CB049E1EAC7F__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// CustomBox.h : header file
//

// Custom Message box

/////////////////////////////////////////////////////////////////////////////
// Custom Message box Dialog

class CCustomMessageBox : public CDialog
{
// Construction
public:
	CCustomMessageBox(CWnd* pParent, LPCSTR lpText, LPCSTR lpCaption, LPCSTR lpButtonName1, LPCSTR lpButtonName2=0, LPCSTR lpButtonName3=0, LPCSTR lpButtonName4=0);

// Dialog Data
	//{{AFX_DATA(CCustomMessageBox)
	//}}AFX_DATA

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CCustomMessageBox)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CCustomMessageBox();
	int GetValue( ) {	return m_nValue; }
	static int Show(LPCSTR lpText, LPCSTR lpCaption, LPCSTR lpButtonName1, LPCSTR lpButtonName2=0, LPCSTR lpButtonName3=0, LPCSTR lpButtonName4=0);

protected:
	// Generated message map functions
	//{{AFX_MSG(CCustomMessageBox)
	afx_msg void OnButton1();
	afx_msg void OnButton2();
	afx_msg void OnButton3();
	afx_msg void OnButton4();
	virtual void OnOK();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()


	CString	m_message;
	CString	m_caption;
	CString	m_buttonName1, m_buttonName2, m_buttonName3, m_buttonName4;
	int m_nValue;
};

#endif // !defined(AFX_CUSTOMBOXDIALOG_H__E1491920_A1B4_47B3_9556_CB049E1EAC7F__INCLUDED_)