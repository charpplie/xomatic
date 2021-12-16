/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2004.
-------------------------------------------------------------------------
$Id: StringInputDialog.h,v 1.1 2008/11/26 18:43:51 PauloZaffari Exp wwwrun $
$DateTime$
Description:  This is the header file for the general utility dialog for
tree element selection. The purpose of this dialog, as one might imagine, 
is to allow the user to select any give tree element from a tree structure
for any purpose necessary.
The recomended way to call this dialog is through DoModal()method, but don't
check its return value. Instead, set up callbacks. If you don't set up the
callbacks, it may be troublesome to set up the initial data and to get data 
from the dialog as well.

Usage Hint: rename dialog.
-------------------------------------------------------------------------
History:
- 28:11:2008   18:43: Created by Paulo Zaffari
*************************************************************************/

#ifndef TreeElementSelectionDialog_h__
#define TreeElementSelectionDialog_h__

#pragma once;

#include "functor.h"

class CTreeElementSelectionDialog: public CDialog
{
	//////////////////////////////////////////////////////////////////////////
	// Types and typedefs.
	public:
		typedef Functor1<CTreeElementSelectionDialog*>	TDCallback;
	protected:
	private:
	//////////////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////////////
	// Methods.
	public:
		CTreeElementSelectionDialog(CString strCaption);

		// The below 2 functions make a COPY of the elements...
		// ... so don't expect them to be updated if you change them after
		// Setting them.
		void SetImageList(CImageList*	poSourceImageList);
		void SetFont(CFont*	poSourceFont);

		void SetInitCallback(TDCallback		fnInitCallback);
		void SetOkCallback(TDCallback			fnOkCallback);
		void SetCancelCallback(TDCallback			fnCancelCallback);

		void DoDataExchange(CDataExchange* pDX);
		BOOL OnInitDialog();
		void OnOK();
		void OnCancel();

		// This functions must be called after either, the create method is called, or better, the do modal is.
		HTREEITEM	InsertItem(const string& rstrItemtext,HTREEITEM hParentItem=TVI_ROOT,DWORD_PTR nItemData=0,int nItemUnselectedImage=0,int nItemSelectedImage=0);
		HTREEITEM	InsertItem(const string& rstrItemtext,HTREEITEM hParentItem=TVI_ROOT,HTREEITEM hAfterWhatItem=TVI_LAST,DWORD_PTR nItemData=0,int nItemUnselectedImage=0,int nItemSelectedImage=0);

		CTreeCtrl& GetTreeControl();
	protected:
	private:
	//////////////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////////////
	// Data.
	public:		
	protected:
		CString				m_strCaption;
		CTreeCtrl			m_oTreeControl;
		bool					m_boIsInitialized;		
		TDCallback		m_fnInitCallback;
		TDCallback		m_fnOkCallback;
		TDCallback		m_fnCancelCallback;
		CFont					m_oFont;
		CImageList		m_oImageList;
	private:
	//////////////////////////////////////////////////////////////////////////
};

#endif // TreeElementSelectionDialog_h__