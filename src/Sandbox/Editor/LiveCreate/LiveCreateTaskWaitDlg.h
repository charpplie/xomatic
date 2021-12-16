#pragma once
////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 2001-2012
// -------------------------------------------------------------------------
//  Created:	8 Mar 2012 by Nicusor Nedelcu
//  Description: This file defines the LiveCreate view, used to host settings
//							for LiveCreate module
//
////////////////////////////////////////////////////////////////////////////
#include "IEditor.h"
#include "IBackgroundTaskManager.h"
#include "Dialogs/BaseFrameWnd.h"

#ifndef NO_LIVECREATE

class CLiveCreateTaskWaitDlg : public CDialog
{
public:
	static bool ShowDialog(CWnd* pParent, IBackgroundTask* pTask, const CString& title, const CString& text);

protected:
	DECLARE_MESSAGE_MAP()

	CLiveCreateTaskWaitDlg(CWnd* pParent, IBackgroundTask* pTask, const CString& title, const CString& text);
	virtual ~CLiveCreateTaskWaitDlg();

	virtual BOOL OnInitDialog();
	afx_msg	void OnCancel();
	afx_msg	void OnTimer(UINT_PTR nTimerID);

	CString m_baseText;
	CString m_title;
	int m_counter;
	IBackgroundTask* m_pTask;
};

#endif