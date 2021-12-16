/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2004.
-------------------------------------------------------------------------
$Id: ProgressDialog.cpp,v 1.1 2009/01/19 17:14:31 PauloZaffari Exp wwwrun $
$DateTime$
Description:  This is the source file for the general utility dialog for
progress display. The purpose of this dialog, as one might imagine, is to 
display the progress of any process.

Usage Hint: rename dialog.
-------------------------------------------------------------------------
History:
- 19:01:2009   17:14: Created by Paulo Zaffari
*************************************************************************/

#include "StdAfx.h"
#include "ProgressDialog.h"

BEGIN_MESSAGE_MAP(CProgressDialog, CDialog)
	ON_COMMAND(IDC_GENERAL_PROGRESS_CANCEL_BUTTON,&CProgressDialog::OnCancel)
END_MESSAGE_MAP()

//////////////////////////////////////////////////////////////////////////
CProgressDialog::CProgressDialog(CWnd*	poParentWindow):
CDialog(IDD_GENERAL_PROGRESS_DIALOG),
m_strText(""),
m_strTitle("Current progress:"),
m_nMinimum(0),
m_nMaximum(100),
m_nPosition(0)
{
	CDialog::Create(IDD_GENERAL_PROGRESS_DIALOG,poParentWindow);
}
//////////////////////////////////////////////////////////////////////////
CProgressDialog::CProgressDialog(CString strText,CString strTitle,int nMinimum,int nMaximum,CWnd*	poParentWindow):
CDialog(IDD_GENERAL_PROGRESS_DIALOG),
m_strText(strText),
m_strTitle(strTitle),
m_nMinimum(nMinimum),
m_nMaximum(nMaximum),
m_nPosition(nMinimum)
{
	CDialog::Create(IDD_GENERAL_PROGRESS_DIALOG,poParentWindow);
}
//////////////////////////////////////////////////////////////////////////
void CProgressDialog::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_GENERAL_PROGRESS_PROGRESS_BAR, m_oProgressControl);
	DDX_Control(pDX,IDC_GENERAL_PROGRESS_CANCEL_BUTTON,m_oCancelButton);
	DDX_Text(pDX,IDC_GENERAL_PROGRESS_TEXT, m_strText);	
}
//////////////////////////////////////////////////////////////////////////
BOOL CProgressDialog::OnInitDialog()
{
	if (__super::OnInitDialog() == FALSE)
		return FALSE;

	SetWindowText(m_strTitle);

	UpdateData(FALSE);

	if (!m_pfnCancelCallback)
	{
		m_oCancelButton.ShowWindow(SW_HIDE);
	}

	return TRUE;
}
//////////////////////////////////////////////////////////////////////////
void CProgressDialog::OnCancel()
{
	if (m_pfnCancelCallback)
	{
		m_pfnCancelCallback();
	}
}
//////////////////////////////////////////////////////////////////////////
void CProgressDialog::SetRange(int nMinimum, int nMaximum)
{
	m_oProgressControl.SetRange32(nMinimum,nMaximum);
}
//////////////////////////////////////////////////////////////////////////
void CProgressDialog::SetPosition(int nPosition)
{
	m_oProgressControl.SetPos(nPosition);
}
//////////////////////////////////////////////////////////////////////////
void CProgressDialog::SetText(CString strText)
{
	m_strText=strText;
	SetDlgItemText(IDC_GENERAL_PROGRESS_TEXT,strText);
}
//////////////////////////////////////////////////////////////////////////
void CProgressDialog::SetTitle(CString strTitle)
{
	m_strTitle=strTitle;
	SetWindowText(m_strTitle);
}
//////////////////////////////////////////////////////////////////////////
void CProgressDialog::SetCancelCallback(TDCancelCallback pfnCancelCallback)
{
	m_pfnCancelCallback=pfnCancelCallback;
	m_oCancelButton.ShowWindow(SW_SHOW);
}
//////////////////////////////////////////////////////////////////////////
