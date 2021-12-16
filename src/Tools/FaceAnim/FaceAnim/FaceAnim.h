// FaceAnim.h : main header file for the FaceAnim application
//
#pragma once

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"       // main symbols


// CFaceAnimApp:
// See FaceAnim.cpp for the implementation of this class
//

struct tFaceDetect;

//////////////////////////////////////////////////////////////////////////
class CFaceAnimApp : public CWinApp
{
public:
	CFaceAnimApp();
	
	char					m_szCalibrationFile[256];
	char					m_szWorkingFolder[256];

// Overrides
public:
	virtual BOOL InitInstance();

// Implementation
	afx_msg void OnAppAbout();
	DECLARE_MESSAGE_MAP()
};

extern CFaceAnimApp theApp;