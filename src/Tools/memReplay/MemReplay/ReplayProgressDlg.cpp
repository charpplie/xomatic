// ReplayProgressDlg.cpp : implementation file
//

#include "stdafx.h"
#include "MemReplay.h"
#include "ReplayProgressDlg.h"


// CProgressTaskDlg dialog

IMPLEMENT_DYNAMIC(CProgressTaskDlg, CDialog)

CProgressTaskDlg::CProgressTaskDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CProgressTaskDlg::IDD, pParent)
	, m_task(NULL)
	, m_thread(NULL)
{

}

CProgressTaskDlg::~CProgressTaskDlg()
{
}

void CProgressTaskDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_REPLAYPROGRESS, m_progressWnd);
}

void CProgressTaskDlg::Run(IProgressTask& task)
{
	m_task = &task;

	DoModal();
}

BEGIN_MESSAGE_MAP(CProgressTaskDlg, CDialog)
END_MESSAGE_MAP()


// CProgressTaskDlg message handlers

LRESULT CProgressTaskDlg::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	DWORD QueueStatus;
	LRESULT resValue = 0;
	bool OnIdleRetVal = true;

	if(message == WM_IDLE) 
	{
		OnIdleRetVal = OnIdle((UINT)wParam) != FALSE;
		if(!OnIdleRetVal)
			wParam = 0;
	}
	else if (message == WM_END)
	{
		EndDialog(IDOK);
		return resValue;
	}
	else
	{
		resValue = CDialog::WindowProc(message, wParam, lParam);
	}

	QueueStatus = GetQueueStatus(QS_ALLINPUT);

	if(HIWORD(QueueStatus) == 0)
		PostMessage(WM_IDLE, wParam + (OnIdleRetVal ? 1 : 0), 0);

	return resValue;
}

BOOL CProgressTaskDlg::OnIdle(UINT count)
{
	if (IsWindowVisible() == FALSE)
	{
		return TRUE;
	}

	m_progressWnd.SetRange32(0, m_task->GetMaximumProgress());

	if (!m_thread)
	{
		m_thread = AfxBeginThread(ProgressThreadProxy, this);
	}
	else
	{
		m_progressWnd.SetPos(m_task->GetProgress());
		Sleep(1);
	}

	return TRUE;
}

UINT CProgressTaskDlg::ProgressThread()
{
	m_task->Start();
	PostMessage(WM_END);

	return 0;
}


IMPLEMENT_DYNAMIC(CProgressReportDlg, CDialog)

CProgressReportDlg::CProgressReportDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CProgressReportDlg::IDD, pParent)
	, m_reporter(NULL)
{

}

CProgressReportDlg::~CProgressReportDlg()
{
}

void CProgressReportDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_REPLAYPROGRESS, m_progressWnd);
}

void CProgressReportDlg::Run(IProgressReporter& reporter)
{
	m_reporter = &reporter;

	DoModal();
}

BEGIN_MESSAGE_MAP(CProgressReportDlg, CDialog)
END_MESSAGE_MAP()


// CProgressReportDlg message handlers

LRESULT CProgressReportDlg::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	DWORD QueueStatus;
	LRESULT resValue = 0;
	bool OnIdleRetVal = true;

	if(message == WM_IDLE) 
	{
		OnIdleRetVal = OnIdle((UINT)wParam) != FALSE;
		if(!OnIdleRetVal)
			wParam = 0;
	}
	else if (message == WM_END)
	{
		EndDialog(IDOK);
		return resValue;
	}
	else
	{
		resValue = CDialog::WindowProc(message, wParam, lParam);
	}

	QueueStatus = GetQueueStatus(QS_ALLINPUT);

	if(HIWORD(QueueStatus) == 0)
		PostMessage(WM_IDLE, wParam + (OnIdleRetVal ? 1 : 0), 0);

	return resValue;
}

BOOL CProgressReportDlg::OnIdle(UINT count)
{
	if (IsWindowVisible() == FALSE)
	{
		return TRUE;
	}

	int progress = m_reporter->GetProgress();
	int total = m_reporter->GetMaximumProgress();

	m_progressWnd.SetRange32(0, total);
	m_progressWnd.SetPos(progress);

	if (progress == total)
	{
		PostMessage(WM_END);
	}
	else
	{
		Sleep(1);
	}

	return TRUE;
}
