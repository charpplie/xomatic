#if !defined(AFX_SETVECTORDLG_H__45679345_0046_4354_986F_888F473684E5__INCLUDED_)
#define AFX_SETVECTORDLG_H__45679345_0046_4354_986F_888F473684E5__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// GotoPositionDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CSetVectorDlg dialog

class CSetVectorDlg : public CDialog
{
	// Construction
public:
	CSetVectorDlg(CWnd* pParent = NULL);   // standard constructor

	// Dialog Data
	//{{AFX_DATA(CSetVectorDlg)
	enum { IDD = IDD_SETVECTOR };
	// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSetVectorDlg)
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

	// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CSetVectorDlg)
	// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG

	virtual BOOL OnInitDialog();
	afx_msg void OnBnClickedOk();
	void SetVector(const Vec3 &v);
	Vec3 GetVectorFromText();
	Vec3 GetVectorFromEditor();
	Vec3 currentVec;

	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SETVECTORDLG_H__45679345_0046_4354_986F_888F473684E5__INCLUDED_)