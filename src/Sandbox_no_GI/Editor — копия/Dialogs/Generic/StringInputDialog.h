/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2004.
-------------------------------------------------------------------------
$Id: StringInputDialog.h,v 1.1 2008/11/21 11:05:05 PauloZaffari Exp wwwrun $
$DateTime$
Description:  This is the header file for the general utility dialog for
string input. The purpose of this dialog, as one might imagine, is to get
string input for any purpose necessary.
              The recomended way to call this dialog is through DoModal() 
method.

Usage Hint: rename dialog.
-------------------------------------------------------------------------
History:
- 21:11:2008   11:05: Created by Paulo Zaffari
*************************************************************************/

#ifndef StringInputDialog_h__
#define StringInputDialog_h__

#pragma once

class CStringInputDialog : public CDialog
{
	//////////////////////////////////////////////////////////////////////////
	// Types & Typedefs
	public:
	protected:
	private:
	//////////////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////////////
	// Methods
	public:
		CStringInputDialog();
		CStringInputDialog(CString strText,CString strTitle);
		void DoDataExchange(CDataExchange* pDX);
		BOOL OnInitDialog();
		void OnOK();

		void SetText(CString strText);
		void SetTitle(CString strTitle);

		CString GetResultingText();
	protected:
	private:
	//////////////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////////////
	// Data fields
	public:
	protected:
		CString   m_strTitle;
		CString   m_strText;
		CEdit     m_nameEdit;
	private:
	//////////////////////////////////////////////////////////////////////////
};

#endif // StringInputDialog_h__
