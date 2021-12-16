// PolyBumpApplication.h : main header file for the PolyBumpApplication application
//
#pragma once

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"       // main symbols



// CPolyBumpApplicationApp:
// See PolyBumpApplication.cpp for the implementation of this class
//

class CPolyBumpApplicationApp : public CWinApp
{
public:
	CPolyBumpApplicationApp();


// Overrides
public:
	virtual BOOL InitInstance();
//	virtual BOOL OnIdle(LONG lCount);

// Implementation
	afx_msg void OnAppAbout();
	DECLARE_MESSAGE_MAP()
	afx_msg void OnFileApplicationsettings();
};

extern CPolyBumpApplicationApp theApp;