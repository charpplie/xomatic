/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2004.
-------------------------------------------------------------------------
$Id: ProgressDialog.h,v 1.1 2009/01/19 17:14:31 PauloZaffari Exp wwwrun $
$DateTime$
Description:  This is the header file for the general utility dialog for
progress display. The purpose of this dialog, as one might imagine, is to 
display the progress of any process.

Usage Hint: rename the dialog.
-------------------------------------------------------------------------
History:
- 19:01:2009   17:14: Created by Paulo Zaffari
*************************************************************************/

#ifndef ProgressDialog_h__
#define ProgressDialog_h__

#pragma once

class CProgressDialog:public CDialog
{
	//////////////////////////////////////////////////////////////////////////
	// Types & Typedefs
	public:
		typedef Functor0		TDCancelCallback;
	protected:
	private:
	//////////////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////////////
	// Methods
	public:
		CProgressDialog(CWnd*	poParentWindow=NULL);
		CProgressDialog(CString strText,CString strTitle,int nMinimum=0,int nMaximum=100,CWnd*	poParentWindow=NULL);

		BOOL	Create();

		void DoDataExchange(CDataExchange* pDX);
		BOOL OnInitDialog();

		afx_msg void OnCancel();

		void SetRange(int nMinimum, int nMaximum);
		void SetPosition(int nPosition);
		void SetText(CString strText);
		void SetTitle(CString strTitle);
		void SetCancelCallback(TDCancelCallback pfnCancelCallback);


		DECLARE_MESSAGE_MAP();
		//}}AFX_MSG
	protected:
	private:
	//////////////////////////////////////////////////////////////////////////


	//////////////////////////////////////////////////////////////////////////
	// Data fields
	public:
	protected:
		CString							m_strTitle;
		CString							m_strText;

		CProgressCtrl				m_oProgressControl;
		CButton							m_oCancelButton;

		int									m_nMinimum;
		int									m_nMaximum;
		int									m_nPosition;

		TDCancelCallback		m_pfnCancelCallback;

		bool								m_boCreated;
	private:
	//////////////////////////////////////////////////////////////////////////


};


#endif // ProgressDialog_h__