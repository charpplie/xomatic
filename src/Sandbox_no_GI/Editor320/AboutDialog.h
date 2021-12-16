#ifndef _ABOUTDIALOG_H_
#define _ABOUTDIALOG_H_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// StartupLogoDialog.h : header file
//

#include "StartupTransparentText.h"

class CAboutDialog : public CDialog
{

	// Construction
public:
	CAboutDialog(const Version &v, CWnd* pParent = NULL);   // standard constructor
	~CAboutDialog();

	void SetVersion( const Version &v );

	// Dialog Data
	//{{AFX_DATA(CAboutDialog)
	enum { IDD = IDD_ABOUTBOX };
	// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAboutDialog)
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

	// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CStartupLogoDialog)
	virtual BOOL OnInitDialog();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnClose();
	afx_msg void OnPaint();
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	static CAboutDialog			*s_pAboutWindow;
	CStartupTransparentText		m_TransparentCopyright;
	CStartupTransparentText		m_TransparentVersion;
	CStartupTransparentText		m_TransparentAllRightReserved;
	CStartupTransparentText		m_TransparentTrademarks;
	CStartupTransparentText		m_TransparentDevelopedBy;
private:
	BITMAP						m_Bitmap;			// Struct to hold info about the bitmap
	CBitmap						m_hBitmap;			// Struct to hold the background bitmap	
	const static COLORREF		kTransparentColor;	// Color used for the transparency
	Version						m_version;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // _ABOUTDIALOG_H_