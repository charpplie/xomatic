#if !defined(AFX_STARTUPLOGODIALOG_H__F22FC7E2_D431_4746_BFB8_9D3E7EF36D8D__INCLUDED_)
#define AFX_STARTUPLOGODIALOG_H__F22FC7E2_D431_4746_BFB8_9D3E7EF36D8D__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// StartupLogoDialog.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CStartupLogoDialog dialog

#include "StartupTransparentText.h"

class CStartupLogoDialog : public CDialog, public IInitializeUIInfo
{

// Construction
public:
	CStartupLogoDialog(CWnd* pParent = NULL);   // standard constructor
	~CStartupLogoDialog();

	void SetVersion( const Version &v );
	void SetInfo( const char *text );

	static void SetText( const char *text );

	virtual void SetInfoText( const char *text );


// Dialog Data
	//{{AFX_DATA(CStartupLogoDialog)
	enum { IDD = IDD_STARTUP_LOGO };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CStartupLogoDialog)
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
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	static CStartupLogoDialog	*s_pLogoWindow;
	CStartupTransparentText		m_TransparentText;
	CStartupTransparentText		m_TransparentVersion;
private:
	BITMAP						m_Bitmap;			// Struct to hold info about the bitmap
	CBitmap						m_hBitmap;			// Struct to hold the background bitmap	
	const static COLORREF		kTransparentColor;	// Color used for the transparency
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STARTUPLOGODIALOG_H__F22FC7E2_D431_4746_BFB8_9D3E7EF36D8D__INCLUDED_)
