//////////////////////////////////  CRYTEK  ////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2011.
// -------------------------------------------------------------------------
//  File Name        : ScriptTermDialog.h
//  Author           : Jaewon Jung
//  Time of creation : 4/19/2011   15:16
//  Compilers        : VS2008
//  Description      : Dialog for python script terminal
// -------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////

#ifndef __SCRIPT_TERM_DIALOG_H__
#define __SCRIPT_TERM_DIALOG_H__
#pragma once

#include "Controls/ACEdit.h"
#include "Util/BoostPythonHelpers.h"

#define SCRIPT_TERM_WINDOW_NAME "Script Terminal"

class CScriptTermDialog : public CXTResizeDialog, public PyScript::IPyScriptListener
{
	DECLARE_DYNCREATE(CScriptTermDialog)
public:
	CScriptTermDialog(CWnd* pParent = NULL);
	~CScriptTermDialog();

	enum { IDD = IDD_SCRIPT_TERMINAL };

	void AppendText(const char *pText);
	void AppendError(const char *pText);

private:
	virtual void DoDataExchange(CDataExchange* pDX);
	afx_msg void OnScriptHelp();

	DECLARE_MESSAGE_MAP()

	virtual BOOL OnInitDialog();
	virtual void PostNcDestroy();
	virtual void OnOK();
	virtual void OnCancel();

	void ExecuteAndPrint(const char *cmd);

	void AppendToConsole(CString string, COLORREF color);
	int GetNumVisibleLines();

	virtual void OnStdOut(const char *pString) { AppendText(pString); }
	virtual void OnStdErr(const char *pString) { AppendError(pString); }

	CRichEditCtrl m_outputCtrl;
	CFont m_outputFont;
	CACEdit m_inputCtrl;
};

#endif // __SCRIPT_TERM_DIALOG_H__