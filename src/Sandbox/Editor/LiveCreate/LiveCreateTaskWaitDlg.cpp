#include "StdAfx.h"
#include "LiveCreateTaskWaitDlg.h"

#ifndef NO_LIVECREATE

#define ID_TIMER_UPDATE_STATUS 1002

BEGIN_MESSAGE_MAP(CLiveCreateTaskWaitDlg, CDialog)
	ON_BN_CLICKED(IDCANCEL, &CLiveCreateTaskWaitDlg::OnCancel)
	ON_WM_DESTROY()
	ON_WM_TIMER()
END_MESSAGE_MAP()


bool CLiveCreateTaskWaitDlg::ShowDialog(CWnd* pParent, IBackgroundTask* pTask, const CString& title, const CString& text)
{
	CLiveCreateTaskWaitDlg dlg(pParent, pTask, title, text);

	if (dlg.DoModal() == IDCANCEL)
		return false;

	return true;
}

CLiveCreateTaskWaitDlg::CLiveCreateTaskWaitDlg(CWnd* pParent, IBackgroundTask* pTask, const CString& title, const CString& text)
	: CDialog(IDD_LIVECREATE_TASK_WAIT, pParent)
	, m_pTask(pTask)
	, m_counter(0)
	, m_baseText(text)
	, m_title(title)
{
	m_pTask->AddRef();
}

CLiveCreateTaskWaitDlg::~CLiveCreateTaskWaitDlg()
{
	SAFE_RELEASE(m_pTask);
}

BOOL CLiveCreateTaskWaitDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	SetTimer(ID_TIMER_UPDATE_STATUS, 200, NULL);

	CWnd* pText = GetDlgItem(IDC_TASK_PROGRESS_TEXT);
	if (pText != NULL)
	{
		pText->SetWindowText(m_baseText);
	}

	SetWindowText(m_title);

	return TRUE;
}

void CLiveCreateTaskWaitDlg::OnCancel()
{
	m_pTask->Cancel();
	CDialog::OnCancel();
}

void CLiveCreateTaskWaitDlg::OnTimer(UINT_PTR nTimerID)
{
	if (nTimerID == ID_TIMER_UPDATE_STATUS)
	{
		// update the progress text
		{
			m_counter += 1;

			CString text = m_baseText;

			const uint32 numDots = m_counter % 5;
			for (uint32 i=0; i<numDots; ++i)
			{
				text += ".";
			}
			
			CWnd* pText = GetDlgItem(IDC_TASK_PROGRESS_TEXT);
			if (pText != NULL)
			{
				pText->SetWindowText(text);
			}
		}

		// task has finished
		if (m_pTask->HasFinished())
		{
			CDialog::OnOK();
		}
	}
}

#endif