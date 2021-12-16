// EngineSettingsManager.h : main header file for the PROJECT_NAME application
//

#pragma once

#ifndef __AFXWIN_H__
	#error "include 'stdafx.h' before including this file for PCH"
#endif

#include "resource.h"		// main symbols



// used to load common XP controls
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")


// CEngineSettingsManagerApp:
// See EngineSettingsManager.cpp for the implementation of this class
//

class CEngineSettingsManagerApp : public CWinApp
{
public:
	CEngineSettingsManagerApp();

// Overrides
	public:
	virtual BOOL InitInstance();

// Implementation

	DECLARE_MESSAGE_MAP()
};

extern CEngineSettingsManagerApp theApp;