// MemReplay.h : main header file for the Demo application
//
#pragma once

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"       // main symbols
#include "ReplaySDK/IInterfaceHooks.h"

class IIdleHandler;
class CMainFrame;

// CMemReplayApp:
// See Demo.cpp for the implementation of this class
//

class CMemReplayApp : public CWinApp, public IInterfaceHooks
{
public:
	typedef void (*InvokeHandler)(void*);
	static const UINT MSG_INVOKE;

public:
	CMemReplayApp();

	void AddIdleHandler(IIdleHandler* handler);
	void RemoveIdleHandler(IIdleHandler* handler);

	UINT PopupMenu(UINT id);
	void ClipboardCopy(const char* text);

	static CMainFrame* GetMainFrame();

// Overrides
public:
	virtual BOOL InitInstance();
	int ExitInstance();

// Implementation
	afx_msg void OnAppAbout();
	afx_msg void OnAppDistribution();
	afx_msg void OnAppReverse();
	afx_msg void OnAppGatherTD();
	afx_msg void OnAppGatherBU();
	afx_msg void OnAppExclude();
	afx_msg void OnAppPlot();
	afx_msg void OnFileOpen();
	afx_msg void OnFileExportToXLS();
	afx_msg void OnEditCopy();
	afx_msg void OnEditFind();
	afx_msg void OnTypeInfoLookup();
	afx_msg void OnToolsValidateAllocations();
	afx_msg void OnToolsFindSizers();
	afx_msg void OnToolsFindOverrunSizers();
	afx_msg void OnToolsAnalyseBuckets();
	afx_msg void OnAppCombined();
	afx_msg void OnAppMainMem();
	afx_msg void OnAppRSXMem();
	afx_msg void OnAppCountMem();
	DECLARE_MESSAGE_MAP()

	CDocument *m_currentDoc;

protected:
	BOOL OnIdle(LONG lCount);

	BOOL PreTranslateMessage(MSG* pMsg);
	BOOL ProcessMessageFilter(int code, LPMSG lpMsg);

private:
	void InvokeOnMainImpl(InvokeHandler func, void* user);
	void ShowMessageImpl(const TCHAR* msg, const TCHAR* title);
	std::string OpenFileImpl(const char* filter, const TCHAR* title);
	std::string OpenFolderImpl(const TCHAR* title);
	void RunProgressTaskImpl(IProgressTask& task);

private:
	typedef std::vector<IIdleHandler*> IdleList;

private:
	IdleList m_idleList;
	HACCEL m_hAccel;
};

extern CMemReplayApp theApp;
