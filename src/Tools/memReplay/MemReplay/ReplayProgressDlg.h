#pragma once

#include "ReplaySDK/IInterfaceHooks.h"

// CProgressTaskDlg dialog


class CProgressTaskDlg : public CDialog
{
	DECLARE_DYNAMIC(CProgressTaskDlg)

public:
	CProgressTaskDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CProgressTaskDlg();

	void Run(IProgressTask& task);

// Dialog Data
	enum { IDD = IDD_REPLAYPROGRESSDLG };

protected:
	enum
	{
		WM_IDLE = WM_USER + 1,
		WM_END,
	};

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	LRESULT CProgressTaskDlg::WindowProc(UINT message, WPARAM wParam, LPARAM lParam);

	BOOL OnIdle(UINT count);

	DECLARE_MESSAGE_MAP()

private:
	static UINT __cdecl ProgressThreadProxy(LPVOID param)
	{
		return reinterpret_cast<CProgressTaskDlg*>(param)->ProgressThread();
	}

private:
	UINT ProgressThread();

private:
	IProgressTask* m_task;

	CWinThread* m_thread;
	int m_stepSize;

	CProgressCtrl m_progressWnd;
};

class CProgressReportDlg : public CDialog
{
	DECLARE_DYNAMIC(CProgressReportDlg)

public:
	CProgressReportDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CProgressReportDlg();

	void Run(IProgressReporter& reporter);

// Dialog Data
	enum { IDD = IDD_REPLAYPROGRESSDLG };

protected:
	enum
	{
		WM_IDLE = WM_USER + 1,
		WM_END,
	};

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);

	BOOL OnIdle(UINT count);

	DECLARE_MESSAGE_MAP()

private:
	UINT ProgressThread();

private:
	IProgressReporter* m_reporter;

	int m_stepSize;

	CProgressCtrl m_progressWnd;
};
