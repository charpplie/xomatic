// PhotoBump10.h : main header file for the PhotoBump10 application
//
#pragma once

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"       // main symbols


// CPhotoBump10App:
// See PhotoBump10.cpp for the implementation of this class
//

class CPhotoBump10View;

//////////////////////////////////////////////////////////////////////////
class CPhotoBump10App : public CWinApp
{
public:
	CPhotoBump10App();


// Overrides
public:
	virtual BOOL	InitInstance();
	virtual BOOL	OnIdle(LONG lCount);

	CPhotoBump10View	*m_pView;
	bool	m_bCmdLineParsed;
	bool	m_bStandalone;		

// Implementation
	afx_msg void OnAppAbout();
	DECLARE_MESSAGE_MAP()
};

extern CPhotoBump10App theApp;